#![allow(unsafe_code)]

//! `foundation/diagnostics.c` 的完整 Rust 實作。
//!
//! 只有 mimalloc 的 process-info 查詢保留在極薄的 C bridge；統計、格式化、
//! 週期 writer、trajectory rotation 與生命週期均由 Rust 管理。

#[cfg(not(feature = "diagnostics-format-only"))]
use std::ffi::CStr;
use std::fs::{self, OpenOptions};
use std::io::Write;
#[cfg(not(feature = "diagnostics-format-only"))]
use std::os::raw::c_char;
use std::os::raw::{c_int, c_long, c_longlong};
#[cfg(not(feature = "diagnostics-format-only"))]
use std::ptr;
use std::sync::atomic::{AtomicBool, AtomicI32, AtomicI64, Ordering};
use std::sync::{Mutex, OnceLock};
use std::thread::{self, JoinHandle};
use std::time::{Duration, SystemTime, UNIX_EPOCH};

const DIAG_INTERVAL_SECONDS: u64 = 5;
const DIAG_NDJSON_CAP_BYTES: u64 = 8 * 1024 * 1024;

#[repr(C)]
pub struct CbmQueryStats {
    pub count: AtomicI32,
    pub errors: AtomicI32,
    pub time_us: AtomicI64,
    pub max_us: AtomicI64,
}

#[no_mangle]
pub static g_query_stats: CbmQueryStats = CbmQueryStats {
    count: AtomicI32::new(0),
    errors: AtomicI32::new(0),
    time_us: AtomicI64::new(0),
    max_us: AtomicI64::new(0),
};

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct CbmDiagQuerySnapshot {
    pub count: c_int,
    pub errors: c_int,
    pub total_us: c_longlong,
    pub avg_us: c_longlong,
    pub max_us: c_longlong,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct CbmDiagSnapshot {
    pub uptime_s: c_long,
    pub rss_bytes: usize,
    pub peak_rss_bytes: usize,
    pub heap_committed_bytes: usize,
    pub peak_committed_bytes: usize,
    pub page_faults: usize,
    pub fd_count: c_int,
    pub queries: CbmDiagQuerySnapshot,
    pub pid: c_int,
}

unsafe extern "C" {
    fn cbm_rs_diag_process_info(
        elapsed_ms: *mut usize,
        user_ms: *mut usize,
        sys_ms: *mut usize,
        current_rss: *mut usize,
        peak_rss: *mut usize,
        current_commit: *mut usize,
        peak_commit: *mut usize,
        page_faults: *mut usize,
    );
    fn cbm_mem_rss() -> usize;
}

#[derive(Default)]
struct DiagState {
    path: String,
    ndjson_path: String,
    ndjson_size: u64,
}

static DIAG_STOP: AtomicBool = AtomicBool::new(false);
static DIAG_START_SECONDS: AtomicI64 = AtomicI64::new(0);
static DIAG_STATE: OnceLock<Mutex<DiagState>> = OnceLock::new();
static DIAG_THREAD: Mutex<Option<JoinHandle<()>>> = Mutex::new(None);

fn diag_state() -> &'static Mutex<DiagState> {
    DIAG_STATE.get_or_init(|| Mutex::new(DiagState::default()))
}

fn lock_state() -> std::sync::MutexGuard<'static, DiagState> {
    diag_state()
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner())
}

fn lock_thread() -> std::sync::MutexGuard<'static, Option<JoinHandle<()>>> {
    DIAG_THREAD
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner())
}

fn now_seconds() -> i64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|duration| duration.as_secs() as i64)
        .unwrap_or(0)
}

#[cfg(not(feature = "diagnostics-format-only"))]
unsafe fn c_string(value: *const c_char) -> Option<String> {
    if value.is_null() {
        return None;
    }
    Some(
        unsafe { CStr::from_ptr(value) }
            .to_string_lossy()
            .into_owned(),
    )
}

#[cfg(not(feature = "diagnostics-format-only"))]
unsafe fn write_result(buf: *mut c_char, buf_sz: usize, value: &str) -> c_int {
    if buf.is_null() || buf_sz == 0 {
        return -1;
    }
    let bytes = value.as_bytes();
    let copy_len = bytes.len().min(buf_sz - 1);
    unsafe {
        ptr::copy_nonoverlapping(bytes.as_ptr().cast::<c_char>(), buf, copy_len);
        *buf.add(copy_len) = 0;
    }
    if bytes.len() >= buf_sz {
        -1
    } else {
        copy_len as c_int
    }
}

