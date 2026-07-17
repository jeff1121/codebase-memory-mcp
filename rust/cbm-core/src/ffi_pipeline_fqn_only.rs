#![allow(unsafe_code)]

//! `fqn.c` 的 Rust direct-only C ABI。

use core::ffi::{c_char, c_void};
use core::ptr;
use std::ffi::CStr;

unsafe extern "C" {
    fn malloc(size: usize) -> *mut c_void;
}

unsafe fn c_bytes<'a>(value: *const c_char) -> Option<&'a [u8]> {
    if value.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(value) }.to_bytes())
    }
}

unsafe fn malloc_string(bytes: &[u8]) -> *mut c_char {
    let Some(size) = bytes.len().checked_add(1) else {
        return ptr::null_mut();
    };
    let output = unsafe { malloc(size) }.cast::<u8>();
    if output.is_null() {
        return ptr::null_mut();
    }
    unsafe {
        ptr::copy_nonoverlapping(bytes.as_ptr(), output, bytes.len());
        *output.add(bytes.len()) = 0;
    }
    output.cast()
}

#[no_mangle]
/// # Safety
///
/// 參數必須是 null 或有效的 NUL 結尾 C string；回傳值由 C `malloc` 配置，呼叫端
/// 必須使用 `free()` 釋放。
pub unsafe extern "C" fn cbm_pipeline_fqn_compute(
    project: *const c_char,
    rel_path: *const c_char,
    name: *const c_char,
) -> *mut c_char {
    let output = crate::pipeline::fqn::fqn_compute(
        unsafe { c_bytes(project) },
        unsafe { c_bytes(rel_path) },
        unsafe { c_bytes(name) },
    );
    unsafe { malloc_string(&output) }
}

#[no_mangle]
/// # Safety
///
/// 參數與 `cbm_pipeline_fqn_compute` 相同。
pub unsafe extern "C" fn cbm_pipeline_fqn_module(
    project: *const c_char,
    rel_path: *const c_char,
) -> *mut c_char {
    let output =
        crate::pipeline::fqn::fqn_module(unsafe { c_bytes(project) }, unsafe { c_bytes(rel_path) });
    unsafe { malloc_string(&output) }
}

#[no_mangle]
/// # Safety
///
/// 參數與 `cbm_pipeline_fqn_compute` 相同。
pub unsafe extern "C" fn cbm_pipeline_fqn_module_dir(
    project: *const c_char,
    rel_path: *const c_char,
    module_is_dir: bool,
) -> *mut c_char {
    let output = crate::pipeline::fqn::fqn_module_dir(
        unsafe { c_bytes(project) },
        unsafe { c_bytes(rel_path) },
        module_is_dir,
    );
    unsafe { malloc_string(&output) }
}

#[no_mangle]
/// # Safety
///
/// 參數與 `cbm_pipeline_fqn_compute` 相同。
pub unsafe extern "C" fn cbm_pipeline_fqn_folder(
    project: *const c_char,
    rel_dir: *const c_char,
) -> *mut c_char {
    let output =
        crate::pipeline::fqn::fqn_folder(unsafe { c_bytes(project) }, unsafe { c_bytes(rel_dir) });
    unsafe { malloc_string(&output) }
}

#[no_mangle]
/// # Safety
///
/// 參數必須是 null 或有效的 NUL 結尾 C string；回傳值由 C `malloc` 配置，無法
/// 判定為 relative import 時回傳 null。
pub unsafe extern "C" fn cbm_pipeline_resolve_relative_import(
    source_rel: *const c_char,
    module_path: *const c_char,
) -> *mut c_char {
    let Some(output) =
        crate::pipeline::fqn::resolve_relative_import(unsafe { c_bytes(source_rel) }, unsafe {
            c_bytes(module_path)
        })
    else {
        return ptr::null_mut();
    };
    unsafe { malloc_string(&output) }
}
