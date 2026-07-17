//! `semantic.c` 的 config/env direct-only replacement。
//!
//! 只取代固定 `cbm_sem_config_t` defaults、threshold override 與 enabled gate；
//! semantic corpus 與 scoring state 仍由 C 提供。

#![allow(unsafe_code)]

use std::os::raw::{c_char, c_double, c_float, c_int};

unsafe extern "C" {
    fn getenv(name: *const c_char) -> *const c_char;
    fn strtod(value: *const c_char, end: *mut *mut c_char) -> c_double;
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct CbmSemConfig {
    pub w_tfidf: c_float,
    pub w_ri: c_float,
    pub w_minhash: c_float,
    pub w_api: c_float,
    pub w_type: c_float,
    pub w_decorator: c_float,
    pub w_struct_profile: c_float,
    pub w_dataflow: c_float,
    pub threshold: c_float,
    pub max_edges: c_int,
}

const DEFAULT_THRESHOLD: c_float = 0.75;

/// 讀取 semantic scoring config，保留 C defaults 與 threshold env override。
///
/// # Safety
/// 此函式不接受 caller pointers；程序環境必須符合 C `getenv`/`strtod` 契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_sem_get_config() -> CbmSemConfig {
    let mut config = CbmSemConfig {
        w_tfidf: 0.20,
        w_ri: 0.25,
        w_minhash: 0.10,
        w_api: 0.15,
        w_type: 0.10,
        w_decorator: 0.05,
        w_struct_profile: 0.10,
        w_dataflow: 0.05,
        threshold: DEFAULT_THRESHOLD,
        max_edges: 10,
    };

    let name = c"CBM_SEMANTIC_THRESHOLD";
    let threshold = getenv(name.as_ptr());
    if !threshold.is_null() {
        let mut end = std::ptr::null_mut();
        let parsed = strtod(threshold, &mut end);
        if !std::ptr::eq(end, threshold) && parsed > 0.0 && parsed <= 1.0 {
            config.threshold = parsed as c_float;
        }
    }
    config
}

/// 檢查 `CBM_SEMANTIC_ENABLED` 是否以字元 `1` 開頭。
///
/// # Safety
/// 此函式不接受 caller pointers；程序環境必須符合 C `getenv` 契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_sem_is_enabled() -> bool {
    let name = c"CBM_SEMANTIC_ENABLED";
    let value = getenv(name.as_ptr());
    !value.is_null() && (*value as u8) == b'1'
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn default_config_matches_c_contract() {
        std::env::remove_var("CBM_SEMANTIC_THRESHOLD");
        let config = unsafe { cbm_sem_get_config() };
        assert_eq!(config.w_tfidf, 0.20);
        assert_eq!(config.w_ri, 0.25);
        assert_eq!(config.w_minhash, 0.10);
        assert_eq!(config.w_api, 0.15);
        assert_eq!(config.w_type, 0.10);
        assert_eq!(config.w_decorator, 0.05);
        assert_eq!(config.w_struct_profile, 0.10);
        assert_eq!(config.w_dataflow, 0.05);
        assert_eq!(config.threshold, 0.75);
        assert_eq!(config.max_edges, 10);
    }

    #[test]
    fn enabled_gate_is_boolean() {
        std::env::remove_var("CBM_SEMANTIC_ENABLED");
        assert!(!unsafe { cbm_sem_is_enabled() });
    }
}
