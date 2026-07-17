#![allow(unsafe_code)]

//! 以 Rust 實作 tree-sitter 的 thread-local slab allocator direct slice。
//!
//! 此模組保留 C allocator 的 64-byte slab、heap fallback、realloc promotion、
//! reclaim 與 tree-sitter allocator callback ABI。頁面本身仍透過 C `malloc`/
//! `free` 配置，讓正式版的 mimalloc override 與既有 C runtime 保持一致。

use core::ffi::c_void;
use std::cell::RefCell;
use std::mem;
use std::ptr;

const SLAB_CHUNK_SIZE: usize = 64;
const SLAB_PAGE_CHUNKS: usize = 1024;
const SLAB_PAGE_SIZE: usize = SLAB_CHUNK_SIZE * SLAB_PAGE_CHUNKS;

#[repr(C)]
struct SlabFreeNode {
    next: *mut SlabFreeNode,
}

struct SlabState {
    pages: Vec<*mut u8>,
    freelist: *mut SlabFreeNode,
}

impl SlabState {
    const fn new() -> Self {
        Self {
            pages: Vec::new(),
            freelist: ptr::null_mut(),
        }
    }
}

thread_local! {
    static TLS_SLAB: RefCell<SlabState> = const { RefCell::new(SlabState::new()) };
}

unsafe extern "C" {
    fn malloc(size: usize) -> *mut c_void;
    fn free(ptr: *mut c_void);
    fn realloc(ptr: *mut c_void, size: usize) -> *mut c_void;
}

#[cfg(not(test))]
unsafe extern "C" {
    fn ts_set_allocator(
        new_malloc: Option<unsafe extern "C" fn(usize) -> *mut c_void>,
        new_calloc: Option<unsafe extern "C" fn(usize, usize) -> *mut c_void>,
        new_realloc: Option<unsafe extern "C" fn(*mut c_void, usize) -> *mut c_void>,
        new_free: Option<unsafe extern "C" fn(*mut c_void)>,
    );
}

fn with_state<T>(f: impl FnOnce(&mut SlabState) -> T) -> T {
    TLS_SLAB.with(|state| f(&mut state.borrow_mut()))
}

unsafe fn push_free(state: &mut SlabState, ptr: *mut u8) {
    let node = ptr.cast::<SlabFreeNode>();
    (*node).next = state.freelist;
    state.freelist = node;
}

unsafe fn slab_grow(state: &mut SlabState) -> bool {
    let page = malloc(SLAB_PAGE_SIZE).cast::<u8>();
    if page.is_null() {
        return false;
    }
    if state.pages.try_reserve(1).is_err() {
        free(page.cast::<c_void>());
        return false;
    }
    state.pages.push(page);
    for index in 0..SLAB_PAGE_CHUNKS {
        push_free(state, page.add(index * SLAB_CHUNK_SIZE));
    }
    true
}

fn slab_owns(state: &SlabState, ptr: *const c_void) -> bool {
    let address = ptr as usize;
    state.pages.iter().any(|&page| {
        let start = page as usize;
        address >= start && address - start < SLAB_PAGE_SIZE
    })
}

unsafe fn slab_malloc(size: usize) -> *mut c_void {
    let size = if size == 0 { 1 } else { size };
    if size > SLAB_CHUNK_SIZE {
        return malloc(size);
    }

    with_state(|state| unsafe {
        if state.freelist.is_null() && !slab_grow(state) {
            return malloc(size);
        }
        let node = state.freelist;
        state.freelist = (*node).next;
        node.cast::<c_void>()
    })
}

unsafe fn slab_calloc(count: usize, size: usize) -> *mut c_void {
    if count != 0 && size > usize::MAX / count {
        return ptr::null_mut();
    }
    let total = count * size;
    let result = slab_malloc(total);
    if !result.is_null() {
        ptr::write_bytes(result.cast::<u8>(), 0, total);
    }
    result
}

unsafe fn slab_realloc(ptr_value: *mut c_void, new_size: usize) -> *mut c_void {
    if ptr_value.is_null() {
        return slab_malloc(new_size);
    }
    if new_size == 0 {
        with_state(|state| unsafe {
            if slab_owns(state, ptr_value) {
                push_free(state, ptr_value.cast::<u8>());
            } else {
                free(ptr_value);
            }
        });
        return ptr::null_mut();
    }

    with_state(|state| unsafe {
        if !slab_owns(state, ptr_value) {
            return realloc(ptr_value, new_size);
        }
        if new_size <= SLAB_CHUNK_SIZE {
            return ptr_value;
        }
        let new_ptr = malloc(new_size);
        if new_ptr.is_null() {
            return ptr::null_mut();
        }
        ptr::copy_nonoverlapping(
            ptr_value.cast::<u8>(),
            new_ptr.cast::<u8>(),
            SLAB_CHUNK_SIZE,
        );
        push_free(state, ptr_value.cast::<u8>());
        new_ptr
    })
}

unsafe fn slab_free(ptr_value: *mut c_void) {
    if ptr_value.is_null() {
        return;
    }
    with_state(|state| unsafe {
        if slab_owns(state, ptr_value) {
            push_free(state, ptr_value.cast::<u8>());
        } else {
            free(ptr_value);
        }
    });
}

#[cfg(not(test))]
unsafe extern "C" fn slab_malloc_callback(size: usize) -> *mut c_void {
    slab_malloc(size)
}

