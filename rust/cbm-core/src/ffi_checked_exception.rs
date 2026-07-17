#![allow(unsafe_code)]

use std::ffi::{c_char, c_int, CStr};

use crate::pipeline::exception;

#[no_mangle]
pub unsafe extern "C" fn cbm_pipeline_is_checked_exception(name: *const c_char) -> c_int {
    let name = if name.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(name) }.to_bytes())
    };
    if exception::is_checked_exception(name) {
        1
    } else {
        0
    }
}
