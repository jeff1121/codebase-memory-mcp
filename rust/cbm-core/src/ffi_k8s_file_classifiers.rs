#![allow(unsafe_code)]

use std::ffi::{c_char, c_int, CStr};

use crate::pipeline::infrascan as pipeline_infrascan;

#[no_mangle]
pub unsafe extern "C" fn cbm_pipeline_is_helm_chart_file(name: *const c_char) -> c_int {
    let name = if name.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(name) }.to_bytes())
    };
    c_int::from(pipeline_infrascan::is_helm_chart_file(name))
}

#[no_mangle]
pub unsafe extern "C" fn cbm_pipeline_is_gomod_file(name: *const c_char) -> c_int {
    let name = if name.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(name) }.to_bytes())
    };
    c_int::from(pipeline_infrascan::is_gomod_file(name))
}

#[no_mangle]
pub unsafe extern "C" fn cbm_pipeline_is_requirements_file(name: *const c_char) -> c_int {
    let name = if name.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(name) }.to_bytes())
    };
    c_int::from(pipeline_infrascan::is_requirements_file(name))
}
