#![allow(unsafe_code)]

use std::ffi::CStr;
use std::os::raw::{c_char, c_int};
use std::ptr;

#[repr(C)]
pub struct CbmCompileFlags {
    pub include_paths: *mut *mut c_char,
    pub include_count: c_int,
    pub defines: *mut *mut c_char,
    pub define_count: c_int,
    pub standard: [c_char; 32],
}

extern "C" {
    fn strdup(s: *const c_char) -> *mut c_char;
}

/// # Safety
///
/// `f`, `args`, `i`, `directory` 必須符合 FFI 指標與邊界契約。
pub unsafe fn try_include_flag(
    f: *mut CbmCompileFlags,
    args: *const *const c_char,
    argc: c_int,
    i: *mut c_int,
    directory: *const c_char,
) -> bool {
    if f.is_null() || args.is_null() || i.is_null() {
        return false;
    }
    let idx = unsafe { *i };
    if idx < 0 || idx >= argc {
        return false;
    }
    let arg_ptr = unsafe { *args.add(idx as usize) };
    if arg_ptr.is_null() {
        return false;
    }
    let arg_bytes = unsafe { CStr::from_ptr(arg_ptr) }.to_bytes();
    if arg_bytes.starts_with(b"-I") {
        let mut path_ptr = unsafe { arg_ptr.add(2) };
        if unsafe { *path_ptr } == 0 && idx + 1 < argc {
            unsafe { *i = idx + 1 };
            path_ptr = unsafe { *args.add((idx + 1) as usize) };
        }
        if !path_ptr.is_null() && unsafe { *path_ptr } != 0 {
            let res = resolve_path(path_ptr, directory);
            if !res.is_null() {
                let count = unsafe { (*f).include_count as usize };
                unsafe {
                    *(*f).include_paths.add(count) = res;
                    (*f).include_count += 1;
                }
            }
        }
        return true;
    }
    if arg_bytes == b"-isystem" && idx + 1 < argc {
        unsafe { *i = idx + 1 };
        let path_ptr = unsafe { *args.add((idx + 1) as usize) };
        if !path_ptr.is_null() {
            let res = resolve_path(path_ptr, directory);
            if !res.is_null() {
                let count = unsafe { (*f).include_count as usize };
                unsafe {
                    *(*f).include_paths.add(count) = res;
                    (*f).include_count += 1;
                }
            }
        }
        return true;
    }
    false
}

/// # Safety
///
/// `f`, `args`, `i` 必須符合 FFI 指標與邊界契約。
pub unsafe fn try_define_flag(
    f: *mut CbmCompileFlags,
    args: *const *const c_char,
    argc: c_int,
    i: *mut c_int,
) -> bool {
    if f.is_null() || args.is_null() || i.is_null() {
        return false;
    }
    let idx = unsafe { *i };
    if idx < 0 || idx >= argc {
        return false;
    }
    let arg_ptr = unsafe { *args.add(idx as usize) };
    if arg_ptr.is_null() {
        return false;
    }
    let arg_bytes = unsafe { CStr::from_ptr(arg_ptr) }.to_bytes();
    if !arg_bytes.starts_with(b"-D") {
        return false;
    }
    let mut def_ptr = unsafe { arg_ptr.add(2) };
    if unsafe { *def_ptr } == 0 && idx + 1 < argc {
        unsafe { *i = idx + 1 };
        def_ptr = unsafe { *args.add((idx + 1) as usize) };
    }
    if !def_ptr.is_null() && unsafe { *def_ptr } != 0 {
        let res = unsafe { strdup(def_ptr) };
        if !res.is_null() {
            let count = unsafe { (*f).define_count as usize };
            unsafe {
                *(*f).defines.add(count) = res;
                (*f).define_count += 1;
            }
        }
    }
    true
}

unsafe fn resolve_path(path: *const c_char, directory: *const c_char) -> *mut c_char {
    if path.is_null() {
        return ptr::null_mut();
    }
    if unsafe { *path } == b'/' as c_char {
        return unsafe { strdup(path) };
    }
    if !directory.is_null() && unsafe { *directory } != 0 {
        let dir_str = unsafe { CStr::from_ptr(directory) }.to_string_lossy();
        let path_str = unsafe { CStr::from_ptr(path) }.to_string_lossy();
        let joined = format!("{}/{}", dir_str, path_str);
        if let Ok(c_str) = std::ffi::CString::new(joined) {
            return unsafe { strdup(c_str.as_ptr()) };
        }
    }
    unsafe { strdup(path) }
}