fn snapshot_from_values(
    count: c_int,
    errors: c_int,
    total_us: c_longlong,
    max_us: c_longlong,
) -> CbmDiagQuerySnapshot {
    CbmDiagQuerySnapshot {
        count,
        errors,
        total_us,
        avg_us: if count > 0 {
            total_us / count as c_longlong
        } else {
            0
        },
        max_us,
    }
}

fn format_json_string(snapshot: &CbmDiagSnapshot) -> String {
    let q = snapshot.queries;
    format!(
        "{{\n  \"uptime_s\": {},\n  \"rss_bytes\": {},\n  \"peak_rss_bytes\": {},\n  \"heap_committed_bytes\": {},\n  \"peak_committed_bytes\": {},\n  \"page_faults\": {},\n  \"fd_count\": {},\n  \"query_count\": {},\n  \"query_errors\": {},\n  \"query_total_us\": {},\n  \"query_avg_us\": {},\n  \"query_max_us\": {},\n  \"pid\": {}\n}}\n",
        snapshot.uptime_s,
        snapshot.rss_bytes,
        snapshot.peak_rss_bytes,
        snapshot.heap_committed_bytes,
        snapshot.peak_committed_bytes,
        snapshot.page_faults,
        snapshot.fd_count,
        q.count,
        q.errors,
        q.total_us,
        q.avg_us,
        q.max_us,
        snapshot.pid
    )
}

fn format_ndjson_string(snapshot: &CbmDiagSnapshot) -> String {
    format!(
        "{{\"uptime_s\":{},\"rss\":{},\"peak_rss\":{},\"committed\":{},\"peak_committed\":{},\"page_faults\":{},\"fd\":{},\"queries\":{}}}\n",
        snapshot.uptime_s,
        snapshot.rss_bytes,
        snapshot.peak_rss_bytes,
        snapshot.heap_committed_bytes,
        snapshot.peak_committed_bytes,
        snapshot.page_faults,
        snapshot.fd_count,
        snapshot.queries.count
    )
}

fn temp_directory() -> String {
    #[cfg(windows)]
    {
        std::env::var("TEMP")
            .or_else(|_| std::env::var("TMP"))
            .unwrap_or_else(|_| ".".to_string())
    }
    #[cfg(not(windows))]
    {
        "/tmp".to_string()
    }
}

fn count_open_fds() -> c_int {
    #[cfg(target_os = "linux")]
    let path = "/proc/self/fd";
    #[cfg(target_os = "macos")]
    let path = "/dev/fd";
    #[cfg(not(any(target_os = "linux", target_os = "macos")))]
    return -1;

    #[cfg(any(target_os = "linux", target_os = "macos"))]
    match fs::read_dir(path) {
        Ok(entries) => entries.filter_map(Result::ok).count() as c_int,
        Err(_) => -1,
    }
}

fn diagnostics_enabled_from_environment() -> bool {
    matches!(
        std::env::var("CBM_DIAGNOSTICS").as_deref(),
        Ok("1") | Ok("true")
    )
}

fn build_snapshot() -> CbmDiagSnapshot {
    let mut elapsed_ms = 0;
    let mut user_ms = 0;
    let mut sys_ms = 0;
    let mut current_rss = 0;
    let mut peak_rss = 0;
    let mut current_commit = 0;
    let mut peak_commit = 0;
    let mut page_faults = 0;
    unsafe {
        cbm_rs_diag_process_info(
            &mut elapsed_ms,
            &mut user_ms,
            &mut sys_ms,
            &mut current_rss,
            &mut peak_rss,
            &mut current_commit,
            &mut peak_commit,
            &mut page_faults,
        );
    }
    if current_rss == 0 {
        current_rss = unsafe { cbm_mem_rss() };
    }

    let start = DIAG_START_SECONDS.load(Ordering::Relaxed);
    let uptime = now_seconds().saturating_sub(start) as c_long;
    let mut queries = CbmDiagQuerySnapshot::default();
    cbm_diag_query_stats_snapshot(&g_query_stats, &mut queries);
    CbmDiagSnapshot {
        uptime_s: uptime,
        rss_bytes: current_rss,
        peak_rss_bytes: peak_rss,
        heap_committed_bytes: current_commit,
        peak_committed_bytes: peak_commit,
        page_faults,
        fd_count: count_open_fds(),
        queries,
        pid: std::process::id() as c_int,
    }
}

