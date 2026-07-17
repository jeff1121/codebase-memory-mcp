#![allow(unsafe_code)]

//! `diagnostics.c` deterministic formatter 的 direct FFI 實作。
//!
//! writer thread、atomic query counter、mimalloc 統計與檔案 I/O 仍由 C 維護；
//! 這個模組只接管環境值判斷、路徑組裝與 JSON/NDJSON 格式化。

use crate::foundation::diagnostics::{self, QuerySnapshot, Snapshot};
use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_long};
use std::ptr;

#[repr(C)]
pub struct CbmDiagQuerySnapshot {
    count: c_int,
    errors: c_int,
    total_us: i64,
    avg_us: i64,
    max_us: i64,
}

#[repr(C)]
pub struct CbmDiagSnapshot {
    uptime_s: c_long,
    rss_bytes: usize,
    peak_rss_bytes: usize,
    heap_committed_bytes: usize,
    peak_committed_bytes: usize,
    page_faults: usize,
    fd_count: c_int,
    queries: CbmDiagQuerySnapshot,
    pid: c_int,
}

unsafe fn c_bytes<'a>(value: *const c_char) -> Option<&'a [u8]> {
    if value.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(value) }.to_bytes())
    }
}

fn write_result(buf: *mut c_char, buf_sz: usize, bytes: &[u8]) -> c_int {
    if buf.is_null() || buf_sz == 0 || bytes.len() >= buf_sz || bytes.len() > i32::MAX as usize {
        return -1;
    }
    unsafe {
        ptr::copy_nonoverlapping(bytes.as_ptr().cast::<c_char>(), buf, bytes.len());
        *buf.add(bytes.len()) = 0;
    }
    bytes.len() as c_int
}

#[cfg(unix)]
fn c_long_to_i64(value: c_long) -> i64 {
    value
}

#[cfg(windows)]
fn c_long_to_i64(value: c_long) -> i64 {
    i64::from(value)
}

#[cfg(not(any(unix, windows)))]
fn c_long_to_i64(value: c_long) -> i64 {
    value as i64
}

fn to_snapshot(value: &CbmDiagSnapshot) -> Snapshot {
    Snapshot {
        uptime_s: c_long_to_i64(value.uptime_s),
        rss_bytes: value.rss_bytes,
        peak_rss_bytes: value.peak_rss_bytes,
        heap_committed_bytes: value.heap_committed_bytes,
        peak_committed_bytes: value.peak_committed_bytes,
        page_faults: value.page_faults,
        fd_count: value.fd_count,
        queries: QuerySnapshot {
            count: value.queries.count,
            errors: value.queries.errors,
            total_us: value.queries.total_us,
            avg_us: value.queries.avg_us,
            max_us: value.queries.max_us,
        },
        pid: value.pid,
    }
}

/// 判斷單一 `CBM_DIAGNOSTICS` 值是否啟用。
///
/// # Safety
/// `value` 必須是可讀取且以 NUL 結尾的 C 字串，或為 NULL。
#[no_mangle]
pub unsafe extern "C" fn cbm_diag_env_enabled_value(value: *const c_char) -> c_int {
    diagnostics::env_enabled_value(unsafe { c_bytes(value) }) as c_int
}

/// 組出 diagnostics 檔案路徑。
///
/// # Safety
/// `buf` 必須指向至少 `buf_sz` bytes 的可寫區域；字串指標必須是可讀取且以 NUL
/// 結尾的 C 字串，或為 NULL。
#[no_mangle]
pub unsafe extern "C" fn cbm_diag_format_path(
    buf: *mut c_char,
    buf_sz: usize,
    tmpdir: *const c_char,
    pid: c_int,
    ext: *const c_char,
) -> c_int {
    let Some(tmpdir) = (unsafe { c_bytes(tmpdir) }) else {
        return -1;
    };
    let Some(ext) = (unsafe { c_bytes(ext) }) else {
        return -1;
    };
    let Some(path) = diagnostics::format_path(Some(tmpdir), pid, Some(ext)) else {
        return -1;
    };
    write_result(buf, buf_sz, &path)
}

/// 將 diagnostics snapshot 格式化為 JSON。
///
/// # Safety
/// `buf` 必須指向至少 `buf_sz` bytes 的可寫區域；`snapshot` 必須指向符合 C ABI
/// 的 `cbm_diag_snapshot_t`。
#[no_mangle]
pub unsafe extern "C" fn cbm_diag_format_json(
    buf: *mut c_char,
    buf_sz: usize,
    snapshot: *const CbmDiagSnapshot,
) -> c_int {
    if snapshot.is_null() {
        return -1;
    }
    let bytes = diagnostics::format_json(&to_snapshot(unsafe { &*snapshot }));
    write_result(buf, buf_sz, &bytes)
}

/// 將 diagnostics snapshot 格式化為單行 NDJSON。
///
/// # Safety
/// `buf` 必須指向至少 `buf_sz` bytes 的可寫區域；`snapshot` 必須指向符合 C ABI
/// 的 `cbm_diag_snapshot_t`。
#[no_mangle]
pub unsafe extern "C" fn cbm_diag_format_ndjson(
    buf: *mut c_char,
    buf_sz: usize,
    snapshot: *const CbmDiagSnapshot,
) -> c_int {
    if snapshot.is_null() {
        return -1;
    }
    let bytes = diagnostics::format_ndjson(&to_snapshot(unsafe { &*snapshot }));
    write_result(buf, buf_sz, &bytes)
}
