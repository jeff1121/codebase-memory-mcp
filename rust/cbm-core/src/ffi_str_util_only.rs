//! `str_util.c` 的 Rust direct ABI adapter。
//!
//! 字串結果一律配置在呼叫端提供的 C `CBMArena`；Rust 不回傳由 Rust allocator
//! 擁有的字串，也不保存任何 C 指標。

#![allow(unsafe_code)]

use std::ffi::{c_char, c_int, c_void};
use std::mem::size_of;
use std::ptr;

#[repr(C)]
pub struct CbmArena {
    _private: [u8; 0],
}

unsafe extern "C" {
    fn cbm_arena_alloc(arena: *mut CbmArena, size: usize) -> *mut c_void;
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `arena` 必須是有效的 C `CBMArena`；其餘參數遵循 `str_util.h` 契約。
pub unsafe extern "C" fn cbm_path_join(
    arena: *mut CbmArena,
    base: *const c_char,
    name: *const c_char,
) -> *mut c_char {
    let needed = unsafe { crate::ffi::cbm_rs_path_join(ptr::null_mut(), 0, base, name) };
    if needed == usize::MAX {
        return ptr::null_mut();
    }
    let Some(size) = needed.checked_add(1) else {
        return ptr::null_mut();
    };
    let output = unsafe { cbm_arena_alloc(arena, size) }.cast::<c_char>();
    if output.is_null() {
        return ptr::null_mut();
    }
    let written = unsafe { crate::ffi::cbm_rs_path_join(output, size, base, name) };
    if written != needed {
        return ptr::null_mut();
    }
    output
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `parts` 必須是 null，或指向至少 `n` 個 NUL 終止字串指標；`arena` 必須是有效的
/// C `CBMArena`。其餘參數遵循 `str_util.h` 契約。
pub unsafe extern "C" fn cbm_path_join_n(
    arena: *mut CbmArena,
    parts: *const *const c_char,
    n: c_int,
) -> *mut c_char {
    let needed = unsafe { crate::ffi::cbm_rs_path_join_n(ptr::null_mut(), 0, parts, n) };
    if needed == usize::MAX {
        return ptr::null_mut();
    }
    let Some(size) = needed.checked_add(1) else {
        return ptr::null_mut();
    };
    let output = unsafe { cbm_arena_alloc(arena, size) }.cast::<c_char>();
    if output.is_null() {
        return ptr::null_mut();
    }
    let written = unsafe { crate::ffi::cbm_rs_path_join_n(output, size, parts, n) };
    if written != needed {
        return ptr::null_mut();
    }
    output
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或指向 NUL 終止字串。
pub unsafe extern "C" fn cbm_path_ext(path: *const c_char) -> *const c_char {
    unsafe { crate::ffi::cbm_rs_path_ext(path) }
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或指向 NUL 終止字串。
pub unsafe extern "C" fn cbm_path_base(path: *const c_char) -> *const c_char {
    unsafe { crate::ffi::cbm_rs_path_base(path) }
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `arena` 必須是有效的 C `CBMArena`；`path` 必須是 null，或指向 NUL 終止字串。
pub unsafe extern "C" fn cbm_path_dir(arena: *mut CbmArena, path: *const c_char) -> *mut c_char {
    let needed = unsafe { crate::ffi::cbm_rs_path_dir(ptr::null_mut(), 0, path) };
    let Some(size) = needed.checked_add(1) else {
        return ptr::null_mut();
    };
    let output = unsafe { cbm_arena_alloc(arena, size) }.cast::<c_char>();
    if output.is_null() {
        return ptr::null_mut();
    }
    let written = unsafe { crate::ffi::cbm_rs_path_dir(output, size, path) };
    if written != needed {
        return ptr::null_mut();
    }
    output
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `s` 與 `prefix` 必須是 null，或指向 NUL 終止字串。
pub unsafe extern "C" fn cbm_str_starts_with(s: *const c_char, prefix: *const c_char) -> bool {
    unsafe { crate::ffi::cbm_rs_str_starts_with(s, prefix) != 0 }
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `s` 與 `suffix` 必須是 null，或指向 NUL 終止字串。
pub unsafe extern "C" fn cbm_str_ends_with(s: *const c_char, suffix: *const c_char) -> bool {
    unsafe { crate::ffi::cbm_rs_str_ends_with(s, suffix) != 0 }
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `s` 與 `sub` 必須是 null，或指向 NUL 終止字串。
pub unsafe extern "C" fn cbm_str_contains(s: *const c_char, sub: *const c_char) -> bool {
    unsafe { crate::ffi::cbm_rs_str_contains(s, sub) != 0 }
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `arena` 必須是有效的 C `CBMArena`；`s` 必須是 null，或指向 NUL 終止字串。
pub unsafe extern "C" fn cbm_str_tolower(arena: *mut CbmArena, s: *const c_char) -> *mut c_char {
    let needed = unsafe { crate::ffi::cbm_rs_str_tolower(ptr::null_mut(), 0, s) };
    if needed == usize::MAX {
        return ptr::null_mut();
    }
    let Some(size) = needed.checked_add(1) else {
        return ptr::null_mut();
    };
    let output = unsafe { cbm_arena_alloc(arena, size) }.cast::<c_char>();
    if output.is_null() {
        return ptr::null_mut();
    }
    if unsafe { crate::ffi::cbm_rs_str_tolower(output, size, s) } != needed {
        return ptr::null_mut();
    }
    output
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `arena` 必須是有效的 C `CBMArena`；`s` 必須是 null，或指向 NUL 終止字串。
pub unsafe extern "C" fn cbm_str_replace_char(
    arena: *mut CbmArena,
    s: *const c_char,
    from: c_char,
    to: c_char,
) -> *mut c_char {
    let needed = unsafe { crate::ffi::cbm_rs_str_replace_char(ptr::null_mut(), 0, s, from, to) };
    if needed == usize::MAX {
        return ptr::null_mut();
    }
    let Some(size) = needed.checked_add(1) else {
        return ptr::null_mut();
    };
    let output = unsafe { cbm_arena_alloc(arena, size) }.cast::<c_char>();
    if output.is_null() {
        return ptr::null_mut();
    }
    if unsafe { crate::ffi::cbm_rs_str_replace_char(output, size, s, from, to) } != needed {
        return ptr::null_mut();
    }
    output
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `arena` 必須是有效的 C `CBMArena`；`path` 必須是 null，或指向 NUL 終止字串。
pub unsafe extern "C" fn cbm_str_strip_ext(
    arena: *mut CbmArena,
    path: *const c_char,
) -> *mut c_char {
    let needed = unsafe { crate::ffi::cbm_rs_str_strip_ext(ptr::null_mut(), 0, path) };
    if needed == usize::MAX {
        return ptr::null_mut();
    }
    let Some(size) = needed.checked_add(1) else {
        return ptr::null_mut();
    };
    let output = unsafe { cbm_arena_alloc(arena, size) }.cast::<c_char>();
    if output.is_null() {
        return ptr::null_mut();
    }
    if unsafe { crate::ffi::cbm_rs_str_strip_ext(output, size, path) } != needed {
        return ptr::null_mut();
    }
    output
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `arena` 必須是有效的 C `CBMArena`；`s` 必須是 null，或指向 NUL 終止字串；
/// `out_count` 必須是可寫的 `int` 指標。
pub unsafe extern "C" fn cbm_str_split(
    arena: *mut CbmArena,
    s: *const c_char,
    delim: c_char,
    out_count: *mut c_int,
) -> *mut *mut c_char {
    if s.is_null() || out_count.is_null() {
        return ptr::null_mut();
    }
    let count = unsafe { crate::ffi::cbm_rs_str_split_count(s, delim) };
    if count == usize::MAX || count > c_int::MAX as usize {
        return ptr::null_mut();
    }
    let Some(array_size) = count
        .checked_add(1)
        .and_then(|items| items.checked_mul(size_of::<*mut c_char>()))
    else {
        return ptr::null_mut();
    };
    let output = unsafe { cbm_arena_alloc(arena, array_size) }.cast::<*mut c_char>();
    if output.is_null() {
        return ptr::null_mut();
    }
    for index in 0..count {
        let part_len =
            unsafe { crate::ffi::cbm_rs_str_split_part(ptr::null_mut(), 0, s, delim, index) };
        if part_len == usize::MAX {
            return ptr::null_mut();
        }
        let Some(part_size) = part_len.checked_add(1) else {
            return ptr::null_mut();
        };
        let part = unsafe { cbm_arena_alloc(arena, part_size) }.cast::<c_char>();
        if part.is_null() {
            return ptr::null_mut();
        }
        let written =
            unsafe { crate::ffi::cbm_rs_str_split_part(part, part_size, s, delim, index) };
        if written != part_len {
            return ptr::null_mut();
        }
        unsafe { *output.add(index) = part };
    }
    unsafe {
        *output.add(count) = ptr::null_mut();
        *out_count = count as c_int;
    }
    output
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `s` 必須是 null，或指向 NUL 終止字串。
pub unsafe extern "C" fn cbm_validate_shell_arg(s: *const c_char) -> bool {
    unsafe { crate::ffi::cbm_rs_validate_shell_arg(s) != 0 }
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向 NUL 終止字串。
pub unsafe extern "C" fn cbm_validate_project_name(name: *const c_char) -> bool {
    unsafe { crate::ffi::cbm_rs_validate_project_name(name) != 0 }
}

#[cfg(feature = "str-util-only")]
#[no_mangle]
/// # Safety
///
/// 參數遵循 `str_util.h` 的 caller-owned buffer 契約。
pub unsafe extern "C" fn cbm_json_escape(
    buf: *mut c_char,
    bufsize: c_int,
    src: *const c_char,
) -> c_int {
    unsafe { crate::ffi::cbm_rs_json_escape(buf, bufsize, src) }
}