#[cfg(not(test))]
unsafe extern "C" fn slab_calloc_callback(count: usize, size: usize) -> *mut c_void {
    slab_calloc(count, size)
}

#[cfg(not(test))]
unsafe extern "C" fn slab_realloc_callback(ptr_value: *mut c_void, size: usize) -> *mut c_void {
    slab_realloc(ptr_value, size)
}

#[cfg(not(test))]
unsafe extern "C" fn slab_free_callback(ptr_value: *mut c_void) {
    slab_free(ptr_value);
}

/// 將 Rust allocator callback 設定給 tree-sitter。
///
/// # Safety
/// 呼叫端必須確保 tree-sitter runtime 已連結，且 callback 設定後由同一套
/// allocator 規則管理其配置；此函式不應在 parser 正在使用期間重複切換。
#[no_mangle]
pub unsafe extern "C" fn cbm_slab_install() {
    #[cfg(not(test))]
    ts_set_allocator(
        Some(slab_malloc_callback),
        Some(slab_calloc_callback),
        Some(slab_realloc_callback),
        Some(slab_free_callback),
    );
}

/// 重建目前執行緒所有 slab page 的 free-list。
///
/// # Safety
/// 呼叫端必須先確保目前執行緒沒有仍被使用的 tree-sitter allocation。
#[no_mangle]
pub unsafe extern "C" fn cbm_slab_reset_thread() {
    with_state(|state| unsafe {
        state.freelist = ptr::null_mut();
        for page_index in 0..state.pages.len() {
            let page = state.pages[page_index];
            for index in 0..SLAB_PAGE_CHUNKS {
                push_free(state, page.add(index * SLAB_CHUNK_SIZE));
            }
        }
    });
}

unsafe fn free_pages(state: &mut SlabState) {
    for page in mem::take(&mut state.pages) {
        free(page.cast::<c_void>());
    }
    state.freelist = ptr::null_mut();
}

/// 釋放目前執行緒的所有 slab page 並清除 allocator 狀態。
///
/// # Safety
/// 呼叫端必須確保目前執行緒沒有任何仍存活的 tree-sitter allocation。
#[no_mangle]
pub unsafe extern "C" fn cbm_slab_destroy_thread() {
    with_state(|state| unsafe { free_pages(state) });
}

/// 回收目前執行緒的所有 slab page，但保留 allocator 可再次配置的狀態。
///
/// # Safety
/// 只有在 tree-sitter tree 與 parser 都已釋放後才能呼叫。
#[no_mangle]
pub unsafe extern "C" fn cbm_slab_reclaim() {
    with_state(|state| unsafe { free_pages(state) });
}

/// 配置一塊供 C 測試與診斷使用的 slab/heap 記憶體。
///
/// # Safety
/// 回傳指標必須只交給相容的 `cbm_slab_test_*` API 釋放或重配置。
#[no_mangle]
pub unsafe extern "C" fn cbm_slab_test_malloc(size: usize) -> *mut c_void {
    slab_malloc(size)
}

/// 釋放由 slab 測試 API 配置的記憶體。
///
/// # Safety
/// `ptr_value` 必須是尚未釋放的相容配置指標，或為 NULL。
#[no_mangle]
pub unsafe extern "C" fn cbm_slab_test_free(ptr_value: *mut c_void) {
    slab_free(ptr_value);
}

/// 重配置由 slab 測試 API 配置的記憶體。
///
/// # Safety
/// `ptr_value` 必須是尚未釋放的相容配置指標，或為 NULL。
#[no_mangle]
pub unsafe extern "C" fn cbm_slab_test_realloc(ptr_value: *mut c_void, size: usize) -> *mut c_void {
    slab_realloc(ptr_value, size)
}

/// 配置歸零的 slab 測試記憶體。
///
/// # Safety
/// 回傳指標必須只交給相容的 `cbm_slab_test_*` API 釋放或重配置。
#[no_mangle]
pub unsafe extern "C" fn cbm_slab_test_calloc(count: usize, size: usize) -> *mut c_void {
    slab_calloc(count, size)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn small_alloc_reuses_and_heap_path_works() {
        unsafe {
            let first = cbm_slab_test_malloc(32);
            assert!(!first.is_null());
            *first.cast::<u8>() = 0x42;
            cbm_slab_test_free(first);

            let second = cbm_slab_test_malloc(32);
            assert!(!second.is_null());
            cbm_slab_test_free(second);

            let heap = cbm_slab_test_malloc(200);
            assert!(!heap.is_null());
            cbm_slab_test_free(heap);
            cbm_slab_destroy_thread();
        }
    }

    #[test]
    fn calloc_and_slab_to_heap_realloc_preserve_contract() {
        unsafe {
            let ptr_value = cbm_slab_test_calloc(1, 32);
            assert!(!ptr_value.is_null());
            assert_eq!(*ptr_value.cast::<u8>(), 0);
            *ptr_value.cast::<u8>() = 0x42;
            let promoted = cbm_slab_test_realloc(ptr_value, 200);
            assert!(!promoted.is_null());
            assert_eq!(*promoted.cast::<u8>(), 0x42);
            cbm_slab_test_free(promoted);
            cbm_slab_reclaim();
            cbm_slab_destroy_thread();
        }
    }
}
