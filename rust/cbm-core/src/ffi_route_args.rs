#![allow(unsafe_code)]

use std::ffi::{c_char, c_int, CStr};

use crate::pipeline::route;

#[no_mangle]
pub unsafe extern "C" fn cbm_pipeline_is_path_keyword(keyword: *const c_char) -> c_int {
    let keyword = if keyword.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(keyword) }.to_bytes())
    };
    if route::is_path_keyword(keyword) {
        1
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn cbm_pipeline_normalize_url_arg(
    url: *const c_char,
    norm: *mut c_char,
    norm_size: c_int,
) -> c_int {
    if url.is_null() || norm.is_null() || norm_size < 2 {
        return 0;
    }
    let url = Some(unsafe { CStr::from_ptr(url) }.to_bytes());
    let Some(normalized) = route::normalize_url_arg(url, norm_size as usize) else {
        return 0;
    };
    let norm = unsafe { std::slice::from_raw_parts_mut(norm.cast::<u8>(), norm_size as usize) };
    norm[..normalized.len()].copy_from_slice(&normalized);
    norm[normalized.len()] = 0;
    1
}
