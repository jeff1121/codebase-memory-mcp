//! 階段性 Rust migration 的 C ABI exports。
//!
//! 這些 symbols 由 `make -f Makefile.cbm rust-ffi-test` 驗證；只有明確 opt-in
//! 的 C wrapper 會呼叫它們。

#![allow(unsafe_code)]

use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_long, c_void};
use std::path::Path;
use std::ptr;
use std::slice;

use crate::foundation::diagnostics;
use crate::foundation::dump_verify;
use crate::foundation::hash_table::HashTable;
use crate::foundation::platform;
use crate::foundation::str_intern::CInternPool;
use crate::foundation::str_util;
use crate::pipeline::plan::{self, PlanKind};
use crate::pipeline::registry::{Entry as RegistryEntry, Registry};

#[repr(C)]
pub struct CbmRsRegistryEntry {
    pub name: *const c_char,
    pub qualified_name: *const c_char,
    pub label: *const c_char,
}

#[repr(C)]
pub struct CbmRsRegistryResolveOut {
    pub confidence: f64,
    pub candidate_count: c_int,
    pub matched: c_int,
}

#[repr(C)]
pub struct CbmRsDiagQuerySnapshot {
    pub count: c_int,
    pub errors: c_int,
    pub total_us: i64,
    pub avg_us: i64,
    pub max_us: i64,
}

#[repr(C)]
pub struct CbmRsDiagSnapshot {
    pub uptime_s: c_long,
    pub rss_bytes: usize,
    pub peak_rss_bytes: usize,
    pub heap_committed_bytes: usize,
    pub peak_committed_bytes: usize,
    pub page_faults: usize,
    pub fd_count: c_int,
    pub queries: CbmRsDiagQuerySnapshot,
    pub pid: c_int,
}

pub struct CbmRsRegistryHandle {
    registry: Registry,
}

#[no_mangle]
pub extern "C" fn cbm_rs_dump_verify_is_degraded(
    committed_nodes: c_int,
    persisted_nodes: c_int,
    ratio: f64,
    min_floor: c_int,
) -> c_int {
    dump_verify::is_degraded(committed_nodes, persisted_nodes, ratio, min_floor) as c_int
}

#[no_mangle]
pub extern "C" fn cbm_rs_intern_create() -> *mut CInternPool {
    Box::into_raw(Box::new(CInternPool::new()))
}

#[no_mangle]
/// # Safety
///
/// `pool` 必須是 null，或是 `cbm_rs_intern_create` 回傳且尚未 free 的指標。
pub unsafe extern "C" fn cbm_rs_intern_free(pool: *mut CInternPool) {
    if pool.is_null() {
        return;
    }
    unsafe {
        drop(Box::from_raw(pool));
    }
}

#[no_mangle]
/// # Safety
///
/// `pool` 必須是 null，或是 `cbm_rs_intern_create` 回傳且仍有效的指標。
/// `value` 必須是 null，或指向至少 `len` bytes 的可讀記憶體。
pub unsafe extern "C" fn cbm_rs_intern_n(
    pool: *mut CInternPool,
    value: *const u8,
    len: usize,
) -> *const u8 {
    if pool.is_null() || value.is_null() {
        return ptr::null();
    }
    if len > isize::MAX as usize || len > u32::MAX as usize {
        return ptr::null();
    }
    let bytes = unsafe { slice::from_raw_parts(value, len) };
    let pool = unsafe { &mut *pool };
    pool.intern_n(bytes).map_or(ptr::null(), |ptr| ptr.cast())
}

#[no_mangle]
/// # Safety
///
/// `pool` 必須是 null，或是 `cbm_rs_intern_create` 回傳且仍有效的指標。
pub unsafe extern "C" fn cbm_rs_intern_count(pool: *const CInternPool) -> u32 {
    if pool.is_null() {
        return 0;
    }
    unsafe { &*pool }.count()
}

#[no_mangle]
/// # Safety
///
/// `pool` 必須是 null，或是 `cbm_rs_intern_create` 回傳且仍有效的指標。
pub unsafe extern "C" fn cbm_rs_intern_bytes(pool: *const CInternPool) -> usize {
    if pool.is_null() {
        return 0;
    }
    unsafe { &*pool }.bytes()
}

