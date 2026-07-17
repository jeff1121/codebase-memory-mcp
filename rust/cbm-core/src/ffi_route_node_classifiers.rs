#![allow(unsafe_code)]

use std::ffi::CStr;
use std::os::raw::{c_char, c_int};

use crate::pipeline::route as pipeline_route;

#[no_mangle]
pub unsafe extern "C" fn cbm_pipeline_is_hash_segment(segment: *const u8, len: usize) -> c_int {
    if segment.is_null() || len == 0 {
        return 0;
    }
    let segment = unsafe { std::slice::from_raw_parts(segment, len) };
    if pipeline_route::is_hash_segment(Some(segment), 12, 3) {
        1
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn cbm_pipeline_is_broker_route(qn: *const c_char) -> c_int {
    let qn = if qn.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(qn).to_bytes() })
    };
    c_int::from(pipeline_route::is_broker_route(qn))
}
