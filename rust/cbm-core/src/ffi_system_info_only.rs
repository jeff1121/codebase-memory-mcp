#![allow(unsafe_code)]

//! `system_info.c` 的 Rust direct-only ABI 實作。
//!
//! 這裡只處理平台資訊、cgroup 限制與 worker policy；公開資料結構仍維持
//! `platform.h` 的 C layout，讓既有 C 呼叫端不需要改動。

use core::ffi::{c_char, c_int, c_void};
use std::ffi::CStr;
#[cfg(not(test))]
use std::ffi::CString;
use std::fs;
use std::mem::size_of;
use std::path::{Path, PathBuf};
use std::ptr;
use std::sync::OnceLock;
use std::thread;

const DEFAULT_CORES: c_int = 1;
const MIN_WORKERS: c_int = 1;
const CBM_WORKERS_MAX: c_int = 256;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CbmSystemInfo {
    pub total_cores: c_int,
    pub perf_cores: c_int,
    pub total_ram: usize,
}

static INFO: OnceLock<CbmSystemInfo> = OnceLock::new();

#[cfg(not(test))]
unsafe extern "C" {
    fn cbm_log_workers_env_invalid(value: *const c_char);
}

fn log_invalid_worker_env(_value: &str) {
    #[cfg(not(test))]
    if let Ok(value) = CString::new(_value) {
        unsafe { cbm_log_workers_env_invalid(value.as_ptr()) };
    }
}

#[allow(dead_code)]
fn available_cores() -> c_int {
    thread::available_parallelism()
        .map(|count| count.get().min(c_int::MAX as usize) as c_int)
        .unwrap_or(DEFAULT_CORES)
}

fn read_trimmed(path: &Path) -> Option<String> {
    let bytes = fs::read(path).ok()?;
    let end = bytes
        .iter()
        .rposition(|byte| !matches!(byte, b'\n' | b' ' | b'\t'))
        .map_or(0, |index| index + 1);
    String::from_utf8(bytes[..end].to_vec()).ok()
}

fn detect_cgroup_cpus(root: &Path) -> c_int {
    if let Some(value) = read_trimmed(&root.join("cpu.max")) {
        if value.starts_with("max") {
            return -1;
        }
        let mut parts = value.split_whitespace();
        let quota = parts.next().and_then(|part| part.parse::<i64>().ok());
        let period = parts.next().and_then(|part| part.parse::<i64>().ok());
        if let (Some(quota), Some(period)) = (quota, period) {
            if quota > 0 && period > 0 {
                let count = quota.saturating_add(period - 1) / period;
                return count.min(c_int::MAX as i64) as c_int;
            }
        }
        return -1;
    }

    let quota = read_trimmed(&root.join("cpu/cpu.cfs_quota_us"))
        .and_then(|value| value.parse::<i64>().ok());
    let period = read_trimmed(&root.join("cpu/cpu.cfs_period_us"))
        .and_then(|value| value.parse::<i64>().ok());
    match (quota, period) {
        (Some(quota), Some(period)) if quota > 0 && period > 0 => {
            let count = quota.saturating_add(period - 1) / period;
            count.min(c_int::MAX as i64) as c_int
        }
        _ => -1,
    }
}

fn detect_cgroup_mem(root: &Path) -> usize {
    if let Some(value) = read_trimmed(&root.join("memory.max")) {
        if value.starts_with("max") {
            return 0;
        }
        return value
            .parse::<u64>()
            .ok()
            .filter(|size| *size > 0)
            .and_then(|size| usize::try_from(size).ok())
            .unwrap_or(0);
    }

    read_trimmed(&root.join("memory/memory.limit_in_bytes"))
        .and_then(|value| value.parse::<u64>().ok())
        .filter(|size| *size > 0 && *size < u64::MAX / 2)
        .and_then(|size| usize::try_from(size).ok())
        .unwrap_or(0)
}