unsafe fn c_bytes<'a>(value: *const c_char) -> Option<&'a [u8]> {
    if value.is_null() {
        return None;
    }
    Some(unsafe { CStr::from_ptr(value) }.to_bytes())
}

static EMPTY_CSTR: &[u8; 1] = b"\0";

unsafe fn write_c_output(buf: *mut c_char, bufsize: usize, output: Option<&[u8]>) -> usize {
    let Some(output) = output else {
        return usize::MAX;
    };

    if !buf.is_null() && bufsize > 0 {
        let copied = output.len().min(bufsize - 1);
        if copied > 0 {
            unsafe {
                ptr::copy_nonoverlapping(output.as_ptr().cast::<c_char>(), buf, copied);
            }
        }
        unsafe {
            *buf.add(copied) = 0;
        }
    }
    output.len()
}

unsafe fn write_c_output_i32(buf: *mut c_char, bufsize: usize, output: Option<&[u8]>) -> c_int {
    let Some(output) = output else {
        return -1;
    };
    if output.len() > c_int::MAX as usize {
        return -1;
    }
    if buf.is_null() || bufsize == 0 {
        return -1;
    }

    let copied = output.len().min(bufsize - 1);
    if copied > 0 {
        unsafe {
            ptr::copy_nonoverlapping(output.as_ptr().cast::<c_char>(), buf, copied);
        }
    }
    unsafe {
        *buf.add(copied) = 0;
    }
    if output.len() >= bufsize {
        return -1;
    }
    output.len() as c_int
}