fn append_trajectory(snapshot: &CbmDiagSnapshot) {
    let line = format_ndjson_string(snapshot);
    let mut state = lock_state();
    if state.ndjson_path.is_empty() {
        return;
    }
    if state.ndjson_size > DIAG_NDJSON_CAP_BYTES {
        let rotated = format!("{}.1", state.ndjson_path);
        let _ = fs::rename(&state.ndjson_path, rotated);
        state.ndjson_size = 0;
    }
    let path = state.ndjson_path.clone();
    if let Ok(mut file) = OpenOptions::new().create(true).append(true).open(path) {
        if file.write_all(line.as_bytes()).is_ok() {
            state.ndjson_size = state.ndjson_size.saturating_add(line.len() as u64);
        }
    }
}

fn write_diagnostics() {
    let path = {
        let state = lock_state();
        state.path.clone()
    };
    if path.is_empty() {
        return;
    }

    let snapshot = build_snapshot();
    let json = format_json_string(&snapshot);
    let temporary = format!("{}.tmp", path);
    let mut wrote = false;
    if let Ok(mut file) = OpenOptions::new()
        .create(true)
        .write(true)
        .truncate(true)
        .open(&temporary)
    {
        if file.write_all(json.as_bytes()).is_ok() && file.flush().is_ok() {
            drop(file);
            let _ = fs::rename(&temporary, &path);
            wrote = true;
        }
    }
    if wrote {
        append_trajectory(&snapshot);
    }
}

fn diagnostics_thread() {
    while !DIAG_STOP.load(Ordering::Relaxed) {
        write_diagnostics();
        thread::sleep(Duration::from_secs(DIAG_INTERVAL_SECONDS));
    }
    write_diagnostics();
}

#[no_mangle]
#[cfg(not(feature = "diagnostics-format-only"))]
pub extern "C" fn cbm_diag_env_enabled_value(value: *const c_char) -> bool {
    let value = unsafe { c_string(value) };
    matches!(value.as_deref(), Some("1") | Some("true"))
}

#[no_mangle]
pub extern "C" fn cbm_diag_query_stats_snapshot(
    stats: *const CbmQueryStats,
    out: *mut CbmDiagQuerySnapshot,
) {
    if out.is_null() {
        return;
    }
    unsafe {
        *out = CbmDiagQuerySnapshot::default();
    }
    if stats.is_null() {
        return;
    }
    let values = unsafe {
        let stats = &*stats;
        (
            stats.count.load(Ordering::Relaxed),
            stats.errors.load(Ordering::Relaxed),
            stats.time_us.load(Ordering::Relaxed),
            stats.max_us.load(Ordering::Relaxed),
        )
    };
    unsafe {
        *out = snapshot_from_values(values.0, values.1, values.2, values.3);
    }
}

#[no_mangle]
pub extern "C" fn cbm_diag_record_query(duration_us: c_longlong, is_error: bool) {
    g_query_stats.count.fetch_add(1, Ordering::Relaxed);
    g_query_stats
        .time_us
        .fetch_add(duration_us, Ordering::Relaxed);
    if is_error {
        g_query_stats.errors.fetch_add(1, Ordering::Relaxed);
    }
    let mut old_max = g_query_stats.max_us.load(Ordering::Relaxed);
    while duration_us > old_max {
        match g_query_stats.max_us.compare_exchange_weak(
            old_max,
            duration_us,
            Ordering::Relaxed,
            Ordering::Relaxed,
        ) {
            Ok(_) => break,
            Err(observed) => old_max = observed,
        }
    }
}

#[no_mangle]
#[cfg(not(feature = "diagnostics-format-only"))]
pub extern "C" fn cbm_diag_format_path(
    buf: *mut c_char,
    buf_sz: usize,
    tmpdir: *const c_char,
    pid: c_int,
    ext: *const c_char,
) -> c_int {
    let Some(tmpdir) = (unsafe { c_string(tmpdir) }) else {
        return -1;
    };
    let Some(ext) = (unsafe { c_string(ext) }) else {
        return -1;
    };
    let path = format!("{tmpdir}/cbm-diagnostics-{pid}.{ext}");
    unsafe { write_result(buf, buf_sz, &path) }
}