#[cfg(target_os = "linux")]
#[allow(dead_code)]
fn host_ram() -> usize {
    let Some(contents) = read_trimmed(Path::new("/proc/meminfo")) else {
        return 0;
    };
    for line in contents.lines() {
        let mut fields = line.split_whitespace();
        if fields.next() == Some("MemTotal:") {
            let Some(kib) = fields.next().and_then(|value| value.parse::<u64>().ok()) else {
                return 0;
            };
            return kib.saturating_mul(1024).try_into().unwrap_or(usize::MAX);
        }
    }
    0
}

#[cfg(not(target_os = "linux"))]
#[allow(dead_code)]
fn host_ram() -> usize {
    0
}

#[cfg(any(
    target_os = "macos",
    target_os = "freebsd",
    target_os = "netbsd",
    target_os = "openbsd"
))]
unsafe extern "C" {
    fn sysctlbyname(
        name: *const c_char,
        oldp: *mut c_void,
        oldlenp: *mut usize,
        newp: *mut c_void,
        newlen: usize,
    ) -> c_int;
}

#[cfg(any(
    target_os = "macos",
    target_os = "freebsd",
    target_os = "netbsd",
    target_os = "openbsd"
))]
fn sysctl_u64(name: &[u8]) -> Option<u64> {
    let mut value = 0_u64;
    let mut length = size_of::<u64>();
    let rc = unsafe {
        sysctlbyname(
            name.as_ptr() as *const c_char,
            (&mut value as *mut u64).cast::<c_void>(),
            &mut length,
            ptr::null_mut(),
            0,
        )
    };
    (rc == 0 && value > 0).then_some(value)
}

#[cfg(target_os = "macos")]
fn detect_system() -> CbmSystemInfo {
    let total_cores = sysctl_u64(b"hw.ncpu\0")
        .and_then(|value| c_int::try_from(value).ok())
        .filter(|value| *value > 0)
        .unwrap_or(DEFAULT_CORES);
    let mut perf_cores = sysctl_u64(b"hw.perflevel0.physicalcpu\0")
        .and_then(|value| c_int::try_from(value).ok())
        .filter(|value| *value > 0)
        .unwrap_or(total_cores);
    let efficiency_cores = sysctl_u64(b"hw.perflevel1.physicalcpu\0")
        .and_then(|value| c_int::try_from(value).ok())
        .unwrap_or(0);
    if perf_cores.saturating_add(efficiency_cores) > total_cores {
        perf_cores = total_cores;
    }
    let total_ram = sysctl_u64(b"hw.memsize\0")
        .and_then(|value| usize::try_from(value).ok())
        .unwrap_or(0);
    CbmSystemInfo {
        total_cores,
        perf_cores,
        total_ram,
    }
}

#[cfg(any(target_os = "freebsd", target_os = "netbsd", target_os = "openbsd"))]
fn detect_system() -> CbmSystemInfo {
    let total_cores = available_cores();
    let total_ram = sysctl_u64(b"hw.physmem64\0")
        .or_else(|| sysctl_u64(b"hw.physmem\0"))
        .and_then(|value| usize::try_from(value).ok())
        .unwrap_or(0);
    CbmSystemInfo {
        total_cores,
        perf_cores: total_cores,
        total_ram,
    }
}

#[cfg(target_os = "linux")]
fn detect_system() -> CbmSystemInfo {
    let host_cores = available_cores();
    let cgroup_cores = detect_cgroup_cpus(Path::new("/sys/fs/cgroup"));
    let total_cores = if cgroup_cores > 0 && cgroup_cores < host_cores {
        cgroup_cores
    } else {
        host_cores
    };
    let host_memory = host_ram();
    let cgroup_memory = detect_cgroup_mem(Path::new("/sys/fs/cgroup"));
    let total_ram = if cgroup_memory > 0 && (host_memory == 0 || cgroup_memory < host_memory) {
        cgroup_memory
    } else {
        host_memory
    };
    CbmSystemInfo {
        total_cores,
        perf_cores: total_cores,
        total_ram,
    }
}

#[cfg(target_os = "windows")]
#[repr(C)]
struct SystemInfo {
    processor_architecture: u16,
    reserved: u16,
    page_size: u32,
    minimum_application_address: *mut c_void,
    maximum_application_address: *mut c_void,
    active_processor_mask: usize,
    number_of_processors: u32,
    processor_type: u32,
    allocation_granularity: u32,
    processor_level: u16,
    processor_revision: u16,
}