unsafe fn c_string_parts<'a>(parts: *const *const c_char, n: c_int) -> Option<Vec<&'a [u8]>> {
    if n <= 0 || parts.is_null() {
        return Some(Vec::new());
    }
    let parts = unsafe { slice::from_raw_parts(parts, n as usize) };
    let mut out = Vec::with_capacity(parts.len());
    for part in parts {
        out.push(unsafe { c_bytes(*part) }?);
    }
    Some(out)
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null，或指向 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_diag_env_enabled_value(value: *const c_char) -> c_int {
    diagnostics::env_enabled_value(unsafe { c_bytes(value) }) as c_int
}

#[no_mangle]
/// # Safety
///
/// `out` 必須是 null，或指向可寫的 `CbmRsDiagQuerySnapshot`。
pub unsafe extern "C" fn cbm_rs_diag_query_snapshot_values(
    count: c_int,
    errors: c_int,
    total_us: i64,
    max_us: i64,
    out: *mut CbmRsDiagQuerySnapshot,
) -> c_int {
    if out.is_null() {
        return -1;
    }
    let snapshot = diagnostics::query_stats_snapshot_values(count, errors, total_us, max_us);
    unsafe {
        (*out).count = snapshot.count;
        (*out).errors = snapshot.errors;
        (*out).total_us = snapshot.total_us;
        (*out).avg_us = snapshot.avg_us;
        (*out).max_us = snapshot.max_us;
    }
    0
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。`tmpdir` 與
/// `ext` 必須是 null，或指向 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_diag_format_path(
    buf: *mut c_char,
    bufsize: usize,
    tmpdir: *const c_char,
    pid: c_int,
    ext: *const c_char,
) -> c_int {
    let output = diagnostics::format_path(unsafe { c_bytes(tmpdir) }, pid, unsafe { c_bytes(ext) });
    unsafe { write_c_output_i32(buf, bufsize, output.as_deref()) }
}

fn diag_snapshot_from_ffi(snapshot: &CbmRsDiagSnapshot) -> diagnostics::Snapshot {
    diagnostics::Snapshot {
        uptime_s: c_long_to_i64(snapshot.uptime_s),
        rss_bytes: snapshot.rss_bytes,
        peak_rss_bytes: snapshot.peak_rss_bytes,
        heap_committed_bytes: snapshot.heap_committed_bytes,
        peak_committed_bytes: snapshot.peak_committed_bytes,
        page_faults: snapshot.page_faults,
        fd_count: snapshot.fd_count,
        queries: diagnostics::QuerySnapshot {
            count: snapshot.queries.count,
            errors: snapshot.queries.errors,
            total_us: snapshot.queries.total_us,
            avg_us: snapshot.queries.avg_us,
            max_us: snapshot.queries.max_us,
        },
        pid: snapshot.pid,
    }
}

#[cfg(windows)]
fn c_long_to_i64(value: c_long) -> i64 {
    i64::from(value)
}

#[cfg(not(windows))]
fn c_long_to_i64(value: c_long) -> i64 {
    value
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。`snapshot`
/// 必須是 null，或指向有效的 `CbmRsDiagSnapshot`。
pub unsafe extern "C" fn cbm_rs_diag_format_json(
    buf: *mut c_char,
    bufsize: usize,
    snapshot: *const CbmRsDiagSnapshot,
) -> c_int {
    if snapshot.is_null() {
        return -1;
    }
    let snapshot = diag_snapshot_from_ffi(unsafe { &*snapshot });
    let output = diagnostics::format_json(&snapshot);
    unsafe { write_c_output_i32(buf, bufsize, Some(output.as_slice())) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。`snapshot`
/// 必須是 null，或指向有效的 `CbmRsDiagSnapshot`。
pub unsafe extern "C" fn cbm_rs_diag_format_ndjson(
    buf: *mut c_char,
    bufsize: usize,
    snapshot: *const CbmRsDiagSnapshot,
) -> c_int {
    if snapshot.is_null() {
        return -1;
    }
    let snapshot = diag_snapshot_from_ffi(unsafe { &*snapshot });
    let output = diagnostics::format_ndjson(&snapshot);
    unsafe { write_c_output_i32(buf, bufsize, Some(output.as_slice())) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；null 或
/// `bufsize == 0` 會回傳錯誤。此 API 僅回傳 Rust pipeline plan parity
/// fixture，不執行任何 pipeline pass。
pub unsafe extern "C" fn cbm_rs_pipeline_plan_describe(
    kind: c_int,
    mode: c_int,
    worker_count: c_int,
    file_count: c_int,
    buf: *mut c_char,
    bufsize: usize,
) -> c_int {
    let Some(kind) = PlanKind::from_i32(kind) else {
        return -1;
    };
    let output = plan::describe(kind, mode, worker_count, file_count);
    unsafe { write_c_output_i32(buf, bufsize, Some(output.as_bytes())) }
}

#[no_mangle]
/// # Safety
///
/// `entries` 必須是 null，或指向至少 `entry_count` 個 entry。所有 C string
/// 指標只會在此呼叫期間借用；Rust 不保存呼叫端傳入的指標。`qn_buf`、
/// `strategy_buf` 與 `out` 必須是 null，或指向可寫記憶體。
pub unsafe extern "C" fn cbm_rs_registry_resolve_once(
    entries: *const CbmRsRegistryEntry,
    entry_count: c_int,
    callee_name: *const c_char,
    module_qn: *const c_char,
    import_keys: *const *const c_char,
    import_vals: *const *const c_char,
    import_count: c_int,
    qn_buf: *mut c_char,
    qn_buf_size: usize,
    strategy_buf: *mut c_char,
    strategy_buf_size: usize,
    out: *mut CbmRsRegistryResolveOut,
) -> c_int {
    if !out.is_null() {
        unsafe {
            (*out).confidence = 0.0;
            (*out).candidate_count = 0;
            (*out).matched = 0;
        }
    }
    unsafe {
        write_c_output(qn_buf, qn_buf_size, Some(b""));
        write_c_output(strategy_buf, strategy_buf_size, Some(b""));
    }

    if entry_count < 0 || import_count < 0 || callee_name.is_null() {
        return -1;
    }
    if entry_count > 0 && entries.is_null() {
        return -1;
    }
    let raw_entries = if entry_count == 0 {
        &[][..]
    } else {
        unsafe { slice::from_raw_parts(entries, entry_count as usize) }
    };
    let mut registry_entries = Vec::with_capacity(raw_entries.len());
    for entry in raw_entries {
        let Some(qualified_name) = (unsafe { c_bytes(entry.qualified_name) }) else {
            continue;
        };
        registry_entries.push(RegistryEntry {
            qualified_name: qualified_name.to_vec(),
        });
    }
    let Some(callee) = (unsafe { c_bytes(callee_name) }) else {
        return -1;
    };
    let module = unsafe { c_bytes(module_qn) }.unwrap_or(EMPTY_CSTR.as_slice());
    let Some(keys) = (unsafe { c_string_parts(import_keys, import_count) }) else {
        return -1;
    };
    let Some(vals) = (unsafe { c_string_parts(import_vals, import_count) }) else {
        return -1;
    };

    let registry = Registry::from_entries(&registry_entries);
    let Some(resolution) = registry.resolve(callee, module, &keys, &vals) else {
        return 0;
    };

    unsafe {
        write_c_output(
            qn_buf,
            qn_buf_size,
            Some(resolution.qualified_name.as_slice()),
        );
        write_c_output(
            strategy_buf,
            strategy_buf_size,
            Some(resolution.strategy.as_bytes()),
        );
        if !out.is_null() {
            (*out).confidence = resolution.confidence;
            (*out).candidate_count = resolution.candidate_count;
            (*out).matched = 1;
        }
    }
    1
}

#[no_mangle]
pub extern "C" fn cbm_rs_registry_create() -> *mut CbmRsRegistryHandle {
    Box::into_raw(Box::new(CbmRsRegistryHandle {
        registry: Registry::default(),
    }))
}

#[no_mangle]
/// # Safety
///
/// `registry` 必須是 null，或是 `cbm_rs_registry_create` 回傳且尚未 free 的指標。
pub unsafe extern "C" fn cbm_rs_registry_free(registry: *mut CbmRsRegistryHandle) {
    if registry.is_null() {
        return;
    }
    unsafe {
        drop(Box::from_raw(registry));
    }
}

#[no_mangle]
/// # Safety
///
/// `registry` 必須是 `cbm_rs_registry_create` 回傳且仍有效的指標。
/// `qualified_name` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_registry_add(
    registry: *mut CbmRsRegistryHandle,
    qualified_name: *const c_char,
) -> c_int {
    if registry.is_null() || qualified_name.is_null() {
        return -1;
    }
    let Some(qn) = (unsafe { c_bytes(qualified_name) }) else {
        return -1;
    };
    unsafe { &mut *registry }.registry.add(qn.to_vec());
    1
}

#[no_mangle]
/// # Safety
///
/// `registry` 必須是 `cbm_rs_registry_create` 回傳且仍有效的指標。所有 C string
/// 指標只會在此呼叫期間借用；`qn_buf`、`strategy_buf` 與 `out` 必須是 null，
/// 或指向可寫記憶體。若 output buffer 太小，回傳 `-2`。
pub unsafe extern "C" fn cbm_rs_registry_resolve(
    registry: *const CbmRsRegistryHandle,
    callee_name: *const c_char,
    module_qn: *const c_char,
    import_keys: *const *const c_char,
    import_vals: *const *const c_char,
    import_count: c_int,
    qn_buf: *mut c_char,
    qn_buf_size: usize,
    strategy_buf: *mut c_char,
    strategy_buf_size: usize,
    out: *mut CbmRsRegistryResolveOut,
) -> c_int {
    if !out.is_null() {
        unsafe {
            (*out).confidence = 0.0;
            (*out).candidate_count = 0;
            (*out).matched = 0;
        }
    }
    unsafe {
        write_c_output(qn_buf, qn_buf_size, Some(b""));
        write_c_output(strategy_buf, strategy_buf_size, Some(b""));
    }

    if registry.is_null() || callee_name.is_null() || import_count < 0 {
        return -1;
    }
    let Some(callee) = (unsafe { c_bytes(callee_name) }) else {
        return -1;
    };
    let module = unsafe { c_bytes(module_qn) }.unwrap_or(EMPTY_CSTR.as_slice());
    let Some(keys) = (unsafe { c_string_parts(import_keys, import_count) }) else {
        return -1;
    };
    let Some(vals) = (unsafe { c_string_parts(import_vals, import_count) }) else {
        return -1;
    };

    let Some(resolution) = (unsafe { &*registry })
        .registry
        .resolve(callee, module, &keys, &vals)
    else {
        return 0;
    };

    let qn_len = unsafe {
        write_c_output(
            qn_buf,
            qn_buf_size,
            Some(resolution.qualified_name.as_slice()),
        )
    };
    let strategy_len = unsafe {
        write_c_output(
            strategy_buf,
            strategy_buf_size,
            Some(resolution.strategy.as_bytes()),
        )
    };
    if (!qn_buf.is_null() && qn_buf_size > 0 && qn_len >= qn_buf_size)
        || (!strategy_buf.is_null() && strategy_buf_size > 0 && strategy_len >= strategy_buf_size)
    {
        return -2;
    }
    if !out.is_null() {
        unsafe {
            (*out).confidence = resolution.confidence;
            (*out).candidate_count = resolution.candidate_count;
            (*out).matched = 1;
        }
    }
    1
}

#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或是有效且可寫的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_normalize_path_sep(path: *mut c_char) -> *mut c_char {
    if path.is_null() {
        return ptr::null_mut();
    }
    let len = unsafe { CStr::from_ptr(path) }.to_bytes().len();
    let bytes = unsafe { slice::from_raw_parts_mut(path.cast::<u8>(), len) };
    platform::normalize_path_sep_bytes(bytes);
    path
}

#[no_mangle]
/// # Safety
///
/// `name` 與 `fallback` 必須是 null，或是有效的 NUL-terminated C string。
/// `buf` 必須是 null，或是指向至少 `bufsize` bytes 的可寫記憶體。
/// 回傳完整輸出長度；若 C contract 應回傳 NULL，則回傳 `usize::MAX`。
pub unsafe extern "C" fn cbm_rs_safe_getenv(
    buf: *mut c_char,
    bufsize: usize,
    name: *const c_char,
    fallback: *const c_char,
) -> usize {
    let output =
        platform::safe_getenv_bytes(unsafe { c_bytes(name) }, unsafe { c_bytes(fallback) });
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或是指向至少 `bufsize` bytes 的可寫記憶體。
/// 回傳完整輸出長度；若 C contract 應回傳 NULL，則回傳 `usize::MAX`。
pub unsafe extern "C" fn cbm_rs_get_home_dir(buf: *mut c_char, bufsize: usize) -> usize {
    let output = platform::get_home_dir_bytes();
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或是指向至少 `bufsize` bytes 的可寫記憶體。
/// 回傳完整輸出長度；若 C contract 應回傳 NULL，則回傳 `usize::MAX`。
pub unsafe extern "C" fn cbm_rs_app_config_dir(buf: *mut c_char, bufsize: usize) -> usize {
    let output = platform::app_config_dir_bytes();
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或是指向至少 `bufsize` bytes 的可寫記憶體。
/// 回傳完整輸出長度；若 C contract 應回傳 NULL，則回傳 `usize::MAX`。
pub unsafe extern "C" fn cbm_rs_app_local_dir(buf: *mut c_char, bufsize: usize) -> usize {
    let output = platform::app_local_dir_bytes();
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或是指向至少 `bufsize` bytes 的可寫記憶體。
/// 回傳完整輸出長度；若 C contract 應回傳 NULL，則回傳 `usize::MAX`。
pub unsafe extern "C" fn cbm_rs_resolve_cache_dir(buf: *mut c_char, bufsize: usize) -> usize {
    let output = platform::resolve_cache_dir_bytes();
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `cgroup_root` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_detect_cgroup_cpus(cgroup_root: *const c_char) -> c_int {
    let Some(root) = (unsafe { c_bytes(cgroup_root) }) else {
        return -1;
    };
    let root = String::from_utf8_lossy(root);
    platform::detect_cgroup_cpus(Path::new(root.as_ref())) as c_int
}

#[no_mangle]
/// # Safety
///
/// `cgroup_root` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_detect_cgroup_mem(cgroup_root: *const c_char) -> usize {
    let Some(root) = (unsafe { c_bytes(cgroup_root) }) else {
        return 0;
    };
    let root = String::from_utf8_lossy(root);
    platform::detect_cgroup_mem(Path::new(root.as_ref()))
}

#[no_mangle]
/// # Safety
///
/// `s` 與 `prefix` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_str_starts_with(s: *const c_char, prefix: *const c_char) -> c_int {
    str_util::starts_with_bytes(unsafe { c_bytes(s) }, unsafe { c_bytes(prefix) }) as c_int
}

#[no_mangle]
/// # Safety
///
/// `s` 與 `suffix` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_str_ends_with(s: *const c_char, suffix: *const c_char) -> c_int {
    str_util::ends_with_bytes(unsafe { c_bytes(s) }, unsafe { c_bytes(suffix) }) as c_int
}

#[no_mangle]
/// # Safety
///
/// `s` 與 `sub` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_str_contains(s: *const c_char, sub: *const c_char) -> c_int {
    str_util::contains_bytes(unsafe { c_bytes(s) }, unsafe { c_bytes(sub) }) as c_int
}

#[no_mangle]
/// # Safety
///
/// `base` 與 `name` 必須是 null，或是有效的 NUL-terminated C string。
/// `buf` 必須是 null，或是指向至少 `bufsize` bytes 的可寫記憶體。
/// 回傳完整輸出長度；若 C contract 應回傳 NULL，則回傳 `usize::MAX`。
pub unsafe extern "C" fn cbm_rs_path_join(
    buf: *mut c_char,
    bufsize: usize,
    base: *const c_char,
    name: *const c_char,
) -> usize {
    let output = str_util::path_join_bytes(unsafe { c_bytes(base) }, unsafe { c_bytes(name) });
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `parts` 必須是 null，或是指向至少 `n` 個 NUL-terminated C string 指標。
/// `buf` 必須是 null，或是指向至少 `bufsize` bytes 的可寫記憶體。
/// 回傳完整輸出長度；若 C contract 應回傳 NULL，則回傳 `usize::MAX`。
pub unsafe extern "C" fn cbm_rs_path_join_n(
    buf: *mut c_char,
    bufsize: usize,
    parts: *const *const c_char,
    n: c_int,
) -> usize {
    let output = unsafe { c_string_parts(parts, n) }.and_then(|parts| {
        let Some((first, rest)) = parts.split_first() else {
            return Some(Vec::new());
        };
        let mut joined = (*first).to_vec();
        for part in rest {
            joined = str_util::path_join_bytes(Some(&joined), Some(part))?;
        }
        Some(joined)
    });
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_path_ext(path: *const c_char) -> *const c_char {
    let Some(bytes) = (unsafe { c_bytes(path) }) else {
        return EMPTY_CSTR.as_ptr().cast();
    };
    match str_util::path_ext_offset(Some(bytes)) {
        Some(offset) => unsafe { path.add(offset) },
        None => EMPTY_CSTR.as_ptr().cast(),
    }
}

#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_path_base(path: *const c_char) -> *const c_char {
    let Some(bytes) = (unsafe { c_bytes(path) }) else {
        return EMPTY_CSTR.as_ptr().cast();
    };
    match str_util::path_base_offset(Some(bytes)) {
        Some(offset) => unsafe { path.add(offset) },
        None => EMPTY_CSTR.as_ptr().cast(),
    }
}

#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或是有效的 NUL-terminated C string。
/// `buf` 必須是 null，或是指向至少 `bufsize` bytes 的可寫記憶體。
pub unsafe extern "C" fn cbm_rs_path_dir(
    buf: *mut c_char,
    bufsize: usize,
    path: *const c_char,
) -> usize {
    let output = str_util::path_dir_bytes(unsafe { c_bytes(path) });
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `s` 必須是 null，或是有效的 NUL-terminated C string。
/// `buf` 必須是 null，或是指向至少 `bufsize` bytes 的可寫記憶體。
pub unsafe extern "C" fn cbm_rs_str_tolower(
    buf: *mut c_char,
    bufsize: usize,
    s: *const c_char,
) -> usize {
    let output = str_util::to_lower_bytes(unsafe { c_bytes(s) });
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `s` 必須是 null，或是有效的 NUL-terminated C string。
/// `buf` 必須是 null，或是指向至少 `bufsize` bytes 的可寫記憶體。
pub unsafe extern "C" fn cbm_rs_str_replace_char(
    buf: *mut c_char,
    bufsize: usize,
    s: *const c_char,
    from: c_char,
    to: c_char,
) -> usize {
    let output = str_util::replace_char_bytes(unsafe { c_bytes(s) }, from as u8, to as u8);
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或是有效的 NUL-terminated C string。
/// `buf` 必須是 null，或是指向至少 `bufsize` bytes 的可寫記憶體。
pub unsafe extern "C" fn cbm_rs_str_strip_ext(
    buf: *mut c_char,
    bufsize: usize,
    path: *const c_char,
) -> usize {
    let output = str_util::strip_ext_bytes(unsafe { c_bytes(path) });
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `s` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_str_split_count(s: *const c_char, delim: c_char) -> usize {
    str_util::split_bytes(unsafe { c_bytes(s) }, delim as u8)
        .map_or(usize::MAX, |parts| parts.len())
}

#[no_mangle]
/// # Safety
///
/// `s` 必須是 null，或是有效的 NUL-terminated C string。
/// `buf` 必須是 null，或是指向至少 `bufsize` bytes 的可寫記憶體。
pub unsafe extern "C" fn cbm_rs_str_split_part(
    buf: *mut c_char,
    bufsize: usize,
    s: *const c_char,
    delim: c_char,
    index: usize,
) -> usize {
    let output = str_util::split_bytes(unsafe { c_bytes(s) }, delim as u8)
        .and_then(|parts| parts.get(index).copied());
    unsafe { write_c_output(buf, bufsize, output) }
}

#[no_mangle]
/// # Safety
///
/// `s` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_validate_shell_arg(s: *const c_char) -> c_int {
    str_util::validate_shell_arg_bytes(unsafe { c_bytes(s) }) as c_int
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_validate_project_name(name: *const c_char) -> c_int {
    str_util::validate_project_name_bytes(unsafe { c_bytes(name) }) as c_int
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或是指向至少 `bufsize` bytes 的可寫記憶體。
/// `src` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_json_escape(
    buf: *mut c_char,
    bufsize: c_int,
    src: *const c_char,
) -> c_int {
    if buf.is_null() || bufsize <= 0 {
        return 0;
    }

    let bufsize = bufsize as usize;
    if bufsize > isize::MAX as usize {
        return 0;
    }

    let src = unsafe { c_bytes(src) };
    let (escaped, len) = str_util::json_escape(src, bufsize);
    if len > 0 {
        unsafe {
            ptr::copy_nonoverlapping(escaped.as_ptr().cast::<c_char>(), buf, len);
        }
    }
    unsafe {
        *buf.add(len) = 0;
    }
    len as c_int
}

// ── hash_table：test-only borrowed-pointer 契約 parity FFI ──────────
//
// 對齊 `src/foundation/hash_table.c`（Verstable 實作）的公開契約。此組
// symbols 只由 `make -f Makefile.cbm rust-ffi-test` 的 parity fixture 使用，
// 不接入任何產品 opt-in（hash_table 為 hot path，暫維持 C-only）。

/// foreach callback：`(stored_key_ptr, value_ptr, userdata)`。
pub type CbmRsHtIterFn = extern "C" fn(*const c_char, *mut c_void, *mut c_void);

#[no_mangle]
pub extern "C" fn cbm_rs_ht_create() -> *mut HashTable {
    Box::into_raw(Box::new(HashTable::new()))
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須是 null，或是 `cbm_rs_ht_create` 回傳且尚未 free 的指標。
pub unsafe extern "C" fn cbm_rs_ht_free(ht: *mut HashTable) {
    if ht.is_null() {
        return;
    }
    unsafe {
        drop(Box::from_raw(ht));
    }
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須有效或 null；`key` 必須是 null，或指向 NUL 結尾的 C string。
/// 對齊 `cbm_ht_set`：回傳前一個 value 指標，或 null（無前值 / ht/key 為 null）。
pub unsafe extern "C" fn cbm_rs_ht_set(
    ht: *mut HashTable,
    key: *const c_char,
    value: *mut c_void,
) -> *mut c_void {
    if ht.is_null() || key.is_null() {
        return ptr::null_mut();
    }
    let bytes = unsafe { CStr::from_ptr(key) }.to_bytes();
    let table = unsafe { &mut *ht };
    match table.set(bytes, key as usize, value as usize) {
        Some(prev) => prev as *mut c_void,
        None => ptr::null_mut(),
    }
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須有效或 null；`key` 必須是 null，或指向 NUL 結尾的 C string。
/// 對齊 `cbm_ht_get`：回傳 value 指標，或 null（不存在 / null value / ht/key 為 null）。
pub unsafe extern "C" fn cbm_rs_ht_get(ht: *const HashTable, key: *const c_char) -> *mut c_void {
    if ht.is_null() || key.is_null() {
        return ptr::null_mut();
    }
    let bytes = unsafe { CStr::from_ptr(key) }.to_bytes();
    match unsafe { &*ht }.get(bytes) {
        Some(v) => v as *mut c_void,
        None => ptr::null_mut(),
    }
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須有效或 null；`key` 必須是 null，或指向 NUL 結尾的 C string。
/// 對齊 `cbm_ht_has`。
pub unsafe extern "C" fn cbm_rs_ht_has(ht: *const HashTable, key: *const c_char) -> bool {
    if ht.is_null() || key.is_null() {
        return false;
    }
    let bytes = unsafe { CStr::from_ptr(key) }.to_bytes();
    unsafe { &*ht }.has(bytes)
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須有效或 null；`key` 必須是 null，或指向 NUL 結尾的 C string。
/// 對齊 `cbm_ht_get_key`：回傳 stored key 指標（最近一次 set 傳入者），或 null。
pub unsafe extern "C" fn cbm_rs_ht_get_key(
    ht: *const HashTable,
    key: *const c_char,
) -> *const c_char {
    if ht.is_null() || key.is_null() {
        return ptr::null();
    }
    let bytes = unsafe { CStr::from_ptr(key) }.to_bytes();
    match unsafe { &*ht }.get_key(bytes) {
        Some(k) => k as *const c_char,
        None => ptr::null(),
    }
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須有效或 null；`key` 必須是 null，或指向 NUL 結尾的 C string。
/// 對齊 `cbm_ht_delete`：回傳移除的 value 指標，或 null。
pub unsafe extern "C" fn cbm_rs_ht_delete(ht: *mut HashTable, key: *const c_char) -> *mut c_void {
    if ht.is_null() || key.is_null() {
        return ptr::null_mut();
    }
    let bytes = unsafe { CStr::from_ptr(key) }.to_bytes();
    let table = unsafe { &mut *ht };
    match table.delete(bytes) {
        Some(v) => v as *mut c_void,
        None => ptr::null_mut(),
    }
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須有效或 null。對齊 `cbm_ht_count`。
pub unsafe extern "C" fn cbm_rs_ht_count(ht: *const HashTable) -> u32 {
    if ht.is_null() {
        return 0;
    }
    unsafe { &*ht }.count() as u32
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須有效或 null。對齊 `cbm_ht_clear`。
pub unsafe extern "C" fn cbm_rs_ht_clear(ht: *mut HashTable) {
    if ht.is_null() {
        return;
    }
    unsafe { &mut *ht }.clear();
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須有效或 null；`func` 為 null 時為 no-op。`userdata` 原樣回傳給
/// callback。對齊 `cbm_ht_foreach`：每項恰好呼叫一次，順序未定義。
pub unsafe extern "C" fn cbm_rs_ht_foreach(
    ht: *const HashTable,
    func: Option<CbmRsHtIterFn>,
    userdata: *mut c_void,
) {
    if ht.is_null() {
        return;
    }
    let Some(func) = func else {
        return;
    };
    for (key_ptr, value_ptr) in unsafe { &*ht }.entries() {
        func(key_ptr as *const c_char, value_ptr as *mut c_void, userdata);
    }
}