#[no_mangle]
#[cfg(not(feature = "diagnostics-format-only"))]
pub extern "C" fn cbm_diag_format_json(
    buf: *mut c_char,
    buf_sz: usize,
    snapshot: *const CbmDiagSnapshot,
) -> c_int {
    if snapshot.is_null() {
        return -1;
    }
    let value = format_json_string(unsafe { &*snapshot });
    unsafe { write_result(buf, buf_sz, &value) }
}

#[no_mangle]
#[cfg(not(feature = "diagnostics-format-only"))]
pub extern "C" fn cbm_diag_format_ndjson(
    buf: *mut c_char,
    buf_sz: usize,
    snapshot: *const CbmDiagSnapshot,
) -> c_int {
    if snapshot.is_null() {
        return -1;
    }
    let value = format_ndjson_string(unsafe { &*snapshot });
    unsafe { write_result(buf, buf_sz, &value) }
}

#[no_mangle]
pub extern "C" fn cbm_diag_start() -> bool {
    if !diagnostics_enabled_from_environment() {
        return false;
    }
    let mut thread_slot = lock_thread();
    if thread_slot.is_some() {
        return false;
    }

    let pid = std::process::id() as c_int;
    let tmpdir = temp_directory();
    let path = format!("{tmpdir}/cbm-diagnostics-{pid}.json");
    let ndjson_path = format!("{tmpdir}/cbm-diagnostics-{pid}.ndjson");
    {
        let mut state = lock_state();
        state.path = path.clone();
        state.ndjson_path = ndjson_path.clone();
        state.ndjson_size = 0;
    }
    DIAG_START_SECONDS.store(now_seconds(), Ordering::Relaxed);
    DIAG_STOP.store(false, Ordering::Relaxed);

    let Ok(handle) = thread::Builder::new()
        .name("cbm-diagnostics".to_string())
        .spawn(diagnostics_thread)
    else {
        let mut state = lock_state();
        state.path.clear();
        state.ndjson_path.clear();
        return false;
    };
    *thread_slot = Some(handle);
    eprintln!(
        "level=info msg=diagnostics.start snapshot={} trajectory={} interval={}s",
        path, ndjson_path, DIAG_INTERVAL_SECONDS
    );
    true
}

#[no_mangle]
pub extern "C" fn cbm_diag_stop() {
    let handle = {
        let mut thread_slot = lock_thread();
        if thread_slot.is_none() {
            return;
        }
        DIAG_STOP.store(true, Ordering::Relaxed);
        thread_slot.take()
    };
    if let Some(handle) = handle {
        let _ = handle.join();
    }

    let (path, ndjson_path) = {
        let mut state = lock_state();
        let paths = (state.path.clone(), state.ndjson_path.clone());
        state.path.clear();
        state.ndjson_path.clear();
        state.ndjson_size = 0;
        paths
    };
    let _ = fs::remove_file(&path);
    let _ = fs::remove_file(format!("{path}.tmp"));
    if !ndjson_path.is_empty() {
        eprintln!(
            "level=info msg=diagnostics.trajectory_kept path={}",
            ndjson_path
        );
    }
}

#[cfg(test)]
mod tests {
    use super::{format_json_string, format_ndjson_string, snapshot_from_values, CbmDiagSnapshot};

    #[test]
    fn query_average_uses_integer_division_like_c() {
        let snapshot = snapshot_from_values(3, 1, 425, 250);
        assert_eq!(snapshot.avg_us, 141);
    }

    #[test]
    fn formatters_keep_c_field_order() {
        let snapshot = CbmDiagSnapshot {
            uptime_s: 17,
            rss_bytes: 1024,
            peak_rss_bytes: 2048,
            heap_committed_bytes: 4096,
            peak_committed_bytes: 8192,
            page_faults: 23,
            fd_count: 9,
            queries: snapshot_from_values(3, 1, 425, 250),
            pid: 4242,
        };
        assert!(format_json_string(&snapshot).starts_with("{\n  \"uptime_s\": 17,"));
        assert_eq!(
            format_ndjson_string(&snapshot),
            "{\"uptime_s\":17,\"rss\":1024,\"peak_rss\":2048,\"committed\":4096,\"peak_committed\":8192,\"page_faults\":23,\"fd\":9,\"queries\":3}\n"
        );
    }
}