#[cfg(target_os = "windows")]
#[repr(C)]
struct MemoryStatusEx {
    length: u32,
    memory_load: u32,
    total_phys: u64,
    avail_phys: u64,
    total_page_file: u64,
    avail_page_file: u64,
    total_virtual: u64,
    avail_virtual: u64,
    avail_extended_virtual: u64,
}

#[cfg(target_os = "windows")]
unsafe extern "system" {
    fn GetSystemInfo(info: *mut SystemInfo);
    fn GlobalMemoryStatusEx(status: *mut MemoryStatusEx) -> i32;
}

#[cfg(target_os = "windows")]
fn detect_system() -> CbmSystemInfo {
    let mut system: SystemInfo = unsafe { std::mem::zeroed() };
    unsafe { GetSystemInfo(&mut system) };
    let total_cores = c_int::try_from(system.number_of_processors)
        .ok()
        .filter(|value| *value > 0)
        .unwrap_or(DEFAULT_CORES);
    let mut memory: MemoryStatusEx = unsafe { std::mem::zeroed() };
    memory.length = size_of::<MemoryStatusEx>() as u32;
    let total_ram = if unsafe { GlobalMemoryStatusEx(&mut memory) } != 0 {
        usize::try_from(memory.total_phys).unwrap_or(usize::MAX)
    } else {
        0
    };
    CbmSystemInfo {
        total_cores,
        perf_cores: total_cores,
        total_ram,
    }
}

#[cfg(not(any(
    target_os = "linux",
    target_os = "macos",
    target_os = "freebsd",
    target_os = "netbsd",
    target_os = "openbsd",
    target_os = "windows"
)))]
fn detect_system() -> CbmSystemInfo {
    let total_cores = available_cores();
    CbmSystemInfo {
        total_cores,
        perf_cores: total_cores,
        total_ram: host_ram(),
    }
}

unsafe fn c_path(path: *const c_char) -> Option<PathBuf> {
    if path.is_null() {
        return None;
    }
    let bytes = unsafe { CStr::from_ptr(path) }.to_bytes();
    Some(PathBuf::from(std::str::from_utf8(bytes).ok()?))
}

#[no_mangle]
pub extern "C" fn cbm_system_info() -> CbmSystemInfo {
    *INFO.get_or_init(detect_system)
}

#[no_mangle]
pub extern "C" fn cbm_default_worker_count(initial: bool) -> c_int {
    if let Some(value) = std::env::var_os("CBM_WORKERS") {
        let value = value.to_string_lossy();
        if let Ok(workers) = value.parse::<c_int>() {
            if (MIN_WORKERS..=CBM_WORKERS_MAX).contains(&workers) {
                return workers;
            }
        }
        log_invalid_worker_env(&value);
    }
    let info = cbm_system_info();
    if initial {
        info.total_cores
    } else {
        (info.perf_cores - 1).max(MIN_WORKERS)
    }
}

#[no_mangle]
/// 讀取指定 cgroup 根目錄的 CPU 配額。
///
/// # Safety
/// `cgroup_root` 必須是 null 或指向以 NUL 結尾的有效 C 字串。
pub unsafe extern "C" fn cbm_detect_cgroup_cpus(cgroup_root: *const c_char) -> c_int {
    let Some(root) = (unsafe { c_path(cgroup_root) }) else {
        return -1;
    };
    detect_cgroup_cpus(&root)
}

#[no_mangle]
/// 讀取指定 cgroup 根目錄的記憶體限制。
///
/// # Safety
/// `cgroup_root` 必須是 null 或指向以 NUL 結尾的有效 C 字串。
pub unsafe extern "C" fn cbm_detect_cgroup_mem(cgroup_root: *const c_char) -> usize {
    let Some(root) = (unsafe { c_path(cgroup_root) }) else {
        return 0;
    };
    detect_cgroup_mem(&root)
}
