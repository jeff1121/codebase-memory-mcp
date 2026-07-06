//! 對齊 `src/foundation/mem.c` 的 test-only budget/pressure 合約。
//!
//! 只提供 pure value helper，不接產品 opt-in；C 端預設仍使用原本
//! mimalloc-backed `mem.c` 實作。

#[cfg(any(target_os = "linux", target_os = "android"))]
use std::fs::File;
#[cfg(any(target_os = "linux", target_os = "android"))]
use std::io::Read;
#[cfg(any(target_os = "linux", target_os = "android"))]
use std::path::Path;
use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};

use crate::foundation::platform;

const MAX_RAM_FRACTION: f64 = 1.0;
const DEFAULT_RAM_FRACTION: f64 = 0.5;
const MB_DIVISOR: usize = 1024 * 1024;
const FALLBACK_RAM_BYTES: usize = 4 * MB_DIVISOR * 1024;

static BUDGET: AtomicUsize = AtomicUsize::new(0);
static INITIALIZED: AtomicBool = AtomicBool::new(false);
static WAS_OVER: AtomicBool = AtomicBool::new(false);

pub fn init(ram_fraction: f64) {
    if INITIALIZED
        .compare_exchange(false, true, Ordering::AcqRel, Ordering::Acquire)
        .is_err()
    {
        return;
    }

    if let Some(override_mb) = env_budget_override() {
        BUDGET.store(override_mb.saturating_mul(MB_DIVISOR), Ordering::Release);
        return;
    }

    let mut fraction = ram_fraction;
    if !(0.0..=MAX_RAM_FRACTION).contains(&fraction) {
        fraction = DEFAULT_RAM_FRACTION;
    }

    let total_ram = total_ram_bytes().unwrap_or(FALLBACK_RAM_BYTES);
    let mut budget = 0usize;
    if total_ram > 0 {
        budget = (total_ram as f64 * fraction).floor() as usize;
    }
    BUDGET.store(budget, Ordering::Release);
}

pub fn rss() -> usize {
    os_rss().unwrap_or(0)
}

pub fn peak_rss() -> usize {
    os_peak_rss().unwrap_or_else(rss)
}

pub fn budget() -> usize {
    BUDGET.load(Ordering::Acquire)
}

pub fn over_budget() -> bool {
    let current = rss();
    check_pressure(current);
    current > budget()
}

pub fn worker_budget(num_workers: i32) -> usize {
    let denom = if num_workers <= 0 {
        1usize
    } else {
        num_workers as usize
    };
    let current_budget = budget();
    if denom == 0 {
        return current_budget;
    }
    current_budget / denom
}

pub fn collect() {
    // C 實作用 mimalloc 的 mi_collect(true) 收回 page，這裡保留 no-op safety point。
}

fn check_pressure(current_rss: usize) {
    let current_budget = budget();
    if current_budget == 0 {
        return;
    }
    let over = current_rss > current_budget;
    if over {
        WAS_OVER.store(true, Ordering::Release);
    } else {
        WAS_OVER.store(false, Ordering::Release);
    }
}

fn env_budget_override() -> Option<usize> {
    let raw = platform::safe_getenv_bytes(Some(b"CBM_MEM_BUDGET_MB"), None)?;
    if raw.is_empty() {
        return None;
    }
    let Ok(raw) = std::str::from_utf8(&raw) else {
        return None;
    };
    let value = raw.trim();
    if value.is_empty() {
        return None;
    }
    let Ok(parsed) = value.parse::<usize>() else {
        return None;
    };
    if parsed == 0 {
        return None;
    }
    Some(parsed)
}

fn total_ram_bytes() -> Option<usize> {
    #[allow(unused_mut)]
    let mut total = host_total_ram_bytes();
    #[cfg(any(target_os = "linux", target_os = "android"))]
    {
        let cgroup = platform::detect_cgroup_mem(Path::new("/sys/fs/cgroup"));
        if cgroup > 0 {
            total = total.and_then(|host| {
                if cgroup < host {
                    Some(cgroup)
                } else {
                    Some(host)
                }
            });
        }
    }
    total.filter(|value| *value > 0)
}

