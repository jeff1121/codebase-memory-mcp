#![allow(unsafe_code)]

//! Unified memory budget ABI 的 Rust direct 實作。

use std::os::raw::{c_char, c_int};
use std::ptr;
use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};

use crate::foundation::platform;

const DEFAULT_RAM_FRACTION: f64 = 0.5;
const MAX_RAM_FRACTION: f64 = 1.0;
const MB_DIVISOR: usize = 1024 * 1024;

#[repr(C)]
struct CbmSystemInfo {
    _total_cores: c_int,
    _perf_cores: c_int,
    total_ram: usize,
}

unsafe extern "C" {
    fn cbm_system_info() -> CbmSystemInfo;
    fn cbm_rs_mem_mimalloc_init();
    fn cbm_rs_mem_process_info(current: *mut usize, peak: *mut usize);
    fn cbm_rs_mem_os_rss() -> usize;
    fn cbm_rs_mem_bridge_collect();
    fn cbm_rs_mem_log_init(budget: usize, total_ram: usize, source: *const c_char);
    fn cbm_rs_mem_log_invalid_env(value: *const c_char);
    fn cbm_rs_mem_log_pressure(over: c_int, rss: usize, budget: usize);
}

static BUDGET: AtomicUsize = AtomicUsize::new(0);
static INITIALIZED: AtomicBool = AtomicBool::new(false);
static WAS_OVER: AtomicBool = AtomicBool::new(false);

fn parse_positive_long(value: &[u8]) -> Option<usize> {
    let mut index = 0;
    while value
        .get(index)
        .is_some_and(|byte| byte.is_ascii_whitespace())
    {
        index += 1;
    }
    if value.get(index) == Some(&b'+') {
        index += 1;
    } else if value.get(index) == Some(&b'-') {
        return None;
    }
    let start = index;
    let mut parsed = 0u128;
    while let Some(byte @ b'0'..=b'9') = value.get(index).copied() {
        parsed = parsed
            .saturating_mul(10)
            .saturating_add(u128::from(byte - b'0'));
        index += 1;
    }
    if index == start || parsed == 0 {
        return None;
    }
    Some(parsed.min(usize::MAX as u128) as usize)
}

fn env_budget_override() -> Option<(usize, *const c_char)> {
    let value = platform::safe_getenv_bytes(Some(b"CBM_MEM_BUDGET_MB"), None)?;
    let parsed = parse_positive_long(&value);
    if parsed.is_none() {
        // SAFETY: value is copied into an owned NUL-terminated buffer for the bridge.
        let mut nulled = value;
        nulled.push(0);
        let ptr = nulled.as_ptr().cast::<c_char>();
        // The invalid value is logged before the temporary buffer is dropped.
        unsafe { cbm_rs_mem_log_invalid_env(ptr) };
    }
    let parsed = parsed?;
    Some((parsed, c"CBM_MEM_BUDGET_MB".as_ptr().cast()))
}

fn check_pressure(rss: usize) {
    let budget = BUDGET.load(Ordering::Acquire);
    if budget == 0 {
        return;
    }
    let over = rss > budget;
    let was_over = WAS_OVER.swap(over, Ordering::AcqRel);
    if was_over != over {
        // SAFETY: bridge only observes scalar values during this call.
        unsafe { cbm_rs_mem_log_pressure(over as c_int, rss, budget) };
    }
}

