#![allow(unsafe_code)]

//! 以 Rust 實作 profiling runtime 的 direct slice。
//!
//! `cbm_profile_active` 與四個 C API 保持原 ABI；時間取得仍使用平台的
//! monotonic clock，結構化 log 的 variadic 呼叫則留在固定參數的 C bridge。

use core::ffi::{c_char, c_int, c_long};
use std::ffi::CStr;

const PROFILE_ENV: &[u8] = b"CBM_PROFILE\0";
const CLOCK_MONOTONIC: c_int = if cfg!(target_os = "macos") { 6 } else { 1 };

#[repr(C)]
pub struct CbmTimespec {
    tv_sec: i64,
    tv_nsec: c_long,
}

unsafe extern "C" {
    fn getenv(name: *const c_char) -> *const c_char;
}

#[cfg(not(windows))]
unsafe extern "C" {
    fn clock_gettime(clk_id: c_int, ts: *mut CbmTimespec) -> c_int;
}

#[cfg(windows)]
unsafe extern "C" {
    fn cbm_clock_gettime(clk_id: c_int, ts: *mut CbmTimespec) -> c_int;
}

#[cfg(not(test))]
unsafe extern "C" {
    fn cbm_log_profile_elapsed(
        phase: *const c_char,
        sub: *const c_char,
        ms: c_long,
        us: c_long,
        items: c_long,
    );
}

/// 與 C `bool cbm_profile_active` 相同名稱與配置的全域狀態。
#[no_mangle]
pub static mut cbm_profile_active: bool = false;

fn profile_env_enabled_bytes(value: Option<&[u8]>) -> bool {
    value.is_some_and(|bytes| !bytes.is_empty() && bytes[0] != b'0')
}

unsafe fn profile_env_enabled(value: *const c_char) -> bool {
    if value.is_null() {
        return false;
    }
    profile_env_enabled_bytes(Some(CStr::from_ptr(value).to_bytes()))
}

unsafe fn profile_clock_gettime(ts: *mut CbmTimespec) {
    #[cfg(not(windows))]
    {
        let _ = clock_gettime(CLOCK_MONOTONIC, ts);
    }
    #[cfg(windows)]
    {
        let _ = cbm_clock_gettime(CLOCK_MONOTONIC, ts);
    }
}

#[allow(clippy::unnecessary_cast)]
fn elapsed_us(start: &CbmTimespec, now: &CbmTimespec) -> i64 {
    (now.tv_sec - start.tv_sec) * 1_000_000 + (now.tv_nsec as i64 - start.tv_nsec as i64) / 1_000
}

/// 從 `CBM_PROFILE` 初始化 profiling active flag。
#[no_mangle]
pub extern "C" fn cbm_profile_init() {
    unsafe {
        if profile_env_enabled(getenv(PROFILE_ENV.as_ptr().cast())) {
            cbm_profile_active = true;
        }
    }
}

/// 強制啟用 profiling。
#[no_mangle]
pub extern "C" fn cbm_profile_enable() {
    unsafe {
        cbm_profile_active = true;
    }
}

/// 取得 monotonic timestamp。
///
/// # Safety
/// `ts` 必須指向可寫入且符合 C `struct timespec` ABI 的記憶體。
#[no_mangle]
pub unsafe extern "C" fn cbm_profile_now(ts: *mut CbmTimespec) {
    profile_clock_gettime(ts);
}

/// 記錄 profiling elapsed log。
///
/// # Safety
/// `phase`、`sub` 與 `start` 必須是有效的 C 字串/指標，且 `start` 指向
/// `cbm_profile_now` 填入的 timespec。
#[no_mangle]
pub unsafe extern "C" fn cbm_profile_log_elapsed(
    phase: *const c_char,
    sub: *const c_char,
    start: *const CbmTimespec,
    items: c_long,
) {
    let mut now = CbmTimespec {
        tv_sec: 0,
        tv_nsec: 0,
    };
    profile_clock_gettime(&mut now);
    let us = elapsed_us(&*start, &now);
    let ms = us / 1_000;

    #[cfg(not(test))]
    cbm_log_profile_elapsed(phase, sub, ms as c_long, us as c_long, items);

    #[cfg(test)]
    {
        let _ = (phase, sub, ms, us, items);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn env_gate_matches_c_profile_policy() {
        assert!(!profile_env_enabled_bytes(None));
        assert!(!profile_env_enabled_bytes(Some(b"")));
        assert!(!profile_env_enabled_bytes(Some(b"0")));
        assert!(!profile_env_enabled_bytes(Some(b"00")));
        assert!(profile_env_enabled_bytes(Some(b"1")));
        assert!(profile_env_enabled_bytes(Some(b"false")));
    }

    #[test]
    fn elapsed_math_matches_c_units() {
        let start = CbmTimespec {
            tv_sec: 10,
            tv_nsec: 900_000_000,
        };
        let now = CbmTimespec {
            tv_sec: 12,
            tv_nsec: 100_250_000,
        };
        assert_eq!(elapsed_us(&start, &now), 1_200_250);
    }
}