#[cfg(any(target_os = "linux", target_os = "android"))]
fn host_total_ram_bytes() -> Option<usize> {
    let mut file = File::open("/proc/meminfo").ok()?;
    let mut buf = String::new();
    file.read_to_string(&mut buf).ok()?;
    for line in buf.lines() {
        if let Some(rest) = line.strip_prefix("MemTotal:") {
            let parts = rest.split_whitespace().collect::<Vec<_>>();
            if parts.len() < 2 {
                return None;
            }
            let Ok(kb) = parts[0].parse::<u64>() else {
                return None;
            };
            return Some((kb as usize).saturating_mul(1024));
        }
    }
    None
}

#[cfg(not(any(target_os = "linux", target_os = "android")))]
fn host_total_ram_bytes() -> Option<usize> {
    None
}

#[cfg(any(target_os = "linux", target_os = "android"))]
fn os_rss() -> Option<usize> {
    parse_proc_status_bytes("VmRSS:")
}

#[cfg(any(target_os = "linux", target_os = "android"))]
fn os_peak_rss() -> Option<usize> {
    parse_proc_status_bytes("VmHWM:")
}

#[cfg(not(any(target_os = "linux", target_os = "android")))]
fn os_rss() -> Option<usize> {
    None
}

#[cfg(not(any(target_os = "linux", target_os = "android")))]
fn os_peak_rss() -> Option<usize> {
    None
}

#[cfg(any(target_os = "linux", target_os = "android"))]
fn parse_proc_status_bytes(name: &str) -> Option<usize> {
    let mut file = File::open("/proc/self/status").ok()?;
    let mut buf = String::new();
    file.read_to_string(&mut buf).ok()?;
    for line in buf.lines() {
        if !line.starts_with(name) {
            continue;
        }
        let tokens: Vec<_> = line.split_whitespace().collect();
        let Some(value) = tokens.get(1) else {
            return None;
        };
        let Ok(kb) = value.parse::<u64>() else {
            return None;
        };
        return Some((kb as usize).saturating_mul(1024));
    }
    None
}

#[cfg(test)]
pub fn reset_for_tests() {
    INITIALIZED.store(false, Ordering::Release);
    BUDGET.store(0, Ordering::Release);
    WAS_OVER.store(false, Ordering::Release);
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::{Mutex, OnceLock};

    fn mem_test_lock() -> std::sync::MutexGuard<'static, ()> {
        static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
        LOCK.get_or_init(|| Mutex::new(())).lock().unwrap()
    }

    #[test]
    fn init_only_writes_budget_once() {
        let _guard = mem_test_lock();
        reset_for_tests();
        init(0.5);
        let first = budget();
        assert!(first > 0);

        init(0.9);
        assert_eq!(budget(), first);
    }

    #[test]
    fn env_override_takes_precedence() {
        let _guard = mem_test_lock();
        reset_for_tests();
        std::env::set_var("CBM_MEM_BUDGET_MB", "2048");
        init(0.1);
        assert_eq!(budget(), 2048 * MB_DIVISOR);
        std::env::remove_var("CBM_MEM_BUDGET_MB");
    }

    #[test]
    fn worker_budget_divisions_match_contract() {
        let _guard = mem_test_lock();
        reset_for_tests();
        init(0.5);
        let b = budget();
        assert_eq!(worker_budget(4), b / 4);
        assert_eq!(worker_budget(0), b);
        assert_eq!(worker_budget(-3), b);
    }

    #[test]
    fn reporting_and_collect_are_safe() {
        let _guard = mem_test_lock();
        reset_for_tests();
        init(0.5);
        collect();
        assert_eq!(
            WAS_OVER.load(Ordering::Acquire),
            over_budget() && budget() != 0
        );
    }
}