/// 初始化記憶體 budget；只有第一次呼叫有效。
///
/// # Safety
/// 此函式沒有 raw-pointer 參數；其 process-global 狀態必須只透過本 ABI 存取。
#[no_mangle]
pub extern "C" fn cbm_mem_init(ram_fraction: f64) {
    if INITIALIZED
        .compare_exchange(false, true, Ordering::AcqRel, Ordering::Acquire)
        .is_err()
    {
        return;
    }

    // SAFETY: production bridge 只設定 mimalloc process options；smoke 由 no-op stub 提供。
    unsafe { cbm_rs_mem_mimalloc_init() };

    if let Some((override_mb, source)) = env_budget_override() {
        let budget = override_mb.saturating_mul(MB_DIVISOR);
        BUDGET.store(budget, Ordering::Release);
        // SAFETY: NULL source 代表預設 log 內容；bridge 不保存指標。
        unsafe { cbm_rs_mem_log_init(budget, 0, source) };
        return;
    }

    let fraction =
        if ram_fraction.is_finite() && ram_fraction > 0.0 && ram_fraction <= MAX_RAM_FRACTION {
            ram_fraction
        } else {
            DEFAULT_RAM_FRACTION
        };
    // SAFETY: cbm_system_info 回傳固定 repr(C) scalar 結構。
    let total_ram = unsafe { cbm_system_info() }.total_ram;
    let budget = (total_ram as f64 * fraction).floor() as usize;
    BUDGET.store(budget, Ordering::Release);
    // SAFETY: NULL source 代表 RAM fraction 設定；bridge 不保存指標。
    unsafe { cbm_rs_mem_log_init(budget, total_ram, ptr::null()) };
}

/// 取得目前 RSS；mimalloc 無法提供時使用 OS fallback。
#[no_mangle]
pub extern "C" fn cbm_mem_rss() -> usize {
    let mut current = 0;
    let mut peak = 0;
    // SAFETY: bridge 只寫入兩個 caller-owned size_t 輸出值。
    unsafe { cbm_rs_mem_process_info(&mut current, &mut peak) };
    if current > 0 {
        return current;
    }
    // SAFETY: bridge 回傳 process RSS scalar，不保存 Rust 指標。
    unsafe { cbm_rs_mem_os_rss() }
}

/// 取得 peak RSS；無 peak 資料時回傳目前 RSS 的 best effort。
#[no_mangle]
pub extern "C" fn cbm_mem_peak_rss() -> usize {
    let mut current = 0;
    let mut peak = 0;
    // SAFETY: bridge 只寫入兩個 caller-owned size_t 輸出值。
    unsafe { cbm_rs_mem_process_info(&mut current, &mut peak) };
    if peak > 0 {
        return peak;
    }
    // SAFETY: bridge 回傳 process RSS scalar，不保存 Rust 指標。
    unsafe { cbm_rs_mem_os_rss() }
}

/// 取得目前記憶體 budget。
#[no_mangle]
pub extern "C" fn cbm_mem_budget() -> usize {
    BUDGET.load(Ordering::Acquire)
}

/// 回報目前 RSS 是否超過 budget，並更新 pressure hysteresis。
#[no_mangle]
pub extern "C" fn cbm_mem_over_budget() -> bool {
    let rss = cbm_mem_rss();
    check_pressure(rss);
    rss > cbm_mem_budget()
}

/// 回傳每個 worker 的 advisory budget；非正 worker 數視為一個 worker。
#[no_mangle]
pub extern "C" fn cbm_mem_worker_budget(num_workers: c_int) -> usize {
    let workers = if num_workers <= 0 {
        1usize
    } else {
        num_workers as usize
    };
    cbm_mem_budget() / workers
}

/// 交由 mimalloc 回收未使用頁面。
#[no_mangle]
pub extern "C" fn cbm_mem_collect() {
    // SAFETY: production bridge 呼叫 mimalloc collect；smoke 由 no-op stub 提供。
    unsafe { cbm_rs_mem_bridge_collect() };
}

#[cfg(test)]
fn worker_budget_for_test(num_workers: c_int, budget: usize) -> usize {
    let workers = if num_workers <= 0 {
        1usize
    } else {
        num_workers as usize
    };
    budget / workers
}

#[cfg(test)]
mod tests {
    use super::{parse_positive_long, worker_budget_for_test};

    #[test]
    fn parses_strtol_like_positive_prefix() {
        assert_eq!(parse_positive_long(b"2048junk"), Some(2048));
        assert_eq!(parse_positive_long(b" +7"), Some(7));
        assert_eq!(parse_positive_long(b"-1"), None);
        assert_eq!(parse_positive_long(b"junk"), None);
    }

    #[test]
    fn worker_budget_uses_one_for_non_positive_values() {
        assert_eq!(worker_budget_for_test(0, 100), 100);
        assert_eq!(worker_budget_for_test(-3, 100), 100);
        assert_eq!(worker_budget_for_test(4, 100), 25);
    }
}
