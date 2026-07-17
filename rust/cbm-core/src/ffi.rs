//! 階段性 Rust migration 的 C ABI exports。
//!
//! 這些 symbols 由 `make -f Makefile.cbm rust-ffi-test` 驗證；只有明確 opt-in
//! 的 C wrapper 會呼叫它們。

#![allow(unsafe_code)]

use std::alloc::{alloc, dealloc, Layout};
use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_long, c_void};
use std::path::{Path, PathBuf};
use std::process::Command;
use std::ptr;
use std::slice;

use crate::cli::hook as cli_hook;
use crate::cli::progress_sink as cli_progress_sink;
use crate::cli::version as cli_version;
use crate::cypher;
use crate::discover as discover_filter;
use crate::foundation::compat as foundation_compat;
use crate::foundation::diagnostics;
use crate::foundation::dump_verify;
use crate::foundation::hash_table::HashTable;
use crate::foundation::log as foundation_log;
use crate::foundation::mem as foundation_mem;
use crate::foundation::platform;
use crate::foundation::profile as foundation_profile;
use crate::foundation::str_intern::CInternPool;
use crate::foundation::str_util;
use crate::foundation::yaml::Node as YamlNode;
use crate::git_context as git_runtime;
use crate::mcp;
use crate::pipeline::artifact as pipeline_artifact;
use crate::pipeline::configures as pipeline_configures;
use crate::pipeline::exception as pipeline_exception;
use crate::pipeline::fqn as pipeline_fqn;
use crate::pipeline::gitdiff as pipeline_gitdiff;
use crate::pipeline::githistory as pipeline_githistory;
use crate::pipeline::graph_mutation;
use crate::pipeline::infrascan as pipeline_infrascan;
use crate::pipeline::module as pipeline_module;
use crate::pipeline::plan::{self, PlanKind};
use crate::pipeline::registry::{Entry as RegistryEntry, Registry};
use crate::pipeline::route as pipeline_route;
use crate::pipeline::test_detect as pipeline_test_detect;
use crate::store::{
    arch_helpers as store_arch_helpers, open as store_open, pragmas as store_pragmas,
    schema_manifest, search_pattern as store_search_pattern, tokenization,
};
use crate::watcher as watcher_runtime;
use std::sync::{Mutex, OnceLock};

#[no_mangle]
/// # Safety
///
/// `rel_path` 與 `local_prefix` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_discover_local_rel_path_offset_v1(
    rel_path: *const c_char,
    local_prefix: *const c_char,
) -> usize {
    discover_filter::local_rel_path_offset(unsafe { c_bytes(rel_path) }, unsafe {
        c_bytes(local_prefix)
    })
}

#[cfg(feature = "discover-local-rel-path-only")]
#[no_mangle]
/// # Safety
///
/// `rel_path` 與 `local_prefix` 必須是 null，或是有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_discover_local_rel_path_offset(
    rel_path: *const c_char,
    local_prefix: *const c_char,
) -> usize {
    unsafe { cbm_rs_discover_local_rel_path_offset_v1(rel_path, local_prefix) }
}

#[cfg(not(windows))]
#[allow(improper_ctypes)]
unsafe extern "C" {
    fn popen(cmd: *const c_char, mode: *const c_char) -> *mut c_void;
    fn pclose(f: *mut c_void) -> c_int;
}

#[cfg(windows)]
#[allow(improper_ctypes)]
unsafe extern "C" {
    fn _popen(cmd: *const c_char, mode: *const c_char) -> *mut c_void;
    fn _pclose(f: *mut c_void) -> c_int;
}

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

#[repr(C)]
pub struct CbmRsMcpToolsPageBoundsV1 {
    pub start: c_int,
    pub end: c_int,
    pub has_next: c_int,
    pub next_cursor: c_int,
}

#[repr(C)]
pub struct CbmRsPipelinePlanStep {
    pub kind: c_int,
    pub policy: c_int,
}

#[repr(C)]
pub struct CbmRsPipelinePlanStepV2 {
    pub kind: c_int,
    pub phase: c_int,
    pub policy: c_int,
    pub gate_flags: u32,
    pub requires_mask: u64,
    pub effect_flags: u32,
}

#[repr(C)]
pub struct CbmRsPipelineTopStepV1 {
    pub kind: c_int,
    pub phase: c_int,
    pub policy: c_int,
    pub gate_flags: u32,
    pub requires_mask: u64,
    pub effect_flags: u32,
    pub nested_plan_kind: c_int,
}

#[repr(C)]
pub struct CbmRsGbufMutationMetaV1 {
    pub kind: c_int,
    pub result_kind: c_int,
    pub required_fields: u32,
    pub optional_fields: u32,
    pub effect_flags: u32,
    pub validation_flags: u32,
    pub reserved0: u32,
    pub reserved1: u32,
}

#[repr(C)]
pub struct CbmRsGbufMutationCmdV1 {
    pub kind: c_int,
    pub reserved0: c_int,
    pub label: *const c_char,
    pub name: *const c_char,
    pub qualified_name: *const c_char,
    pub file_path: *const c_char,
    pub start_line: c_int,
    pub end_line: c_int,
    pub source_id: i64,
    pub target_id: i64,
    pub edge_type: *const c_char,
    pub properties_json: *const c_char,
}

#[repr(C)]
pub struct CbmRsGbufMutationValidationV1 {
    pub ok: c_int,
    pub error_code: c_int,
    pub missing_fields: u32,
    pub invalid_fields: u32,
    pub normalized_flags: u32,
    pub reserved0: u32,
}

#[repr(C)]
pub struct CbmRsStoreSchemaManifestEntryV1 {
    pub kind: c_int,
    pub flags: u32,
    pub name: *const c_char,
    pub table_name: *const c_char,
    pub column_name: *const c_char,
}

#[repr(C)]
pub struct CbmRsStoreSchemaManifestEntryV2 {
    pub kind: c_int,
    pub flags: u32,
    pub ordinal: c_int,
    pub hidden: c_int,
    pub name: *const c_char,
    pub table_name: *const c_char,
    pub column_name: *const c_char,
    pub column_type: *const c_char,
    pub default_sql: *const c_char,
    pub columns_csv: *const c_char,
    pub sql_fragment: *const c_char,
}

#[repr(C)]
pub struct CbmRsStoreConnectionManifestEntryV1 {
    pub kind: c_int,
    pub flags: u32,
    pub mode_mask: u32,
    pub ordinal: c_int,
    pub value_i64: i64,
    pub name: *const c_char,
    pub sql: *const c_char,
    pub env_name: *const c_char,
}

#[repr(C)]
pub struct CbmRsCypherTokenV1 {
    pub kind: c_int,
    pub pos: c_int,
    pub text_len: c_int,
    pub reserved0: c_int,
}

#[repr(C)]
pub struct CbmRsTraceAttrV1 {
    pub key: *const c_char,
    pub string_value: *const c_char,
}

#[repr(C)]
pub struct CbmRsTraceResourceV1 {
    pub attributes: *const CbmRsTraceAttrV1,
    pub attr_count: c_int,
}

#[repr(C)]
pub struct CbmRsTraceSpanV1 {
    pub kind: c_int,
    pub attributes: *const CbmRsTraceAttrV1,
    pub attr_count: c_int,
    pub start_time: *const c_char,
    pub end_time: *const c_char,
}

#[repr(C)]
pub struct CbmRsChangedFileV1 {
    pub status: [c_char; 4],
    pub path: [c_char; 512],
    pub old_path: [c_char; 512],
}

#[repr(C)]
pub struct CbmRsChangedHunkV1 {
    pub path: [c_char; 512],
    pub start_line: c_int,
    pub end_line: c_int,
}

#[repr(C)]
pub struct CbmRsHttpSpanInfoV1 {
    pub service_name: *const c_char,
    pub method: *const c_char,
    pub path: *const c_char,
    pub status_code: *const c_char,
    pub span_kind: c_int,
    pub duration_ns: i64,
}

static mut TRACE_URL_BUF: [c_char; crate::traces::URL_BUF_CAP] = [0; crate::traces::URL_BUF_CAP];

#[repr(C)]
pub struct CbmRsMcpJsonRpcParseOutV1 {
    pub id: i64,
    pub has_id: c_int,
    pub id_kind: c_int,
    pub has_params: c_int,
    pub jsonrpc_len: usize,
    pub method_len: usize,
    pub id_str_len: usize,
    pub params_len: usize,
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

fn parse_dump_verify_min_ratio(value: *const c_char) -> (f64, c_int) {
    let (ratio, valid) = dump_verify::parse_min_ratio_bytes(unsafe { c_bytes(value) });
    (ratio, valid as c_int)
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null，或是有效的 NUL-terminated C string。
/// `valid_out` 必須是 null，或是指向至少一個 `c_int` 的可寫記憶體。
/// 回傳解析後的 ratio；`valid_out` 會寫入 0/1 反映是否落在 `0..=1`。
pub unsafe extern "C" fn cbm_rs_dump_verify_parse_min_ratio_v1(
    value: *const c_char,
    valid_out: *mut c_int,
) -> f64 {
    let (ratio, valid) = parse_dump_verify_min_ratio(value);
    if !valid_out.is_null() {
        unsafe {
            *valid_out = valid;
        }
    }
    ratio
}

#[cfg(feature = "dump-verify-only")]
unsafe extern "C" {
    fn cbm_log_dump_verify_invalid_ratio(value: *const c_char);
}

#[cfg(feature = "dump-verify-only")]
fn log_dump_verify_invalid(value: &[u8]) {
    let mut value_buf = [0_u8; 32];
    let len = value.len().min(value_buf.len() - 1);
    value_buf[..len].copy_from_slice(&value[..len]);

    unsafe {
        cbm_log_dump_verify_invalid_ratio(value_buf.as_ptr().cast());
    }
}

#[cfg(feature = "dump-verify-only")]
#[no_mangle]
pub extern "C" fn cbm_dump_verify_is_degraded(
    committed_nodes: c_int,
    persisted_nodes: c_int,
    ratio: f64,
    min_floor: c_int,
) -> bool {
    cbm_rs_dump_verify_is_degraded(committed_nodes, persisted_nodes, ratio, min_floor) != 0
}

#[cfg(feature = "dump-verify-only")]
#[no_mangle]
pub extern "C" fn cbm_dump_verify_min_ratio() -> f64 {
    let Some(mut value) = platform::safe_getenv_bytes(Some(b"CBM_DUMP_VERIFY_MIN_RATIO"), None)
    else {
        return dump_verify::DEFAULT_RATIO;
    };
    value.truncate(31);

    let (ratio, valid) = dump_verify::parse_min_ratio_bytes(Some(value.as_slice()));
    if valid {
        return ratio;
    }

    log_dump_verify_invalid(&value);
    dump_verify::DEFAULT_RATIO
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

#[cfg(feature = "str-intern-only")]
#[no_mangle]
pub extern "C" fn cbm_intern_create() -> *mut CInternPool {
    cbm_rs_intern_create()
}

#[cfg(feature = "str-intern-only")]
#[no_mangle]
/// # Safety
///
/// `pool` 必須是 null，或是 `cbm_intern_create` 回傳且尚未 free 的指標。
pub unsafe extern "C" fn cbm_intern_free(pool: *mut CInternPool) {
    unsafe { cbm_rs_intern_free(pool) }
}

#[cfg(feature = "str-intern-only")]
#[no_mangle]
/// # Safety
///
/// `pool` 必須是 null，或是仍有效的 direct intern pool；`value` 必須是 null，
/// 或指向至少 `len` bytes 的可讀記憶體。
pub unsafe extern "C" fn cbm_intern_n(
    pool: *mut CInternPool,
    value: *const c_char,
    len: usize,
) -> *const c_char {
    if value.is_null() {
        return ptr::null();
    }

    unsafe { cbm_rs_intern_n(pool, value.cast::<u8>(), len).cast() }
}

#[cfg(feature = "str-intern-only")]
#[no_mangle]
/// # Safety
///
/// `pool` 必須是 null，或是仍有效的 direct intern pool；`value` 必須是 null，
/// 或指向 NUL-terminated C string。
pub unsafe extern "C" fn cbm_intern(pool: *mut CInternPool, value: *const c_char) -> *const c_char {
    if value.is_null() {
        return ptr::null();
    }

    let len = unsafe { CStr::from_ptr(value).to_bytes().len() };
    unsafe { cbm_intern_n(pool, value, len) }
}

#[cfg(feature = "str-intern-only")]
#[no_mangle]
/// # Safety
///
/// `pool` 必須是 null，或是仍有效的 direct intern pool。
pub unsafe extern "C" fn cbm_intern_count(pool: *const CInternPool) -> u32 {
    unsafe { cbm_rs_intern_count(pool) }
}

#[cfg(feature = "str-intern-only")]
#[no_mangle]
/// # Safety
///
/// `pool` 必須是 null，或是仍有效的 direct intern pool。
pub unsafe extern "C" fn cbm_intern_bytes(pool: *const CInternPool) -> usize {
    unsafe { cbm_rs_intern_bytes(pool) }
}

#[no_mangle]
/// # Safety
///
/// `r` 必須是 null，或指向 layout-compatible `cbm_trace_resource_t`。
pub unsafe extern "C" fn cbm_rs_traces_extract_service_name_v1(
    r: *const CbmRsTraceResourceV1,
) -> *const c_char {
    if r.is_null() {
        return EMPTY_CSTR.as_ptr().cast();
    }

    let resource = unsafe { &*r };
    let attributes = unsafe { trace_attr_slice(resource.attributes, resource.attr_count) };
    crate::traces::extract_service_name(unsafe { trace_attributes(attributes) })
        .map_or_else(|| EMPTY_CSTR.as_ptr().cast(), |value| value.as_ptr().cast())
}

#[no_mangle]
/// # Safety
///
/// `span` 必須是 null，或指向 layout-compatible `cbm_trace_span_t`。
/// `out` 必須是 null，或指向 layout-compatible `cbm_http_span_info_t`。
pub unsafe extern "C" fn cbm_rs_traces_extract_http_info_v1(
    span: *const CbmRsTraceSpanV1,
    service_name: *const c_char,
    out: *mut CbmRsHttpSpanInfoV1,
) -> bool {
    if span.is_null() || out.is_null() {
        return false;
    }

    unsafe {
        ptr::write_bytes(
            out.cast::<u8>(),
            0,
            core::mem::size_of::<CbmRsHttpSpanInfoV1>(),
        );
    }
    let span = unsafe { &*span };
    unsafe {
        (*out).service_name = if service_name.is_null() {
            EMPTY_CSTR.as_ptr().cast()
        } else {
            service_name
        };
        (*out).span_kind = span.kind;
    }

    let attributes = unsafe { trace_attr_slice(span.attributes, span.attr_count) };
    let info = crate::traces::extract_http_info(unsafe { trace_attributes(attributes) });
    if let Some(method) = info.method {
        unsafe {
            (*out).method = method.as_ptr().cast();
        }
    }
    if let Some(status_code) = info.status_code {
        unsafe {
            (*out).status_code = status_code.as_ptr().cast();
        }
    }

    let Some(path) = info.path else {
        return false;
    };
    let path = match path {
        crate::traces::HttpPath::Attribute(path) => path.as_ptr().cast(),
        crate::traces::HttpPath::Url(url) => {
            let output = unsafe {
                slice::from_raw_parts_mut(
                    core::ptr::addr_of_mut!(TRACE_URL_BUF).cast::<u8>(),
                    crate::traces::URL_BUF_CAP,
                )
            };
            crate::traces::write_path(url, output);
            if output[0] == 0 {
                return false;
            }
            output.as_ptr().cast()
        }
    };
    unsafe {
        (*out).path = path;
    }
    if !crate::traces::http_info_has_path(info) {
        return false;
    }

    unsafe {
        (*out).duration_ns =
            crate::traces::parse_duration(c_bytes(span.start_time), c_bytes(span.end_time));
    }
    true
}

#[no_mangle]
/// # Safety
///
/// `url` 必須是 null，或指向 NUL-terminated C string。
/// `buf` 必須是 null，或指向至少 `buf_sz` bytes 的可寫記憶體。
pub unsafe extern "C" fn cbm_rs_traces_extract_path_from_url_v1(
    url: *const c_char,
    buf: *mut c_char,
    buf_sz: usize,
) -> *const c_char {
    if buf.is_null() {
        return EMPTY_CSTR.as_ptr().cast();
    }
    if buf_sz == 0 {
        return buf;
    }

    let output = unsafe { slice::from_raw_parts_mut(buf.cast::<u8>(), buf_sz) };
    crate::traces::write_path_from_url(unsafe { c_bytes(url) }, output);
    buf
}

#[no_mangle]
/// # Safety
///
/// `start_nano` 與 `end_nano` 必須是 null，或指向 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_traces_parse_duration_v1(
    start_nano: *const c_char,
    end_nano: *const c_char,
) -> i64 {
    crate::traces::parse_duration(unsafe { c_bytes(start_nano) }, unsafe { c_bytes(end_nano) })
}

#[no_mangle]
/// # Safety
///
/// `values` 必須是 null，或指向至少 `count` 個 `int64_t`。
pub unsafe extern "C" fn cbm_rs_traces_calculate_p99_v1(values: *mut i64, count: c_int) -> i64 {
    if values.is_null() || count <= 0 {
        return 0;
    }
    let values = unsafe { slice::from_raw_parts_mut(values, count as usize) };
    crate::traces::calculate_p99(values)
}

#[cfg(feature = "traces-only")]
#[no_mangle]
/// # Safety
///
/// `r` 必須是 null，或指向與 `cbm_trace_resource_t` layout 相容的有效資料。
/// 這是 `CBM_USE_RUST_TRACES_ONLY=1` 時直接取代 C helper 的 `traces.h` ABI。
pub unsafe extern "C" fn cbm_extract_service_name(r: *const CbmRsTraceResourceV1) -> *const c_char {
    unsafe { cbm_rs_traces_extract_service_name_v1(r) }
}

#[cfg(feature = "traces-only")]
#[no_mangle]
/// # Safety
///
/// `span` 與 `out` 必須分別是 null 或與 `cbm_trace_span_t` / `cbm_http_span_info_t`
/// layout 相容的有效記憶體。這是 Rust-only traces 的既有 C ABI。
pub unsafe extern "C" fn cbm_extract_http_info(
    span: *const CbmRsTraceSpanV1,
    service_name: *const c_char,
    out: *mut CbmRsHttpSpanInfoV1,
) -> bool {
    unsafe { cbm_rs_traces_extract_http_info_v1(span, service_name, out) }
}

#[cfg(feature = "traces-only")]
#[no_mangle]
/// # Safety
///
/// `url` 必須是 null，或有效的 NUL-terminated C string；`buf` 必須是 null，或指向
/// 至少 `buf_sz` bytes 的可寫記憶體。這是 Rust-only traces 的既有 C ABI。
pub unsafe extern "C" fn cbm_extract_path_from_url(
    url: *const c_char,
    buf: *mut c_char,
    buf_sz: usize,
) -> *const c_char {
    unsafe { cbm_rs_traces_extract_path_from_url_v1(url, buf, buf_sz) }
}

#[cfg(feature = "traces-only")]
#[no_mangle]
/// # Safety
///
/// `start_nano` 與 `end_nano` 必須是 null，或有效的 NUL-terminated C string。
/// 這是 Rust-only traces 的既有 C ABI。
pub unsafe extern "C" fn cbm_parse_duration(
    start_nano: *const c_char,
    end_nano: *const c_char,
) -> i64 {
    unsafe { cbm_rs_traces_parse_duration_v1(start_nano, end_nano) }
}

#[cfg(feature = "traces-only")]
#[no_mangle]
/// # Safety
///
/// `values` 必須是 null，或指向至少 `count` 個 `int64_t` 的可讀寫記憶體。
/// 這是 Rust-only traces 的既有 C ABI。
pub unsafe extern "C" fn cbm_calculate_p99(values: *mut i64, count: c_int) -> i64 {
    unsafe { cbm_rs_traces_calculate_p99_v1(values, count) }
}

unsafe fn c_bytes<'a>(value: *const c_char) -> Option<&'a [u8]> {
    if value.is_null() {
        return None;
    }
    Some(unsafe { CStr::from_ptr(value) }.to_bytes())
}

unsafe fn trace_attr_slice<'a>(
    attributes: *const CbmRsTraceAttrV1,
    attr_count: c_int,
) -> &'a [CbmRsTraceAttrV1] {
    if attributes.is_null() || attr_count <= 0 {
        return &[];
    }
    unsafe { slice::from_raw_parts(attributes, attr_count as usize) }
}

unsafe fn trace_attributes<'a>(
    attributes: &'a [CbmRsTraceAttrV1],
) -> impl Iterator<Item = crate::traces::TraceAttribute<'a>> + 'a {
    attributes.iter().filter_map(|attr| {
        if attr.key.is_null() {
            return None;
        }
        let key = unsafe { c_bytes(attr.key) }?;
        Some(crate::traces::TraceAttribute {
            key,
            string_value: unsafe { c_bytes(attr.string_value) },
        })
    })
}

unsafe fn c_bytes_with_len<'a>(value: *const u8, len: c_int) -> Option<&'a [u8]> {
    if value.is_null() || len < 0 {
        return None;
    }
    Some(unsafe { slice::from_raw_parts(value, len as usize) })
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

unsafe fn copy_c_path_without_query_bounded(path: *const c_char, buf: *mut c_char, bufsize: usize) {
    if buf.is_null() || bufsize == 0 {
        return;
    }
    unsafe { *buf = 0 };
    if path.is_null() {
        return;
    }

    let mut index = 0;
    while index < bufsize - 1 {
        let byte = unsafe { *path.add(index) as u8 };
        if byte == 0 || byte == b'?' || byte == b'#' {
            break;
        }
        unsafe { *buf.add(index) = byte as c_char };
        index += 1;
    }
    unsafe { *buf.add(index) = 0 };
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

const ARENA_MAX_BLOCKS: usize = 256;
const ARENA_DEFAULT_BLOCK_SIZE: usize = 64 * 1024;
const ARENA_MIN_BLOCK_SIZE: usize = 64;
const ARENA_ALIGN_MASK: usize = 7;
const ARENA_BLOCK_ALIGN: usize = 8;

#[repr(C)]
pub struct CbmRsArena {
    pub blocks: [*mut c_char; ARENA_MAX_BLOCKS],
    pub block_sizes: [usize; ARENA_MAX_BLOCKS],
    pub nblocks: c_int,
    pub block_size: usize,
    pub used: usize,
    pub total_alloc: usize,
}

fn arena_empty() -> CbmRsArena {
    CbmRsArena {
        blocks: [ptr::null_mut(); ARENA_MAX_BLOCKS],
        block_sizes: [0; ARENA_MAX_BLOCKS],
        nblocks: 0,
        block_size: 0,
        used: 0,
        total_alloc: 0,
    }
}

unsafe fn arena_alloc_block(size: usize) -> *mut c_char {
    let Ok(layout) = Layout::from_size_align(size, ARENA_BLOCK_ALIGN) else {
        return ptr::null_mut();
    };
    unsafe { alloc(layout).cast::<c_char>() }
}

unsafe fn arena_free_block(ptr: *mut c_char, size: usize) {
    if ptr.is_null() || size == 0 {
        return;
    }
    let Ok(layout) = Layout::from_size_align(size, ARENA_BLOCK_ALIGN) else {
        return;
    };
    unsafe {
        dealloc(ptr.cast::<u8>(), layout);
    }
}

fn arena_align_size(n: usize) -> Option<usize> {
    Some(n.checked_add(ARENA_ALIGN_MASK)? & !ARENA_ALIGN_MASK)
}

unsafe fn arena_grow(arena: &mut CbmRsArena, min_size: usize) -> bool {
    if arena.nblocks < 0 || arena.nblocks as usize >= ARENA_MAX_BLOCKS {
        return false;
    }
    let Some(doubled) = arena.block_size.checked_mul(2) else {
        return false;
    };
    let new_size = doubled.max(min_size);
    let block = unsafe { arena_alloc_block(new_size) };
    if block.is_null() {
        return false;
    }
    let index = arena.nblocks as usize;
    arena.blocks[index] = block;
    arena.block_sizes[index] = new_size;
    arena.nblocks += 1;
    arena.block_size = new_size;
    arena.used = 0;
    true
}

#[no_mangle]
/// # Safety
///
/// `arena` 必須是 null，或指向 layout-compatible `CBMArena` 的可寫記憶體。
pub unsafe extern "C" fn cbm_rs_arena_init(arena: *mut CbmRsArena) {
    unsafe { cbm_rs_arena_init_sized(arena, ARENA_DEFAULT_BLOCK_SIZE) };
}

#[no_mangle]
/// # Safety
///
/// `arena` 必須是 null，或指向 layout-compatible `CBMArena` 的可寫記憶體。
pub unsafe extern "C" fn cbm_rs_arena_init_sized(arena: *mut CbmRsArena, block_size: usize) {
    if arena.is_null() {
        return;
    }
    let block_size = block_size.max(ARENA_MIN_BLOCK_SIZE);
    let mut next = arena_empty();
    next.block_size = block_size;
    let block = unsafe { arena_alloc_block(block_size) };
    if !block.is_null() {
        next.blocks[0] = block;
        next.block_sizes[0] = block_size;
        next.nblocks = 1;
    }
    unsafe {
        *arena = next;
    }
}

#[no_mangle]
/// # Safety
///
/// `arena` 必須是 null，或指向由 `cbm_rs_arena_init*` 初始化且仍有效的
/// layout-compatible `CBMArena`。
pub unsafe extern "C" fn cbm_rs_arena_alloc(arena: *mut CbmRsArena, n: usize) -> *mut c_void {
    if arena.is_null() || n == 0 {
        return ptr::null_mut();
    }
    let arena = unsafe { &mut *arena };
    if arena.nblocks <= 0 {
        return ptr::null_mut();
    }
    let Some(aligned) = arena_align_size(n) else {
        return ptr::null_mut();
    };
    if aligned > arena.block_size.saturating_sub(arena.used)
        && !unsafe { arena_grow(arena, aligned) }
    {
        return ptr::null_mut();
    }
    let Some(new_used) = arena.used.checked_add(aligned) else {
        return ptr::null_mut();
    };
    let Some(new_total) = arena.total_alloc.checked_add(aligned) else {
        return ptr::null_mut();
    };
    let index = (arena.nblocks - 1) as usize;
    let ptr = unsafe { arena.blocks[index].add(arena.used) };
    arena.used = new_used;
    arena.total_alloc = new_total;
    ptr.cast::<c_void>()
}

#[no_mangle]
/// # Safety
///
/// `arena` 必須是 null，或指向由 `cbm_rs_arena_init*` 初始化且仍有效的
/// layout-compatible `CBMArena`。
pub unsafe extern "C" fn cbm_rs_arena_calloc(arena: *mut CbmRsArena, n: usize) -> *mut c_void {
    let ptr = unsafe { cbm_rs_arena_alloc(arena, n) };
    if !ptr.is_null() {
        unsafe {
            ptr::write_bytes(ptr, 0, n);
        }
    }
    ptr
}

#[no_mangle]
/// # Safety
///
/// `arena` 必須有效或 null；`value` 必須是 null，或指向 NUL-terminated
/// C string。
pub unsafe extern "C" fn cbm_rs_arena_strdup(
    arena: *mut CbmRsArena,
    value: *const c_char,
) -> *mut c_char {
    if value.is_null() {
        return ptr::null_mut();
    }
    let bytes = unsafe { CStr::from_ptr(value) }.to_bytes();
    unsafe { cbm_rs_arena_strndup(arena, bytes.as_ptr(), bytes.len()) }
}

#[no_mangle]
/// # Safety
///
/// `arena` 必須有效或 null；`value` 必須是 null，或指向至少 `len` bytes
/// 的可讀記憶體。
pub unsafe extern "C" fn cbm_rs_arena_strndup(
    arena: *mut CbmRsArena,
    value: *const u8,
    len: usize,
) -> *mut c_char {
    if value.is_null() {
        return ptr::null_mut();
    }
    let Some(len_with_nul) = len.checked_add(1) else {
        return ptr::null_mut();
    };
    let dst = unsafe { cbm_rs_arena_alloc(arena, len_with_nul) }.cast::<c_char>();
    if dst.is_null() {
        return ptr::null_mut();
    }
    unsafe {
        ptr::copy_nonoverlapping(value.cast::<c_char>(), dst, len);
        *dst.add(len) = 0;
    }
    dst
}

#[no_mangle]
/// # Safety
///
/// `arena` 必須是 null，或指向由 `cbm_rs_arena_init*` 初始化且仍有效的
/// layout-compatible `CBMArena`。
pub unsafe extern "C" fn cbm_rs_arena_reset(arena: *mut CbmRsArena) {
    if arena.is_null() {
        return;
    }
    let arena = unsafe { &mut *arena };
    let nblocks = arena.nblocks.max(0) as usize;
    for i in 1..nblocks.min(ARENA_MAX_BLOCKS) {
        unsafe { arena_free_block(arena.blocks[i], arena.block_sizes[i]) };
        arena.blocks[i] = ptr::null_mut();
        arena.block_sizes[i] = 0;
    }
    if arena.nblocks > 1 {
        arena.nblocks = 1;
    }
    arena.used = 0;
    arena.total_alloc = 0;
    if arena.nblocks == 1 {
        arena.block_size = arena.block_sizes[0];
    }
}

#[no_mangle]
/// # Safety
///
/// `arena` 必須是 null，或指向由 `cbm_rs_arena_init*` 初始化且仍有效的
/// layout-compatible `CBMArena`。
pub unsafe extern "C" fn cbm_rs_arena_destroy(arena: *mut CbmRsArena) {
    if arena.is_null() {
        return;
    }
    let arena_ref = unsafe { &mut *arena };
    let nblocks = arena_ref.nblocks.max(0) as usize;
    for i in 0..nblocks.min(ARENA_MAX_BLOCKS) {
        unsafe { arena_free_block(arena_ref.blocks[i], arena_ref.block_sizes[i]) };
    }
    unsafe {
        *arena = arena_empty();
    }
}

#[no_mangle]
/// # Safety
///
/// `arena` 必須是 null，或指向 layout-compatible `CBMArena`。
pub unsafe extern "C" fn cbm_rs_arena_total(arena: *const CbmRsArena) -> usize {
    if arena.is_null() {
        return 0;
    }
    unsafe { &*arena }.total_alloc
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
pub extern "C" fn cbm_rs_compat_regex_known_flags(flags: c_int) -> c_int {
    foundation_compat::regex_known_flags(flags)
}

#[no_mangle]
pub extern "C" fn cbm_rs_compat_regex_match_cap(nmatch: c_int, has_matches: c_int) -> c_int {
    foundation_compat::regex_match_cap(nmatch, has_matches != 0)
}

#[no_mangle]
pub extern "C" fn cbm_rs_compat_regex_status(matched: c_int) -> c_int {
    foundation_compat::regex_status_matched(matched != 0)
}

#[no_mangle]
pub extern "C" fn cbm_rs_compat_thread_effective_stack_size(stack_size: usize) -> usize {
    foundation_compat::thread_effective_stack_size(stack_size)
}

#[no_mangle]
pub extern "C" fn cbm_rs_compat_aligned_alloc_precheck(
    alignment: usize,
    size: usize,
    pointer_size: usize,
) -> bool {
    foundation_compat::aligned_alloc_precheck(alignment, size, pointer_size)
}

#[derive(Clone, Copy)]
#[repr(C)]
pub struct CbmCompatFsDirent {
    name: [u8; 260],
    is_dir: bool,
    d_type: u8,
}

struct CbmCompatFsDir {
    entries: Vec<CbmCompatFsDirent>,
    cursor: usize,
    current: CbmCompatFsDirent,
}

const CBM_RS_DIRENT_NAME_MAX: usize = 260;

fn cstr_to_path(path: *const c_char) -> Option<PathBuf> {
    if path.is_null() {
        None
    } else {
        let value = unsafe { CStr::from_ptr(path).to_string_lossy().into_owned() };
        Some(PathBuf::from(value))
    }
}

fn cstr_to_string(path: *const c_char) -> Option<String> {
    if path.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(path).to_string_lossy().into_owned() })
    }
}

fn copy_dirent_name(out: &mut [u8; CBM_RS_DIRENT_NAME_MAX], name: &str) {
    if CBM_RS_DIRENT_NAME_MAX == 0 {
        return;
    }
    let bytes = name.as_bytes();
    let n = bytes.len().min(CBM_RS_DIRENT_NAME_MAX - 1);
    out[..n].copy_from_slice(&bytes[..n]);
    out[n] = b'\0';
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_compat_fs_opendir(path: *const c_char) -> *mut c_void {
    let Some(dir_path) = cstr_to_path(path) else {
        return ptr::null_mut();
    };

    let mut entries = Vec::new();
    let iter = match std::fs::read_dir(Path::new(&dir_path)) {
        Ok(value) => value,
        Err(_) => return ptr::null_mut(),
    };
    for entry in iter {
        let entry = match entry {
            Ok(value) => value,
            Err(_) => return ptr::null_mut(),
        };

        let file_name = entry.file_name();
        let name = file_name.to_string_lossy();
        if name == "." || name == ".." {
            continue;
        }

        let is_dir = entry
            .file_type()
            .map(|file_type| file_type.is_dir())
            .unwrap_or(false);

        let mut item = CbmCompatFsDirent {
            name: [0; CBM_RS_DIRENT_NAME_MAX],
            is_dir,
            d_type: 0,
        };
        copy_dirent_name(&mut item.name, &name);
        entries.push(item);
    }

    Box::into_raw(Box::new(CbmCompatFsDir {
        entries,
        cursor: 0,
        current: CbmCompatFsDirent {
            name: [0; CBM_RS_DIRENT_NAME_MAX],
            is_dir: false,
            d_type: 0,
        },
    })) as *mut c_void
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_compat_fs_readdir(handle: *mut c_void) -> *mut CbmCompatFsDirent {
    let dir = match handle.cast::<CbmCompatFsDir>().as_mut() {
        Some(value) => value,
        None => return ptr::null_mut(),
    };

    if dir.cursor >= dir.entries.len() {
        return ptr::null_mut();
    }

    dir.current = dir.entries[dir.cursor];
    dir.cursor += 1;
    &mut dir.current
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_compat_fs_closedir(handle: *mut c_void) {
    if handle.is_null() {
        return;
    }
    let _ = Box::<CbmCompatFsDir>::from_raw(handle.cast::<CbmCompatFsDir>());
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_compat_fs_popen(
    cmd: *const c_char,
    mode: *const c_char,
) -> *mut c_void {
    if cmd.is_null() || mode.is_null() {
        return ptr::null_mut();
    }

    #[cfg(windows)]
    return unsafe { _popen(cmd, mode) };

    #[cfg(not(windows))]
    unsafe {
        popen(cmd, mode)
    }
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_compat_fs_pclose(stream: *mut c_void) -> c_int {
    if stream.is_null() {
        return -1;
    }

    #[cfg(windows)]
    return unsafe { _pclose(stream) };

    #[cfg(not(windows))]
    unsafe {
        pclose(stream)
    }
}

#[no_mangle]
pub extern "C" fn cbm_rs_compat_fs_mkdir_p(path: *const c_char, _mode: c_int) -> bool {
    let Some(dir_path) = cstr_to_path(path) else {
        return false;
    };
    std::fs::create_dir_all(dir_path).is_ok()
}

#[no_mangle]
pub extern "C" fn cbm_rs_compat_fs_unlink(path: *const c_char) -> c_int {
    let Some(path) = cstr_to_path(path) else {
        return -1;
    };
    if std::fs::remove_file(path).is_ok() {
        0
    } else {
        -1
    }
}

#[no_mangle]
pub extern "C" fn cbm_rs_compat_fs_rmdir(path: *const c_char) -> c_int {
    let Some(path) = cstr_to_path(path) else {
        return -1;
    };
    if std::fs::remove_dir(path).is_ok() {
        0
    } else {
        -1
    }
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_compat_fs_exec_no_shell(argv: *const *const c_char) -> c_int {
    if argv.is_null() {
        return -1;
    }

    let mut args: Vec<String> = Vec::new();
    let mut cursor = argv;
    loop {
        let next = unsafe { *cursor };
        if next.is_null() {
            break;
        }
        let arg = match cstr_to_string(next) {
            Some(value) => value,
            None => return -1,
        };
        args.push(arg);
        unsafe { cursor = cursor.add(1) };
    }

    if args.is_empty() {
        return -1;
    }

    let status = Command::new(&args[0]).args(&args[1..]).status();
    status.map_or(-1, |value| value.code().unwrap_or(-1))
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null，或指向 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_log_parse_level_value(
    value: *const c_char,
    current: c_int,
) -> c_int {
    foundation_log::parse_level_value(unsafe { c_bytes(value) }, current)
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null，或指向 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_log_parse_format_value(
    value: *const c_char,
    current: c_int,
) -> c_int {
    foundation_log::parse_format_value(unsafe { c_bytes(value) }, current)
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`value` 必須是
/// null，或指向 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_log_sanitize_text_atom(
    buf: *mut c_char,
    bufsize: usize,
    value: *const c_char,
) -> c_int {
    let output = foundation_log::sanitize_text_atom(unsafe { c_bytes(value) });
    unsafe { write_c_output_i32(buf, bufsize, Some(output.as_slice())) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`value` 必須是
/// null，或指向 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_log_json_string(
    buf: *mut c_char,
    bufsize: usize,
    value: *const c_char,
) -> c_int {
    let output = foundation_log::json_string(unsafe { c_bytes(value) });
    unsafe { write_c_output_i32(buf, bufsize, Some(output.as_slice())) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`path` 必須是
/// null，或指向 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_log_http_path_without_query(
    buf: *mut c_char,
    bufsize: usize,
    path: *const c_char,
) -> c_int {
    let output = foundation_log::http_path_without_query(unsafe { c_bytes(path) });
    unsafe { write_c_output_i32(buf, bufsize, Some(output.as_slice())) }
}

#[no_mangle]
pub extern "C" fn cbm_rs_log_http_status_level(status: c_int) -> c_int {
    foundation_log::http_status_level(status)
}

struct CbmRsProgressSinkRuntime {
    out: usize,
}

static CBM_RS_PROGRESS_SINK_RUNTIME: OnceLock<Mutex<CbmRsProgressSinkRuntime>> = OnceLock::new();

fn cbm_rs_progress_sink_runtime() -> &'static Mutex<CbmRsProgressSinkRuntime> {
    CBM_RS_PROGRESS_SINK_RUNTIME.get_or_init(|| Mutex::new(CbmRsProgressSinkRuntime { out: 0 }))
}

unsafe extern "C" {
    #[cfg(feature = "progress-sink-only")]
    fn cbm_log_set_sink(sink: Option<unsafe extern "C" fn(*const c_char)>);
    fn fwrite(
        ptr: *const c_void,
        size: usize,
        nmemb: usize,
        stream: *mut cli_progress_sink::CFile,
    ) -> usize;
    fn fflush(stream: *mut cli_progress_sink::CFile) -> c_int;
}

fn cbm_rs_progress_sink_write_emit(out: usize, emit: cli_progress_sink::ProgressEmit) {
    if out == 0 {
        return;
    }
    let stream = out as *mut cli_progress_sink::CFile;
    unsafe {
        if !emit.bytes.is_empty() {
            let _ = fwrite(
                emit.bytes.as_ptr().cast::<c_void>(),
                1,
                emit.bytes.len(),
                stream,
            );
        }
        if emit.flush {
            let _ = fflush(stream);
        }
    }
}

#[no_mangle]
/// # Safety
///
/// `out` 必須是 null，或指向有效的 `FILE`。
pub unsafe extern "C" fn cbm_rs_progress_sink_init(out: *mut cli_progress_sink::CFile) {
    let mut guard = match cbm_rs_progress_sink_runtime().lock() {
        Ok(guard) => guard,
        Err(poisoned) => poisoned.into_inner(),
    };
    guard.out = out as usize;
    cli_progress_sink::reset();
}

#[no_mangle]
pub extern "C" fn cbm_rs_progress_sink_fini() {
    let mut guard = match cbm_rs_progress_sink_runtime().lock() {
        Ok(guard) => guard,
        Err(poisoned) => poisoned.into_inner(),
    };
    let out = guard.out;
    let emit = cli_progress_sink::fini();
    if let Some(emit) = emit {
        cbm_rs_progress_sink_write_emit(out, emit);
    }
    guard.out = 0;
}

#[no_mangle]
/// # Safety
///
/// `line` 必須是 null，或指向有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_progress_sink_fn(line: *const c_char) {
    if line.is_null() {
        return;
    }

    let guard = match cbm_rs_progress_sink_runtime().lock() {
        Ok(guard) => guard,
        Err(poisoned) => poisoned.into_inner(),
    };
    let out = guard.out;
    if out == 0 {
        return;
    }

    let emit = cli_progress_sink::handle_line(unsafe { CStr::from_ptr(line) });
    if let Some(emit) = emit {
        cbm_rs_progress_sink_write_emit(out, emit);
    }
}

#[cfg(feature = "progress-sink-only")]
#[no_mangle]
/// # Safety
///
/// `out` 必須是 null，或指向有效的 `FILE`。
pub unsafe extern "C" fn cbm_progress_sink_init(out: *mut cli_progress_sink::CFile) {
    unsafe { cbm_rs_progress_sink_init(out) };
    unsafe { cbm_log_set_sink(Some(cbm_progress_sink_fn)) };
}

#[cfg(feature = "progress-sink-only")]
#[no_mangle]
pub extern "C" fn cbm_progress_sink_fini() {
    cbm_rs_progress_sink_fini();
    unsafe { cbm_log_set_sink(None) };
}

#[cfg(feature = "progress-sink-only")]
#[no_mangle]
/// # Safety
///
/// `line` 必須是 null，或指向有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_progress_sink_fn(line: *const c_char) {
    unsafe { cbm_rs_progress_sink_fn(line) };
}

#[no_mangle]
/// # Safety
///
/// `left` 與 `right` 必須是 null，或指向有效的 NUL 結尾 C string；null 會視為空版本。
pub unsafe extern "C" fn cbm_rs_cli_compare_versions_v1(
    left: *const c_char,
    right: *const c_char,
) -> c_int {
    cli_version::compare_versions(unsafe { c_bytes(left) }, unsafe { c_bytes(right) })
}

#[cfg(feature = "cli-version-only")]
#[no_mangle]
/// # Safety
///
/// `left` 與 `right` 必須是 null，或指向有效的 NUL 結尾 C string；null 會視為空版本。
pub unsafe extern "C" fn cbm_cli_compare_versions_v1(
    left: *const c_char,
    right: *const c_char,
) -> c_int {
    cbm_rs_cli_compare_versions_v1(left, right)
}

#[no_mangle]
/// # Safety
///
/// `pattern` 必須是 null，或指向有效的 NUL 結尾 C 字串；`buf` 若非 null，必須指向至少
/// `bufsize` bytes 的可寫緩衝區。回傳 1 表示找到 token，0 表示沒有可用 token。
pub unsafe extern "C" fn cbm_rs_cli_hook_extract_token_v1(
    pattern: *const c_char,
    buf: *mut c_char,
    bufsize: usize,
) -> c_int {
    let Some(output) = cli_hook::extract_token(unsafe { c_bytes(pattern) }, bufsize) else {
        return 0;
    };
    unsafe { write_c_output(buf, bufsize, Some(&output)) };
    1
}

#[cfg(feature = "cli-hook-token-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_cli_hook_extract_token(
    pattern: *const c_char,
    buf: *mut c_char,
    bufsize: usize,
) -> bool {
    unsafe { cbm_rs_cli_hook_extract_token_v1(pattern, buf, bufsize) != 0 }
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null，或指向 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_profile_env_enabled(value: *const c_char) -> bool {
    foundation_profile::env_enabled(unsafe { c_bytes(value) })
}

#[no_mangle]
pub extern "C" fn cbm_rs_profile_elapsed_us(
    start_sec: i64,
    start_nsec: i64,
    now_sec: i64,
    now_nsec: i64,
) -> i64 {
    foundation_profile::elapsed_us(start_sec, start_nsec, now_sec, now_nsec)
}

#[no_mangle]
pub extern "C" fn cbm_rs_profile_elapsed_ms(us: i64) -> i64 {
    foundation_profile::elapsed_ms(us)
}

#[no_mangle]
pub extern "C" fn cbm_rs_profile_rate_per_s(items: i64, us: i64) -> i64 {
    foundation_profile::rate_per_s(items, us).unwrap_or(-1)
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
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`abs_path` 必須是
/// null，或指向 NUL-terminated C string。
pub unsafe extern "C" fn cbm_rs_pipeline_project_name_from_path(
    buf: *mut c_char,
    bufsize: usize,
    abs_path: *const c_char,
) -> usize {
    let output = pipeline_fqn::project_name_from_path_bytes(unsafe { c_bytes(abs_path) });
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_is_env_var_name_v1(value: *const c_char) -> c_int {
    c_int::from(pipeline_configures::is_env_var_name(unsafe {
        c_bytes(value)
    }))
}

#[no_mangle]
/// # Safety
///
/// `key` 必須是 null，或指向有效的 NUL 結尾 C string；`norm_out` 必須是 null，
/// 或指向至少 `norm_sz` bytes 的可寫記憶體。回傳 token 數；null input 或無效
/// buffer 時回傳 0，截斷不視為錯誤。
pub unsafe extern "C" fn cbm_rs_pipeline_normalize_config_key_v1(
    key: *const c_char,
    norm_out: *mut c_char,
    norm_sz: usize,
) -> c_int {
    if norm_out.is_null() || norm_sz == 0 {
        return 0;
    }
    let Some(key) = (unsafe { c_bytes(key) }) else {
        return 0;
    };
    let (normalized, token_count) = pipeline_configures::normalize_config_key_bytes(Some(key));
    let copied = normalized.len().min(norm_sz - 1);
    if copied > 0 {
        unsafe {
            ptr::copy_nonoverlapping(normalized.as_ptr().cast::<c_char>(), norm_out, copied);
        }
    }
    unsafe {
        *norm_out.add(copied) = 0;
    }
    token_count as c_int
}

#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_has_config_extension_v1(path: *const c_char) -> c_int {
    c_int::from(pipeline_configures::has_config_extension(unsafe {
        c_bytes(path)
    }))
}

#[cfg(feature = "pipeline-configures-only")]
#[no_mangle]
/// # Safety
///
/// `value` 必須是 null，或指向有效的 NUL 結尾 C string。這是
/// `CBM_USE_RUST_PIPELINE_CONFIGURES_ONLY=1` 時直接取代 C helper 的 `pipeline.h` ABI。
pub unsafe extern "C" fn cbm_is_env_var_name(value: *const c_char) -> bool {
    unsafe { cbm_rs_pipeline_is_env_var_name_v1(value) != 0 }
}

#[cfg(feature = "pipeline-configures-only")]
#[no_mangle]
/// # Safety
///
/// `key` 必須是 null，或指向有效的 NUL 結尾 C string；`norm_out` 必須是 null，或
/// 指向至少 `norm_sz` bytes 的可寫記憶體。這是 Rust-only configures 的既有 C ABI。
pub unsafe extern "C" fn cbm_normalize_config_key(
    key: *const c_char,
    norm_out: *mut c_char,
    norm_sz: usize,
) -> c_int {
    unsafe { cbm_rs_pipeline_normalize_config_key_v1(key, norm_out, norm_sz) }
}

#[cfg(feature = "pipeline-configures-only")]
#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或指向有效的 NUL 結尾 C string。這是
/// `CBM_USE_RUST_PIPELINE_CONFIGURES_ONLY=1` 時直接取代 C helper 的 `pipeline.h` ABI。
pub unsafe extern "C" fn cbm_has_config_extension(path: *const c_char) -> bool {
    unsafe { cbm_rs_pipeline_has_config_extension_v1(path) != 0 }
}

#[no_mangle]
/// # Safety
///
/// `dirname` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_discover_should_skip_dir_v1(
    dirname: *const c_char,
    mode: c_int,
) -> c_int {
    c_int::from(discover_filter::should_skip_dir(
        unsafe { c_bytes(dirname) },
        mode,
    ))
}

#[no_mangle]
/// # Safety
///
/// `filename` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_discover_has_ignored_suffix_v1(
    filename: *const c_char,
    mode: c_int,
) -> c_int {
    c_int::from(discover_filter::has_ignored_suffix(
        unsafe { c_bytes(filename) },
        mode,
    ))
}

#[no_mangle]
/// # Safety
///
/// `filename` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_discover_should_skip_filename_v1(
    filename: *const c_char,
    mode: c_int,
) -> c_int {
    c_int::from(discover_filter::should_skip_filename(
        unsafe { c_bytes(filename) },
        mode,
    ))
}

#[no_mangle]
/// # Safety
///
/// `filename` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_discover_matches_fast_pattern_v1(
    filename: *const c_char,
    mode: c_int,
) -> c_int {
    c_int::from(discover_filter::matches_fast_pattern(
        unsafe { c_bytes(filename) },
        mode,
    ))
}

#[cfg(feature = "discover-filters-only")]
#[no_mangle]
/// # Safety
///
/// `dirname` 必須是 null，或指向有效的 NUL 結尾 C string。這是
/// `CBM_USE_RUST_DISCOVER_FILTERS=1` 時直接取代 C helper 的既有 C ABI。
pub unsafe extern "C" fn cbm_should_skip_dir(dirname: *const c_char, mode: c_int) -> bool {
    unsafe { cbm_rs_discover_should_skip_dir_v1(dirname, mode) != 0 }
}

#[cfg(feature = "discover-filters-only")]
#[no_mangle]
/// # Safety
///
/// `filename` 必須是 null，或指向有效的 NUL 結尾 C string。這是
/// `CBM_USE_RUST_DISCOVER_FILTERS=1` 時直接取代 C helper 的既有 C ABI。
pub unsafe extern "C" fn cbm_has_ignored_suffix(filename: *const c_char, mode: c_int) -> bool {
    unsafe { cbm_rs_discover_has_ignored_suffix_v1(filename, mode) != 0 }
}

#[cfg(feature = "discover-filters-only")]
#[no_mangle]
/// # Safety
///
/// `filename` 必須是 null，或指向有效的 NUL 結尾 C string。這是
/// `CBM_USE_RUST_DISCOVER_FILTERS=1` 時直接取代 C helper 的既有 C ABI。
pub unsafe extern "C" fn cbm_should_skip_filename(filename: *const c_char, mode: c_int) -> bool {
    unsafe { cbm_rs_discover_should_skip_filename_v1(filename, mode) != 0 }
}

#[cfg(feature = "discover-filters-only")]
#[no_mangle]
/// # Safety
///
/// `filename` 必須是 null，或指向有效的 NUL 結尾 C string。這是
/// `CBM_USE_RUST_DISCOVER_FILTERS=1` 時直接取代 C helper 的既有 C ABI。
pub unsafe extern "C" fn cbm_matches_fast_pattern(filename: *const c_char, mode: c_int) -> bool {
    unsafe { cbm_rs_discover_matches_fast_pattern_v1(filename, mode) != 0 }
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null，或指向有效的 NUL 結尾 C string；`list` 必須是 null，或指向
/// NULL 結尾的 C string 指標陣列。
pub unsafe extern "C" fn cbm_rs_discover_str_in_list_v1(
    value: *const c_char,
    list: *const *const c_char,
) -> c_int {
    let Some(value) = (unsafe { c_bytes(value) }) else {
        return 0;
    };
    if list.is_null() {
        return 0;
    }

    let mut index = 0;
    loop {
        let item_ptr = unsafe { *list.add(index) };
        if item_ptr.is_null() {
            return 0;
        }
        if let Some(item) = unsafe { c_bytes(item_ptr) } {
            if discover_filter::str_in_list(value, &[item]) {
                return 1;
            }
        }
        index += 1;
    }
}

#[no_mangle]
/// # Safety
///
/// `value` 與 `suffix` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_discover_ends_with_v1(
    value: *const c_char,
    suffix: *const c_char,
) -> c_int {
    c_int::from(discover_filter::ends_with(
        unsafe { c_bytes(value) },
        unsafe { c_bytes(suffix) },
    ))
}

#[no_mangle]
/// # Safety
///
/// `value` 與 `substring` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_discover_str_contains_v1(
    value: *const c_char,
    substring: *const c_char,
) -> c_int {
    c_int::from(discover_filter::str_contains(
        unsafe { c_bytes(value) },
        unsafe { c_bytes(substring) },
    ))
}

#[no_mangle]
/// # Safety
///
/// `left` 與 `right` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_discover_ascii_ieq_v1(
    left: *const c_char,
    right: *const c_char,
) -> c_int {
    c_int::from(discover_filter::ascii_ieq(
        unsafe { c_bytes(left) },
        unsafe { c_bytes(right) },
    ))
}

#[cfg(feature = "discover-string-helpers-only")]
#[no_mangle]
/// # Safety
///
/// `value` 必須是 null，或指向有效的 NUL 結尾 C string；`list` 必須是 null，或指向
/// NULL 結尾的 C string 指標陣列。
pub unsafe extern "C" fn cbm_discover_str_in_list(
    value: *const c_char,
    list: *const *const c_char,
) -> bool {
    unsafe { cbm_rs_discover_str_in_list_v1(value, list) != 0 }
}

#[cfg(feature = "discover-string-helpers-only")]
#[no_mangle]
/// # Safety
///
/// `value` 與 `suffix` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_discover_ends_with(
    value: *const c_char,
    suffix: *const c_char,
) -> bool {
    unsafe { cbm_rs_discover_ends_with_v1(value, suffix) != 0 }
}

#[cfg(feature = "discover-string-helpers-only")]
#[no_mangle]
/// # Safety
///
/// `value` 與 `substring` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_discover_str_contains(
    value: *const c_char,
    substring: *const c_char,
) -> bool {
    unsafe { cbm_rs_discover_str_contains_v1(value, substring) != 0 }
}

#[cfg(feature = "discover-string-helpers-only")]
#[no_mangle]
/// # Safety
///
/// `left` 與 `right` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_discover_ascii_ieq(left: *const c_char, right: *const c_char) -> bool {
    unsafe { cbm_rs_discover_ascii_ieq_v1(left, right) != 0 }
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null，或指向有效的 NUL 結尾 C 字串。null 與空字串回 0。
pub unsafe extern "C" fn cbm_rs_discover_has_trailing_sep_v1(value: *const c_char) -> c_int {
    c_int::from(discover_filter::has_trailing_sep(unsafe { c_bytes(value) }))
}

#[cfg(feature = "discover-trailing-sep-only")]
#[no_mangle]
/// # Safety
///
/// 與 `cbm_rs_discover_has_trailing_sep_v1` 相同的 NULL／NUL 字串契約；
/// direct-only 時匯出同名公開 bridge symbol。
pub unsafe extern "C" fn cbm_discover_has_trailing_sep(value: *const c_char) -> bool {
    unsafe { cbm_rs_discover_has_trailing_sep_v1(value) != 0 }
}

#[no_mangle]
/// # Safety
///
/// `base` 與 `relative` 必須是 null，或指向有效的 NUL 結尾 C 字串；`buf` 若非 null，必須
/// 指向至少 `bufsize` bytes 的可寫緩衝區。
pub unsafe extern "C" fn cbm_rs_discover_path_join_v1(
    base: *const c_char,
    relative: *const c_char,
    buf: *mut c_char,
    bufsize: usize,
) {
    if buf.is_null() || bufsize == 0 {
        return;
    }
    let output = discover_filter::path_join(
        unsafe { c_bytes(base) },
        unsafe { c_bytes(relative) },
        bufsize,
    );
    unsafe { write_c_output(buf, bufsize, Some(&output)) };
}

#[cfg(feature = "discover-path-join-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_discover_path_join(
    base: *const c_char,
    relative: *const c_char,
    buf: *mut c_char,
    bufsize: usize,
) {
    unsafe { cbm_rs_discover_path_join_v1(base, relative, buf, bufsize) }
}

#[no_mangle]
pub extern "C" fn cbm_rs_watcher_poll_interval_ms_v1(file_count: c_int) -> c_int {
    watcher_runtime::poll_interval_ms(file_count)
}

#[cfg(feature = "watcher-poll-interval-only")]
#[no_mangle]
pub extern "C" fn cbm_watcher_poll_interval_ms(file_count: c_int) -> c_int {
    cbm_rs_watcher_poll_interval_ms_v1(file_count)
}

#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或指向有效的 NUL 結尾 C 字串；Rust 不保存該指標。
pub unsafe extern "C" fn cbm_rs_git_path_is_absolute_v1(path: *const c_char) -> c_int {
    c_int::from(git_runtime::path_is_absolute(unsafe { c_bytes(path) }))
}

#[cfg(feature = "git-path-absolute-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_git_path_is_absolute(path: *const c_char) -> bool {
    unsafe { cbm_rs_git_path_is_absolute_v1(path) != 0 }
}

#[no_mangle]
/// # Safety
///
/// `source` 必須是 null，或指向有效的 NUL 結尾 C 字串；Rust 不保存該指標。若 escape 後的
/// 內容加上 NUL 無法以 `int` buffer size 表示，回傳 -1。
pub unsafe extern "C" fn cbm_rs_git_json_escaped_len_v1(source: *const c_char) -> c_int {
    git_runtime::json_escaped_len(unsafe { c_bytes(source) })
}

#[cfg(feature = "git-json-escaped-len-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_git_json_escaped_len(source: *const c_char) -> c_int {
    unsafe { cbm_rs_git_json_escaped_len_v1(source) }
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null，或指向可寫入且以 NUL 結尾的 C 字串；Rust 只修改尾端 CR/LF。
pub unsafe extern "C" fn cbm_rs_git_trim_newlines_v1(value: *mut c_char) {
    if value.is_null() {
        return;
    }
    let len = unsafe { CStr::from_ptr(value) }.to_bytes().len();
    let bytes = unsafe { std::slice::from_raw_parts_mut(value.cast::<u8>(), len) };
    git_runtime::trim_newlines(bytes);
}

#[cfg(feature = "git-trim-newlines-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_git_trim_newlines(value: *mut c_char) {
    unsafe { cbm_rs_git_trim_newlines_v1(value) }
}

#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或指向有效的 NUL 結尾 C 字串；`buf` 若非 null，必須指向至少
/// `bufsize` bytes 的可寫緩衝區，且不得與 `path` 重疊。Rust 不保存任何輸入指標。
pub unsafe extern "C" fn cbm_rs_log_copy_path_without_query_v1(
    path: *const c_char,
    buf: *mut c_char,
    bufsize: usize,
) {
    unsafe { copy_c_path_without_query_bounded(path, buf, bufsize) };
}

#[cfg(feature = "log-copy-path-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_log_copy_path_without_query(
    path: *const c_char,
    buf: *mut c_char,
    bufsize: usize,
) {
    unsafe { cbm_rs_log_copy_path_without_query_v1(path, buf, bufsize) }
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_is_dockerfile_v1(name: *const c_char) -> c_int {
    c_int::from(pipeline_infrascan::is_dockerfile(unsafe { c_bytes(name) }))
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_is_compose_file_v1(name: *const c_char) -> c_int {
    c_int::from(pipeline_infrascan::is_compose_file(unsafe {
        c_bytes(name)
    }))
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_is_env_file_v1(name: *const c_char) -> c_int {
    c_int::from(pipeline_infrascan::is_env_file(unsafe { c_bytes(name) }))
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_is_cloudbuild_file_v1(name: *const c_char) -> c_int {
    c_int::from(pipeline_infrascan::is_cloudbuild_file(unsafe {
        c_bytes(name)
    }))
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_is_kustomize_file_v1(name: *const c_char) -> c_int {
    c_int::from(pipeline_infrascan::is_kustomize_file(unsafe {
        c_bytes(name)
    }))
}

#[no_mangle]
/// # Safety
///
/// `name` 與 `ext` 必須是 null，或指向有效的 NUL 結尾 C string。`name` 保留給 C ABI
/// 對齊，但此 helper 只依 `ext` 進行純字節級判斷。
pub unsafe extern "C" fn cbm_rs_pipeline_is_shell_script_v1(
    name: *const c_char,
    ext: *const c_char,
) -> c_int {
    c_int::from(pipeline_infrascan::is_shell_script(
        unsafe { c_bytes(name) },
        unsafe { c_bytes(ext) },
    ))
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向有效 NUL 結尾 C 字串；Rust 不保存該指標。
pub unsafe extern "C" fn cbm_rs_pipeline_is_helm_chart_file_v1(name: *const c_char) -> c_int {
    c_int::from(pipeline_infrascan::is_helm_chart_file(unsafe {
        c_bytes(name)
    }))
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向有效 NUL 結尾 C 字串；Rust 不保存該指標。
pub unsafe extern "C" fn cbm_rs_pipeline_is_gomod_file_v1(name: *const c_char) -> c_int {
    c_int::from(pipeline_infrascan::is_gomod_file(unsafe { c_bytes(name) }))
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向有效 NUL 結尾 C 字串；Rust 不保存該指標。
pub unsafe extern "C" fn cbm_rs_pipeline_is_requirements_file_v1(name: *const c_char) -> c_int {
    c_int::from(pipeline_infrascan::is_requirements_file(unsafe {
        c_bytes(name)
    }))
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_clean_json_brackets_v1(
    input: *const c_char,
    output: *mut c_char,
    output_size: usize,
) {
    if input.is_null() || output.is_null() || output_size == 0 {
        return;
    }
    let Some(value) =
        pipeline_infrascan::clean_json_brackets(unsafe { c_bytes(input) }, output_size)
    else {
        return;
    };
    let length = value.len().min(output_size - 1);
    unsafe {
        std::ptr::copy_nonoverlapping(value.as_ptr().cast::<c_char>(), output, length);
        *output.add(length) = 0;
    }
}

#[cfg(feature = "pipeline-infrascan-json-only")]
#[no_mangle]
/// # Safety
///
/// 非 NULL 的 `input` 必須為 NUL 結尾字串；當 `output` 非 NULL 且
/// `output_size` 大於零時，必須指向至少 `output_size` bytes 的可寫緩衝區。
pub unsafe extern "C" fn cbm_clean_json_brackets(
    input: *const c_char,
    output: *mut c_char,
    output_size: usize,
) {
    unsafe { cbm_rs_pipeline_clean_json_brackets_v1(input, output, output_size) }
}

#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_is_trackable_file_v1(path: *const c_char) -> c_int {
    c_int::from(pipeline_githistory::is_trackable_file(unsafe {
        c_bytes(path)
    }))
}

#[cfg(feature = "pipeline-githistory-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同的 nullable NUL 字串契約；direct-only 匯出同名公開 bridge。
pub unsafe extern "C" fn cbm_is_trackable_file(path: *const c_char) -> bool {
    pipeline_githistory::is_trackable_file(unsafe { c_bytes(path) })
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向有效的 NUL 結尾 C string；`out_start` 與 `out_count`
/// 必須指向可寫入的 `int` 儲存空間。
pub unsafe extern "C" fn cbm_rs_pipeline_parse_range_v1(
    input: *const c_char,
    out_start: *mut c_int,
    out_count: *mut c_int,
) {
    if out_start.is_null() || out_count.is_null() {
        return;
    }
    let (start, count) = pipeline_gitdiff::parse_range(unsafe { c_bytes(input) });
    unsafe {
        *out_start = start;
        *out_count = count;
    }
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向有效的 NUL 結尾 C string；`out` 必須是 null，或指向至少
/// `max_out` 個 `cbm_changed_file_t` 等價儲存空間。輸出由呼叫端擁有。
pub unsafe extern "C" fn cbm_rs_pipeline_parse_name_status_v1(
    input: *const c_char,
    out: *mut CbmRsChangedFileV1,
    max_out: c_int,
) -> c_int {
    if out.is_null() || max_out <= 0 {
        return 0;
    }
    let parsed = pipeline_gitdiff::parse_name_status(unsafe { c_bytes(input) }, max_out as usize);
    for (index, entry) in parsed.iter().enumerate() {
        let dst = unsafe { &mut *out.add(index) };
        dst.status = [0; 4];
        dst.path = [0; 512];
        dst.old_path = [0; 512];
        dst.status[0] = entry.status as c_char;
        copy_c_bytes(&mut dst.path, &entry.path);
        copy_c_bytes(&mut dst.old_path, &entry.old_path);
    }
    parsed.len() as c_int
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向有效的 NUL 結尾 C string；`out` 必須是 null，或指向至少
/// `max_out` 個 `cbm_changed_hunk_t` 等價儲存空間。輸出由呼叫端擁有。
pub unsafe extern "C" fn cbm_rs_pipeline_parse_hunks_v1(
    input: *const c_char,
    out: *mut CbmRsChangedHunkV1,
    max_out: c_int,
) -> c_int {
    if out.is_null() || max_out <= 0 {
        return 0;
    }
    let parsed = pipeline_gitdiff::parse_hunks(unsafe { c_bytes(input) }, max_out as usize);
    for (index, entry) in parsed.iter().enumerate() {
        let dst = unsafe { &mut *out.add(index) };
        dst.path = [0; 512];
        copy_c_bytes(&mut dst.path, &entry.path);
        dst.start_line = entry.start_line;
        dst.end_line = entry.end_line;
    }
    parsed.len() as c_int
}

#[no_mangle]
/// # Safety
///
/// 此 ABI 只接受 C enum 的整數值，不保存任何呼叫端指標。
pub unsafe extern "C" fn cbm_rs_pipeline_is_module_dir_v1(language: c_int) -> c_int {
    c_int::from(pipeline_module::is_module_dir(language))
}

fn copy_c_bytes<const N: usize>(dst: &mut [c_char; N], src: &[u8]) {
    for (out, input) in dst.iter_mut().zip(src.iter().copied()) {
        *out = input as c_char;
    }
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向至少 `len` bytes 的可讀記憶體；null 或負
/// `len` 會回傳 `-1`。此 API 只固定 Cypher lexer token metadata，不建立 AST。
pub unsafe extern "C" fn cbm_rs_cypher_lex_token_count_v1(input: *const u8, len: c_int) -> c_int {
    let Some(input) = (unsafe { c_bytes_with_len(input, len) }) else {
        return -1;
    };
    let count = cypher::lex(input).len();
    if count > c_int::MAX as usize {
        return -1;
    }
    count as c_int
}

#[no_mangle]
/// # Safety
///
/// `input` contract 同 `cbm_rs_cypher_lex_token_count_v1`。`out` 必須非 null，
/// 且指向至少 `cap` 個 `CbmRsCypherTokenV1` 的可寫陣列；cap 不足時回傳 `-1`。
pub unsafe extern "C" fn cbm_rs_cypher_lex_tokens_v1(
    input: *const u8,
    len: c_int,
    out: *mut CbmRsCypherTokenV1,
    cap: c_int,
) -> c_int {
    if out.is_null() || cap < 0 {
        return -1;
    }
    let Some(input) = (unsafe { c_bytes_with_len(input, len) }) else {
        return -1;
    };
    let tokens = cypher::lex(input);
    if cap < tokens.len() as c_int {
        return -1;
    }
    let out = unsafe { slice::from_raw_parts_mut(out, tokens.len()) };
    for (dst, token) in out.iter_mut().zip(tokens) {
        dst.kind = token.kind;
        dst.pos = token.pos;
        dst.text_len = token.text.len().min(c_int::MAX as usize) as c_int;
        dst.reserved0 = 0;
    }
    out.len() as c_int
}

#[no_mangle]
/// # Safety
///
/// `input` contract 同 `cbm_rs_cypher_lex_token_count_v1`。`buf` 必須是 null，
/// 或指向 `bufsize` bytes 的可寫記憶體；回傳完整 summary 長度，短 buffer 會
/// NUL 結尾截斷。此 API 僅供 test-only parity fixture 使用。
pub unsafe extern "C" fn cbm_rs_cypher_lex_summary_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const u8,
    len: c_int,
) -> usize {
    let output = unsafe { c_bytes_with_len(input, len) }.map(cypher::summary);
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `input` contract 同 `cbm_rs_cypher_lex_token_count_v1`。`buf` 必須是 null，
/// 或指向 `bufsize` bytes 的可寫記憶體；回傳完整 normalized AST summary 長度，
/// 短 buffer 會 NUL 結尾截斷。null input、負 len 或 parse failure 回傳 `SIZE_MAX`。
/// 此 API 僅供 test-only parity fixture 使用，不執行 query，也不接 production opt-in。
pub unsafe extern "C" fn cbm_rs_cypher_parse_summary_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const u8,
    len: c_int,
) -> usize {
    let output = unsafe { c_bytes_with_len(input, len) }.and_then(cypher::parse_summary);
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// `src/cypher/cypher.c` 單字元 token classifier ABI。輸入必須在 `0..=255`，
/// 其餘值回傳 `-1`；已辨識字元回傳既有 token enum，未知 byte 回傳 `TOK_EOF`。
/// 不借用、不保存也不回傳 C 指標。
pub extern "C" fn cbm_rs_cypher_single_char_kind_v1(input: c_int) -> c_int {
    if !(0..=u8::MAX as c_int).contains(&input) {
        return -1;
    }
    cypher::single_char_kind_or_eof(input as u8) as c_int
}

#[no_mangle]
/// `src/cypher/cypher.c` 雙字元 token classifier ABI。輸入必須各自在 `0..=255`，
/// 其餘值回傳 `-1`；已辨識 pair 回傳既有 token enum，未知 pair 回傳 `TOK_EOF`。
/// 不借用、不保存也不回傳 C 指標。
pub extern "C" fn cbm_rs_cypher_two_char_kind_v1(first: c_int, second: c_int) -> c_int {
    if !(0..=u8::MAX as c_int).contains(&first) || !(0..=u8::MAX as c_int).contains(&second) {
        return -1;
    }
    cypher::two_char_kind_or_eof(first as u8, second as u8) as c_int
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向 NUL-terminated C string。回傳符合的 scalar function
/// canonical 名稱索引（0..N-1，對齊 C `scalar_func_canonical` 的 `names` 順序），
/// 或 `-1` 表示不符合。ASCII 大小寫不敏感，對齊 C `cyp_ci_eq`。
pub unsafe extern "C" fn cbm_rs_cypher_scalar_func_index_v1(input: *const c_char) -> c_int {
    match cypher::scalar_func_index(unsafe { c_bytes(input) }) {
        Some(idx) => idx as c_int,
        None => -1,
    }
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向 NUL-terminated C string。回傳符合的 multi-argument
/// function canonical 名稱索引（0..N-1，對齊 C `multiarg_func_canonical` 的
/// `names` 順序），或 `-1` 表示不符合。ASCII 大小寫不敏感，對齊 C `cyp_ci_eq`。
pub unsafe extern "C" fn cbm_rs_cypher_multiarg_func_index_v1(input: *const c_char) -> c_int {
    match cypher::multiarg_func_index(unsafe { c_bytes(input) }) {
        Some(idx) => idx as c_int,
        None => -1,
    }
}

#[no_mangle]
/// `src/cypher/cypher.c` `agg_func_name` 對應的 aggregate token 名稱索引；
/// 未知 token 回傳 -1。此 ABI 不解析 query，不執行 aggregation。
pub extern "C" fn cbm_rs_cypher_aggregate_func_index_v1(token_kind: c_int) -> c_int {
    match cypher::aggregate_func_index(token_kind) {
        Some(idx) => idx as c_int,
        None => -1,
    }
}

#[no_mangle]
/// `src/cypher/cypher.c` `str_func_name` 對應的 string function token 名稱索引；
/// 未知 token 回傳 -1。此 ABI 不解析 query，不執行 scalar/string function evaluator。
pub extern "C" fn cbm_rs_cypher_string_func_index_v1(token_kind: c_int) -> c_int {
    match cypher::string_func_index(token_kind) {
        Some(idx) => idx as c_int,
        None => -1,
    }
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向至少 `len` bytes 的可讀記憶體；`buf` 必須是
/// null，或指向 `bufsize` bytes 的可寫記憶體。回傳 JSON-RPC request envelope
/// summary 的完整長度；null input、負 len、非 object root 或缺少 string method
/// 回傳 `SIZE_MAX`。此 API 僅供 MCP JSON-RPC codec test-only parity 使用。
pub unsafe extern "C" fn cbm_rs_mcp_jsonrpc_request_summary_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const u8,
    len: c_int,
) -> usize {
    let output = unsafe { c_bytes_with_len(input, len) }.and_then(mcp::jsonrpc_request_summary);
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向 NUL-terminated C string。回傳 `src/mcp/mcp.c`
/// `TOOLS[]` 中符合 tool name 的順序索引，未知或 null 回傳 -1。
pub unsafe extern "C" fn cbm_rs_mcp_tool_index_v1(name: *const c_char) -> c_int {
    match mcp::tool_index(unsafe { c_bytes(name) }) {
        Some(idx) => idx as c_int,
        None => -1,
    }
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向 NUL-terminated C string。回傳 C
/// `cbm_mcp_handle_tool()` dispatch chain 對應的 `TOOLS[]` 順序索引；
/// `trace_call_path` alias 會對應到 `trace_path`，未知或 null 回傳 -1。
pub unsafe extern "C" fn cbm_rs_mcp_tool_dispatch_index_v1(name: *const c_char) -> c_int {
    match mcp::tool_dispatch_index(unsafe { c_bytes(name) }) {
        Some(idx) => idx as c_int,
        None => -1,
    }
}

#[no_mangle]
pub extern "C" fn cbm_rs_mcp_tool_count_v1() -> c_int {
    mcp::tool_count() as c_int
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。成功時回傳
/// `src/mcp/mcp.c` `TOOLS[]` 中指定 index 的 tool name 完整 byte 長度；
/// index 超出範圍時回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_tool_name_v1(
    buf: *mut c_char,
    bufsize: usize,
    index: c_int,
) -> usize {
    let output = mcp::tool_name(i64::from(index));
    unsafe { write_c_output(buf, bufsize, output) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。成功時回傳
/// `src/mcp/mcp.c` `TOOLS[]` 中指定 index 的 tool title 完整 byte 長度；
/// index 超出範圍時回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_tool_title_v1(
    buf: *mut c_char,
    bufsize: usize,
    index: c_int,
) -> usize {
    let output = mcp::tool_title(i64::from(index));
    unsafe { write_c_output(buf, bufsize, output) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。成功時回傳
/// `src/mcp/mcp.c` `TOOLS[]` 中指定 index 的 tool description 完整 byte 長度；
/// index 超出範圍時回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_tool_description_v1(
    buf: *mut c_char,
    bufsize: usize,
    index: c_int,
) -> usize {
    let output = mcp::tool_description(i64::from(index));
    unsafe { write_c_output(buf, bufsize, output) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。成功時回傳
/// `src/mcp/mcp.c` `TOOLS[]` 中指定 index 的 input schema JSON 完整 byte 長度；
/// index 超出範圍時回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_tool_input_schema_v1(
    buf: *mut c_char,
    bufsize: usize,
    index: c_int,
) -> usize {
    let output = mcp::tool_input_schema(i64::from(index));
    unsafe { write_c_output(buf, bufsize, output) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。成功時回傳
/// MCP tools/list output schema JSON 的完整 byte 長度；短 buffer 會截斷但
/// NUL-terminate。
pub unsafe extern "C" fn cbm_rs_mcp_tool_output_schema_v1(
    buf: *mut c_char,
    bufsize: usize,
) -> usize {
    let output = Some(mcp::tool_output_schema());
    unsafe { write_c_output(buf, bufsize, output) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。成功時回傳
/// MCP tools/list result JSON 的完整 byte 長度；短 buffer 會截斷但
/// NUL-terminate。此 ABI 只產生 result object，不包含 JSON-RPC response wrapper
/// 或 Content-Length framing。
pub unsafe extern "C" fn cbm_rs_mcp_tools_list_json_v1(
    buf: *mut c_char,
    bufsize: usize,
    offset: c_int,
    limit: c_int,
    include_next_cursor: c_int,
) -> usize {
    let output = mcp::tools_list_json(
        i64::from(offset),
        i64::from(limit),
        include_next_cursor != 0,
    );
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `line` 必須是 null，或指向 NUL-terminated header line。若精準以
/// `Content-Length:` 開頭回傳 1，否則回傳 0。這只固定 server loop 的
/// framed-message classifier，不解析 body 長度。
pub unsafe extern "C" fn cbm_rs_mcp_content_length_header_matches_v1(line: *const c_char) -> c_int {
    if mcp::content_length_header_matches(unsafe { c_bytes(line) }) {
        1
    } else {
        0
    }
}

#[no_mangle]
/// # Safety
///
/// `line` 必須是 null，或指向 NUL-terminated header line。回傳解析後值；若無法解析
/// 解析 header 或內容非正整數，回傳 `-1`。
pub unsafe extern "C" fn cbm_rs_mcp_content_length_raw_v1(line: *const c_char) -> i64 {
    mcp::content_length_raw_value(unsafe { c_bytes(line) })
}

#[no_mangle]
/// # Safety
///
/// `line` 必須是 null，或指向 NUL-terminated header line。回傳可接受的
/// Content-Length body 長度；不合法、非正數或超過 `max_len` 時回 0。
pub unsafe extern "C" fn cbm_rs_mcp_content_length_v1(
    line: *const c_char,
    max_len: c_int,
) -> c_int {
    let input = unsafe { c_bytes(line) };
    let parsed = mcp::content_length_value(input, i64::from(max_len));
    if parsed > i64::from(c_int::MAX) {
        0
    } else {
        parsed as c_int
    }
}

#[no_mangle]
/// # Safety
///
/// `line` 必須是 null，或指向 NUL-terminated header line。若移除尾端
/// CR/LF 後為空字串，回傳 1；其他情況回傳 0。
pub unsafe extern "C" fn cbm_rs_mcp_content_length_header_is_blank_v1(
    line: *const c_char,
) -> c_int {
    if mcp::content_length_header_is_blank(unsafe { c_bytes(line) }) {
        1
    } else {
        0
    }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`resp` 必須是
/// null，或指向 NUL-terminated response JSON。成功時回傳完整
/// `Content-Length: ...\r\n\r\n...` frame byte 長度；短 buffer 會截斷但
/// NUL-terminate。`resp == NULL` 時回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_content_length_response_v1(
    buf: *mut c_char,
    bufsize: usize,
    resp: *const c_char,
) -> usize {
    let framed = mcp::content_length_response(unsafe { c_bytes(resp) });
    unsafe { write_c_output(buf, bufsize, framed.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`uri` 必須是
/// null，或指向 NUL-terminated file URI。成功時寫入解析出的 raw path 並回傳
/// 完整 path byte 長度；短 buffer 會截斷但 NUL-terminate。invalid URI 回傳
/// `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_parse_file_uri_v1(
    buf: *mut c_char,
    bufsize: usize,
    uri: *const c_char,
) -> usize {
    unsafe { write_c_output(buf, bufsize, mcp::parse_file_uri(c_bytes(uri))) }
}

#[no_mangle]
/// # Safety
///
/// `method` 必須是 null，或指向 NUL-terminated JSON-RPC method string。回傳
/// C dispatch 使用的 stable method kind enum；未知或 null 回傳 0。
pub unsafe extern "C" fn cbm_rs_mcp_method_kind_v1(method: *const c_char) -> c_int {
    mcp::method_kind(unsafe { c_bytes(method) }) as c_int
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。成功時寫入
/// fixed JSON-RPC Method not found error object 並回傳完整 byte 長度；短 buffer
/// 會截斷但 NUL-terminate。
pub unsafe extern "C" fn cbm_rs_mcp_method_not_found_error_v1(
    buf: *mut c_char,
    bufsize: usize,
) -> usize {
    unsafe { write_c_output(buf, bufsize, Some(mcp::method_not_found_error())) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。成功時寫入
/// fixed JSON-RPC parse error message 並回傳完整 byte 長度；短 buffer
/// 會截斷但 NUL-terminate。
pub unsafe extern "C" fn cbm_rs_mcp_parse_error_message_v1(
    buf: *mut c_char,
    bufsize: usize,
) -> usize {
    unsafe { write_c_output(buf, bufsize, Some(mcp::parse_error_message())) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。成功時寫入
/// fixed MCP ping result object 並回傳完整 byte 長度；短 buffer 會截斷但
/// NUL-terminate。
pub unsafe extern "C" fn cbm_rs_mcp_ping_result_v1(buf: *mut c_char, bufsize: usize) -> usize {
    unsafe { write_c_output(buf, bufsize, Some(mcp::ping_result())) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。成功時寫入
/// fixed `tools/call` default arguments object 並回傳完整 byte 長度；短 buffer
/// 會截斷但 NUL-terminate。
pub unsafe extern "C" fn cbm_rs_mcp_tools_call_default_arguments_v1(
    buf: *mut c_char,
    bufsize: usize,
) -> usize {
    unsafe { write_c_output(buf, bufsize, Some(mcp::tools_call_default_arguments())) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。成功時寫入
/// fixed missing-tool-name error text 並回傳完整 byte 長度；短 buffer
/// 會截斷但 NUL-terminate。
pub unsafe extern "C" fn cbm_rs_mcp_missing_tool_name_message_v1(
    buf: *mut c_char,
    bufsize: usize,
) -> usize {
    unsafe { write_c_output(buf, bufsize, Some(mcp::missing_tool_name_message())) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。成功時寫入
/// fixed missing-project JSON error object 並回傳完整 byte 長度；短 buffer
/// 會截斷但 NUL-terminate。
pub unsafe extern "C" fn cbm_rs_mcp_missing_project_error_v1(
    buf: *mut c_char,
    bufsize: usize,
) -> usize {
    unsafe { write_c_output(buf, bufsize, Some(mcp::missing_project_error())) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。成功時寫入
/// fixed project-not-found text 並回傳完整 byte 長度；短 buffer 會截斷但
/// NUL-terminate。
pub unsafe extern "C" fn cbm_rs_mcp_project_not_found_message_v1(
    buf: *mut c_char,
    bufsize: usize,
) -> usize {
    unsafe { write_c_output(buf, bufsize, Some(mcp::project_not_found_message())) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`reason` 與
/// `projects_csv` 必須是 null，或指向 NUL-terminated C string。回傳值對齊
/// C `build_project_list_error()`：當 `count > 0` 時輸出包含
/// `available_projects` 與 `count` 的 JSON；其餘情況輸出「尚未索引」hint。
/// `reason == NULL` 或 (`count > 0` 且 `projects_csv == NULL`) 回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_project_list_error_v1(
    buf: *mut c_char,
    bufsize: usize,
    reason: *const c_char,
    projects_csv: *const c_char,
    count: c_int,
) -> usize {
    let output = mcp::project_list_error(
        unsafe { c_bytes(reason) },
        unsafe { c_bytes(projects_csv) },
        count,
    );
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`tool_name`
/// 必須是 null，或指向 NUL-terminated tool name。成功時寫入
/// `unknown tool: <name>` 並回傳完整 byte 長度；短 buffer 會截斷但
/// NUL-terminate。`tool_name == NULL` 回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_unknown_tool_message_v1(
    buf: *mut c_char,
    bufsize: usize,
    tool_name: *const c_char,
) -> usize {
    let message = mcp::unknown_tool_message(unsafe { c_bytes(tool_name) });
    unsafe { write_c_output(buf, bufsize, message.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `out` 必須非 null，且指向可寫的 `CbmRsMcpToolsPageBoundsV1`。成功回傳 0，
/// 並依 C `cbm_mcp_tools_list_range()` clamp 規則寫入 tools/list page bounds；
/// out 為 null 時回傳 -1。
pub unsafe extern "C" fn cbm_rs_mcp_tools_page_bounds_v1(
    offset: c_int,
    limit: c_int,
    include_next_cursor: c_int,
    tool_count: c_int,
    out: *mut CbmRsMcpToolsPageBoundsV1,
) -> c_int {
    if out.is_null() {
        return -1;
    }
    let bounds = mcp::tools_page_bounds(
        i64::from(offset),
        i64::from(limit),
        include_next_cursor != 0,
        i64::from(tool_count),
    );
    unsafe {
        *out = CbmRsMcpToolsPageBoundsV1 {
            start: bounds.start as c_int,
            end: bounds.end as c_int,
            has_next: c_int::from(bounds.has_next),
            next_cursor: bounds.next_cursor as c_int,
        };
    }
    0
}

#[no_mangle]
/// # Safety
///
/// `input` 可為 null，或指向至少 `len` bytes 的可讀記憶體；`buf` 可為 null，
/// 或指向 `bufsize` bytes 的可寫記憶體。成功永遠回傳選定 protocol version 的
/// 完整 byte 長度；null input、負 len、invalid JSON、root 非 object、缺少
/// `protocolVersion`、非 string 或 unsupported string 都 fallback 到最新支援版本。
pub unsafe extern "C" fn cbm_rs_mcp_initialize_protocol_version_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const u8,
    len: c_int,
) -> usize {
    let output = mcp::initialize_protocol_version(unsafe { c_bytes_with_len(input, len) });
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`input` 必須是
/// null，或指向至少 `len` bytes 的 initialize params JSON。成功時回傳完整
/// initialize result JSON byte 長度；短 buffer 會截斷但 NUL-terminate。
pub unsafe extern "C" fn cbm_rs_mcp_initialize_response_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const u8,
    len: c_int,
) -> usize {
    let output = mcp::initialize_response(unsafe { c_bytes_with_len(input, len) });
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向至少 `len` bytes 的可讀記憶體；`buf` 必須是
/// null，或指向 `bufsize` bytes 的可寫記憶體。成功時回傳 `tools/call`
/// params.name 的完整 byte 長度；null input、負 len、invalid JSON、root 非 object、
/// 缺少 `name` 或首個 `name` 非 string 時回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_tools_call_name_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const u8,
    len: c_int,
) -> usize {
    let output = unsafe { c_bytes_with_len(input, len) }.and_then(mcp::tools_call_name);
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向至少 `len` bytes 的可讀記憶體；`buf` 必須是
/// null，或指向 `bufsize` bytes 的可寫記憶體。成功時回傳 `tools/call`
/// params.arguments JSON value 的完整 byte 長度；缺少 `arguments` 時回傳 `{}`；
/// null input、負 len、invalid JSON 或 root 非 object 時回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_tools_call_arguments_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const u8,
    len: c_int,
) -> usize {
    let output = unsafe { c_bytes_with_len(input, len) }.and_then(mcp::tools_call_arguments);
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。`input` 必須是
/// null，或指向至少 `len` bytes 的 args JSON。`key` 必須是 null，或指向
/// NUL-terminated C string。成功時回傳指定 string value 的完整 byte 長度；
/// key 不存在、非 string、JSON 不合法或參數無效時回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_get_string_arg_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const u8,
    len: c_int,
    key: *const c_char,
) -> usize {
    let output = unsafe { c_bytes_with_len(input, len) }
        .zip(unsafe { c_bytes(key) })
        .and_then(|(input, key)| mcp::string_arg(input, key));
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向至少 `len` bytes 的 args JSON。`key` 必須是
/// null，或指向 NUL-terminated C string。成功時回傳指定 int value；key
/// 不存在、非 int、JSON 不合法或參數無效時回傳 `default_value`。
pub unsafe extern "C" fn cbm_rs_mcp_get_int_arg_v1(
    input: *const u8,
    len: c_int,
    key: *const c_char,
    default_value: c_int,
) -> c_int {
    unsafe { c_bytes_with_len(input, len) }
        .zip(unsafe { c_bytes(key) })
        .map_or(default_value, |(input, key)| {
            mcp::int_arg(input, key, default_value)
        })
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向至少 `len` bytes 的 args JSON。`key` 必須是
/// null，或指向 NUL-terminated C string。成功且 value 為 bool true 時回傳 1；
/// false、key 不存在、非 bool、JSON 不合法或參數無效時回傳 0。
pub unsafe extern "C" fn cbm_rs_mcp_get_bool_arg_v1(
    input: *const u8,
    len: c_int,
    key: *const c_char,
) -> c_int {
    let value = unsafe { c_bytes_with_len(input, len) }
        .zip(unsafe { c_bytes(key) })
        .is_some_and(|(input, key)| mcp::bool_arg(input, key));
    c_int::from(value)
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向 NUL-terminated C string。回傳 1 表示 value
/// 只包含 ASCII uppercase letter / underscore 且長度不超過 64 bytes；null、
/// 過長或其他 byte 回傳 0。
pub unsafe extern "C" fn cbm_rs_mcp_edge_type_valid_v1(input: *const c_char) -> c_int {
    c_int::from(mcp::edge_type_valid(unsafe { c_bytes(input) }))
}

#[cfg(feature = "mcp-edge-type-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同的 nullable NUL 字串契約；direct-only 匯出同名公開 bridge。
pub unsafe extern "C" fn cbm_mcp_edge_type_valid(input: *const c_char) -> bool {
    unsafe { cbm_rs_mcp_edge_type_valid_v1(input) != 0 }
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向 NUL-terminated C string。回傳 1 表示
/// `search_code` root path / file pattern 不含會破壞 shell quoting 的 byte；
/// null 或 denylist byte 回傳 0。
pub unsafe extern "C" fn cbm_rs_mcp_search_path_arg_valid_v1(input: *const c_char) -> c_int {
    c_int::from(mcp::search_path_arg_valid(unsafe { c_bytes(input) }))
}

#[no_mangle]
/// # Safety
///
/// `root_path` 與 `file_pattern` 必須是 null，或指向 NUL-terminated C string。
/// root_path 必填且需通過 `search_path_arg_valid`；file_pattern 可為 null，若提供
/// 也需通過同一規則。符合時回傳 1，否則回傳 0。
pub unsafe extern "C" fn cbm_rs_mcp_search_args_valid_v1(
    root_path: *const c_char,
    file_pattern: *const c_char,
) -> c_int {
    c_int::from(mcp::search_args_valid(
        unsafe { c_bytes(root_path) },
        unsafe { c_bytes(file_pattern) },
    ))
}

#[no_mangle]
/// # Safety
///
/// `path` 與 `root` 必須是 null，或指向 NUL-terminated C string。回傳值是
/// `search_code` grep path 可安全跳過的 byte offset；0 表示不去除 prefix。
/// Rust 不回傳新字串，C 端仍從原 `path` 借用指標。
pub unsafe extern "C" fn cbm_rs_mcp_strip_root_prefix_offset_v1(
    path: *const c_char,
    root: *const c_char,
    root_len: usize,
) -> usize {
    mcp::search_strip_root_prefix_offset(
        unsafe { c_bytes(path) },
        unsafe { c_bytes(root) },
        root_len,
    )
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向 NUL-terminated C string。回傳值對齊
/// `search_code.mode` C helper：0=compact/default、1=full、2=files；null、
/// 空字串或未知 mode 回傳 0。
pub unsafe extern "C" fn cbm_rs_mcp_search_mode_v1(input: *const c_char) -> c_int {
    mcp::search_mode(unsafe { c_bytes(input) })
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向 NUL-terminated C string。回傳值對齊
/// `index_repository.mode` C dispatch：0=full/default、1=moderate、2=fast、
/// 3=cross-repo-intelligence；null、空字串或未知 mode 回傳 0。
pub unsafe extern "C" fn cbm_rs_mcp_index_mode_v1(input: *const c_char) -> c_int {
    mcp::index_mode(unsafe { c_bytes(input) })
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向 NUL-terminated C string。回傳 bitmask 對齊
/// `trace_path.mode` 的預設 edge type set：bit0=CALLS、bit1=DATA_FLOWS、
/// bit2=HTTP_CALLS、bit3=ASYNC_CALLS、bit4..9=CROSS_*；null、空字串或未知
/// mode 回傳 CALLS。
pub unsafe extern "C" fn cbm_rs_mcp_trace_mode_edge_mask_v1(input: *const c_char) -> u32 {
    mcp::trace_mode_edge_mask(unsafe { c_bytes(input) })
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向可寫入的 NUL-terminated C string。此函式會就地把
/// 所有大於 127 的 byte 改成 `?`；null 為 no-op。
pub unsafe extern "C" fn cbm_rs_mcp_sanitize_ascii_in_place_v1(input: *mut c_char) {
    if input.is_null() {
        return;
    }
    let mut len = 0usize;
    while unsafe { *input.add(len) } != 0 {
        len += 1;
    }
    let bytes = unsafe { slice::from_raw_parts_mut(input.cast::<u8>(), len) };
    mcp::sanitize_ascii_in_place(bytes);
}

#[no_mangle]
/// # Safety
///
/// `label` 與 `file` 必須是 null，或指向 NUL-terminated C string。此 ABI 對齊
/// `search_code` `compute_search_score()`：以 `in_degree` 為基底，依 label 與 file
/// path 加減固定分數；不讀取 store、不執行 grep、不處理 JSON。
pub unsafe extern "C" fn cbm_rs_mcp_search_code_score_v1(
    label: *const c_char,
    file: *const c_char,
    in_degree: c_int,
) -> c_int {
    mcp::search_code_score(
        unsafe { c_bytes(label) },
        unsafe { c_bytes(file) },
        in_degree,
    )
}

#[no_mangle]
/// `search_code` result qsort comparator score delta，對齊 C `search_result_cmp()`：
/// left score 較高時回傳負值，right score 較高時回傳正值。此 ABI 不讀取 result
/// struct、不排序、不碰 grep/JSON，只固定 score comparator scalar contract。
pub extern "C" fn cbm_rs_mcp_search_score_cmp_v1(left_score: c_int, right_score: c_int) -> c_int {
    mcp::search_score_cmp(left_score, right_score)
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`file` 必須是 null，
/// 或指向 NUL-terminated C string。成功時寫入 `search_code` directory
/// distribution 使用的 top-level key；null file 回傳 `SIZE_MAX`，短 buffer 會
/// NUL-terminate 截斷。
pub unsafe extern "C" fn cbm_rs_mcp_search_top_dir_v1(
    buf: *mut c_char,
    bufsize: usize,
    file: *const c_char,
) -> usize {
    let output = mcp::search_top_dir(unsafe { c_bytes(file) });
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `scope` 必須是 null，或指向 NUL-terminated C string。回傳值對齊
/// `detect_changes.scope` C helper：null/default、`symbols`、`impact` 皆啟用
/// impacted symbols；其他值只輸出 changed files。
pub unsafe extern "C" fn cbm_rs_mcp_detect_changes_wants_symbols_v1(scope: *const c_char) -> c_int {
    i32::from(mcp::detect_changes_wants_symbols(unsafe { c_bytes(scope) }))
}

#[no_mangle]
/// # Safety
///
/// `label` 必須是 null，或指向 NUL-terminated C string。回傳值對齊
/// `detect_changes` impacted symbols label filter：null、File、Folder、Project
/// 排除，其餘 label 保留。
pub unsafe extern "C" fn cbm_rs_mcp_detect_changes_impacted_label_v1(
    label: *const c_char,
) -> c_int {
    i32::from(mcp::detect_changes_impacted_label(unsafe {
        c_bytes(label)
    }))
}

#[no_mangle]
/// # Safety
///
/// `line` 必須是 null，或指向 NUL-terminated C string。回傳 `detect_changes`
/// 的 path 起始 offset：porcelain 前綴 (`XY `) 會被略過，rename 取 `" -> "`
/// 後 destination；空路徑或 null 會回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_detect_changes_status_path_offset_v1(
    line: *const c_char,
) -> usize {
    mcp::detect_changes_status_path_offset(unsafe { c_bytes(line) })
}

#[no_mangle]
/// `search_code` tightest-node classifier 的單一 node span helper。
/// 命中時回傳 `end_line - start_line`，未命中回傳 -1。
pub extern "C" fn cbm_rs_mcp_search_line_match_span_v1(
    start_line: c_int,
    end_line: c_int,
    line: c_int,
) -> c_int {
    mcp::search_line_match_span(i64::from(start_line), i64::from(end_line), i64::from(line))
        as c_int
}

#[no_mangle]
/// # Safety
///
/// `scores` 必須指向至少 `count` 個 `long` 的可讀陣列（或在 `count <= 0` 時可為
/// null），`ambiguous_out` 必須是可寫指標。回傳 best index；invalid 參數回 -1。
/// 當 top score 有多個候選時 `*ambiguous_out=1`，否則為 0。
pub unsafe extern "C" fn cbm_rs_mcp_search_pick_resolved_index_v1(
    scores: *const c_long,
    count: c_int,
    ambiguous_out: *mut c_int,
) -> c_int {
    if ambiguous_out.is_null() {
        return -1;
    }
    unsafe { *ambiguous_out = 0 };
    if count <= 0 || scores.is_null() {
        return -1;
    }
    let scores = unsafe { slice::from_raw_parts(scores, count as usize) };
    let (index, ambiguous) = mcp::search_pick_resolved_index(scores);
    unsafe { *ambiguous_out = i32::from(ambiguous) };
    index
}

#[no_mangle]
/// # Safety
///
/// `spans` 必須指向至少 `count` 個 `int` 的可讀陣列（或在 `count <= 0` 時可為
/// null）。回傳最小且非負 span 的第一個 index；若無有效 span 或參數無效回 -1。
pub unsafe extern "C" fn cbm_rs_mcp_search_pick_tightest_index_v1(
    spans: *const c_int,
    count: c_int,
) -> c_int {
    if count <= 0 || spans.is_null() {
        return -1;
    }
    let spans = unsafe { slice::from_raw_parts(spans, count as usize) };
    mcp::search_pick_tightest_index(spans)
}

#[no_mangle]
/// 判斷 UTF-8 continuation byte（10xxxxxx），對齊 C `utf8_is_cont()`。
pub extern "C" fn cbm_rs_mcp_utf8_is_cont_v1(byte: c_int) -> c_int {
    let b = (byte & 0xff) as u8;
    i32::from(mcp::utf8_is_cont_byte(b))
}

#[no_mangle]
/// # Safety
///
/// `label` 必須是 null，或指向 NUL-terminated C string。回傳值對齊
/// `node_resolution_score()`：Function/Method tier 最高、Module/File tier 最低、
/// 其他 label 居中，同 tier 時以非負 line span 加權。
pub unsafe extern "C" fn cbm_rs_mcp_node_resolution_score_v1(
    label: *const c_char,
    start_line: c_int,
    end_line: c_int,
) -> c_long {
    mcp::node_resolution_score(
        unsafe { c_bytes(label) },
        i64::from(start_line),
        i64::from(end_line),
    ) as c_long
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向 NUL-terminated C string。回傳值對齊
/// `manage_adr.mode` dispatch：0=get/default、1=update/store、2=sections。
pub unsafe extern "C" fn cbm_rs_mcp_adr_mode_v1(input: *const c_char) -> c_int {
    mcp::adr_mode(unsafe { c_bytes(input) })
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`content` 必須是
/// null，或指向 NUL-terminated C string。成功時寫入 `manage_adr(mode=sections)`
/// 使用的 sections JSON array，回傳完整 byte 長度；短 buffer 會截斷但
/// NUL-terminate。null content 對齊 C helper 產生空 array。
pub unsafe extern "C" fn cbm_rs_mcp_adr_sections_json_v1(
    buf: *mut c_char,
    bufsize: usize,
    content: *const c_char,
) -> usize {
    let output = mcp::adr_sections_json(unsafe { c_bytes(content) });
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向 NUL-terminated C string。`buf` 必須是 null，或指向
/// 可寫入 `bufsize` bytes 的 caller-owned buffer。此 ABI 對齊 `bm25_build_match()`：
/// 只輸出完整 token，並回傳已輸出的 token 數；invalid/null/過小 buffer 回 0，且不寫入。
pub unsafe extern "C" fn cbm_rs_mcp_bm25_build_match_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const c_char,
) -> c_int {
    if buf.is_null() || input.is_null() || bufsize < 2 {
        return 0;
    }
    let (tokens, out) = mcp::bm25_match_query(unsafe { c_bytes(input) }, bufsize);
    let dst = unsafe { slice::from_raw_parts_mut(buf.cast::<u8>(), bufsize) };
    let n = out.len().min(bufsize.saturating_sub(1));
    if n > 0 {
        dst[..n].copy_from_slice(&out[..n]);
    }
    dst[n] = 0;
    tokens
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`input` 必須是
/// null，或指向 NUL-terminated C string。成功時寫入 `search_graph.file_pattern`
/// BM25 path 使用的 SQL LIKE pattern 並回傳完整 byte 長度；短 buffer 會截斷但
/// NUL-terminate。`input == NULL` 回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_bm25_file_pattern_like_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const c_char,
) -> usize {
    let output = mcp::bm25_file_pattern_like(unsafe { c_bytes(input) });
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`input` 必須是
/// null，或指向 NUL-terminated C string。對非 UTF-8 byte 進行 lossy 消毒，將其替換為 REPLACEMENT CHARACTER U+FFFD。
/// `input == NULL` 回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_mcp_sanitize_utf8_lossy_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const c_char,
) -> usize {
    let output = mcp::sanitize_utf8_lossy(unsafe { c_bytes(input) });
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `input` (params_json) 必須是 null，或指向 NUL-terminated C string。
/// `name` 必須是 null，或指向 NUL-terminated C string。
/// 如果 aspect name 被請求則回傳 1，否則回傳 0。
pub unsafe extern "C" fn cbm_rs_mcp_architecture_aspect_wanted_v1(
    input: *const c_char,
    name: *const c_char,
) -> c_int {
    let name_bytes = unsafe { c_bytes(name) }.unwrap_or_default();
    c_int::from(mcp::architecture_aspect_wanted(
        unsafe { c_bytes(input) },
        name_bytes,
    ))
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向 NUL-terminated C string。回傳 1 表示
/// `trace_call_path` 的 file path 符合 MCP test-file substring classifier；
/// null 回傳 0。
pub unsafe extern "C" fn cbm_rs_mcp_trace_is_test_file_v1(input: *const c_char) -> c_int {
    c_int::from(mcp::trace_is_test_file(unsafe { c_bytes(input) }))
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向 NUL-terminated C string。回傳 1 表示檔名是
/// MCP cache scan 可考慮的 project `.db` 檔；null 或 internal/temp marker 回傳 0。
pub unsafe extern "C" fn cbm_rs_mcp_project_db_file_name_v1(input: *const c_char) -> c_int {
    c_int::from(mcp::project_db_file_name(unsafe { c_bytes(input) }))
}

#[no_mangle]
/// # Safety
///
/// `params_json` 與 `active_id_str` 必須是 null，或指向 NUL-terminated C string。
/// 回傳 1 表示 `notifications/cancelled` params.requestId 符合 active request id；
/// invalid JSON、root 非 object、缺少 `requestId` 或型別不符時回傳 0。
pub unsafe extern "C" fn cbm_rs_mcp_cancel_request_matches_v1(
    params_json: *const c_char,
    active_id: i64,
    active_id_str: *const c_char,
) -> c_int {
    let params = unsafe { c_bytes(params_json) };
    let active = unsafe { c_bytes(active_id_str) };
    c_int::from(mcp::cancel_request_matches(params, active_id, active))
}

#[no_mangle]
/// # Safety
///
/// `message` 必須是 null，或指向 NUL-terminated C string；`buf` 必須是 null，
/// 或指向 `bufsize` bytes 的可寫記憶體。回傳 JSON-RPC error response 的完整
/// byte 長度；短 buffer 會 NUL-terminate 截斷。
pub unsafe extern "C" fn cbm_rs_mcp_jsonrpc_format_error_v1(
    buf: *mut c_char,
    bufsize: usize,
    id: i64,
    code: c_int,
    message: *const c_char,
) -> usize {
    let output = mcp::jsonrpc_format_error(id, i64::from(code), unsafe { c_bytes(message) });
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `id_str`、`result_json` 與 `error_json` 必須是 null，或指向 NUL-terminated
/// C string；`buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。回傳
/// JSON-RPC response 的完整 byte 長度；短 buffer 會 NUL-terminate 截斷。
pub unsafe extern "C" fn cbm_rs_mcp_jsonrpc_format_response_v1(
    buf: *mut c_char,
    bufsize: usize,
    id: i64,
    id_str: *const c_char,
    result_json: *const c_char,
    error_json: *const c_char,
) -> usize {
    let output = mcp::jsonrpc_format_response(
        id,
        unsafe { c_bytes(id_str) },
        unsafe { c_bytes(result_json) },
        unsafe { c_bytes(error_json) },
    );
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `text` 必須是 null，或指向 NUL-terminated C string；`buf` 必須是 null，
/// 或指向 `bufsize` bytes 的可寫記憶體。回傳 MCP text result JSON 的完整
/// byte 長度；短 buffer 會 NUL-terminate 截斷。
pub unsafe extern "C" fn cbm_rs_mcp_text_result_v1(
    buf: *mut c_char,
    bufsize: usize,
    text: *const c_char,
    is_error: c_int,
) -> usize {
    let output = mcp::mcp_text_result(unsafe { c_bytes(text) }, is_error != 0);
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向至少 `len` bytes 的可讀記憶體；`out` 必須是可寫
/// 的 `CbmRsMcpJsonRpcParseOutV1*`。各輸出 buffer 可為 null；非 null 時必須指向
/// 對應大小的可寫記憶體。成功回傳 0，parse failure 或 invalid args 回傳 -1。
pub unsafe extern "C" fn cbm_rs_mcp_jsonrpc_parse_v1(
    input: *const u8,
    len: c_int,
    out: *mut CbmRsMcpJsonRpcParseOutV1,
    jsonrpc_buf: *mut c_char,
    jsonrpc_bufsize: usize,
    method_buf: *mut c_char,
    method_bufsize: usize,
    id_str_buf: *mut c_char,
    id_str_bufsize: usize,
    params_buf: *mut c_char,
    params_bufsize: usize,
) -> c_int {
    if out.is_null() {
        return -1;
    }
    let Some(input) = (unsafe { c_bytes_with_len(input, len) }) else {
        return -1;
    };
    let Some(parsed) = mcp::jsonrpc_parse(input) else {
        return -1;
    };

    let (id, has_id, id_kind, id_str) = match parsed.id {
        mcp::ParsedId::None => (-1, 0, 0, None),
        mcp::ParsedId::Int(value) => (value, 1, 1, None),
        mcp::ParsedId::String(value) => (-1, 1, 2, Some(value)),
        mcp::ParsedId::Other => (-1, 1, 3, None),
    };
    let params = parsed.params_raw;

    unsafe {
        *out = CbmRsMcpJsonRpcParseOutV1 {
            id,
            has_id,
            id_kind,
            has_params: c_int::from(params.is_some()),
            jsonrpc_len: parsed.jsonrpc.len(),
            method_len: parsed.method.len(),
            id_str_len: id_str.as_ref().map_or(0, Vec::len),
            params_len: params.as_ref().map_or(0, Vec::len),
        };
        write_c_output(
            jsonrpc_buf,
            jsonrpc_bufsize,
            Some(parsed.jsonrpc.as_slice()),
        );
        write_c_output(method_buf, method_bufsize, Some(parsed.method.as_slice()));
        if let Some(id_str) = id_str.as_ref() {
            write_c_output(id_str_buf, id_str_bufsize, Some(id_str.as_slice()));
        } else if !id_str_buf.is_null() && id_str_bufsize > 0 {
            *id_str_buf = 0;
        }
        if let Some(params) = params.as_ref() {
            write_c_output(params_buf, params_bufsize, Some(params.as_slice()));
        } else if !params_buf.is_null() && params_bufsize > 0 {
            *params_buf = 0;
        }
    }

    0
}

#[no_mangle]
/// # Safety
///
/// `params_json` 必須是 null，或指向 NUL-terminated C string。回傳 tools/list 分頁
/// 的 cursor offset，對齊 C `mcp_tools_cursor_offset`（詳見 `mcp::tools_cursor_offset`）；
/// `tool_count` 由呼叫端傳入 C 的 `TOOL_COUNT`。
pub unsafe extern "C" fn cbm_rs_mcp_tools_cursor_offset_v1(
    params_json: *const c_char,
    tool_count: c_int,
) -> c_int {
    let bytes = unsafe { c_bytes(params_json) };
    mcp::tools_cursor_offset(bytes, i64::from(tool_count)) as c_int
}

#[no_mangle]
pub extern "C" fn cbm_rs_pipeline_incremental_post_step_count(mode: c_int) -> c_int {
    plan::incremental_post_steps(mode).len() as c_int
}

#[no_mangle]
/// # Safety
///
/// `out` 必須非 null，且指向至少 `cap` 個 `CbmRsPipelinePlanStep` 的可寫陣列。
/// cap 不足或 out 為 null 時回傳 `-1`；成功時回傳實際 step 數。
pub unsafe extern "C" fn cbm_rs_pipeline_incremental_post_steps(
    mode: c_int,
    out: *mut CbmRsPipelinePlanStep,
    cap: c_int,
) -> c_int {
    if out.is_null() || cap < 0 {
        return -1;
    }
    let steps = plan::incremental_post_steps(mode);
    if cap < steps.len() as c_int {
        return -1;
    }
    let out = unsafe { slice::from_raw_parts_mut(out, steps.len()) };
    for (dst, step) in out.iter_mut().zip(steps) {
        dst.kind = step.kind as c_int;
        dst.policy = step.policy as c_int;
    }
    out.len() as c_int
}

#[no_mangle]
pub extern "C" fn cbm_rs_pipeline_plan_step_count_v2(
    kind: c_int,
    mode: c_int,
    worker_count: c_int,
    file_count: c_int,
) -> c_int {
    let Some(kind) = PlanKind::from_i32(kind) else {
        return -1;
    };
    let Some(steps) = plan::steps_v2(kind, mode, worker_count, file_count) else {
        return -1;
    };
    steps.len() as c_int
}

#[no_mangle]
/// # Safety
///
/// `out` 必須非 null，且指向至少 `cap` 個 `CbmRsPipelinePlanStepV2` 的可寫陣列。
/// cap 不足、out 為 null、未知或尚未支援的 plan kind 時回傳 `-1`；成功時回傳實際 step 數。
pub unsafe extern "C" fn cbm_rs_pipeline_plan_steps_v2(
    kind: c_int,
    mode: c_int,
    worker_count: c_int,
    file_count: c_int,
    out: *mut CbmRsPipelinePlanStepV2,
    cap: c_int,
) -> c_int {
    if out.is_null() || cap < 0 {
        return -1;
    }
    let Some(kind) = PlanKind::from_i32(kind) else {
        return -1;
    };
    let Some(steps) = plan::steps_v2(kind, mode, worker_count, file_count) else {
        return -1;
    };
    if cap < steps.len() as c_int {
        return -1;
    }
    let out = unsafe { slice::from_raw_parts_mut(out, steps.len()) };
    for (dst, step) in out.iter_mut().zip(steps) {
        dst.kind = step.kind as c_int;
        dst.phase = step.phase as c_int;
        dst.policy = step.policy as c_int;
        dst.gate_flags = step.gate_flags;
        dst.requires_mask = step.requires_mask;
        dst.effect_flags = step.effect_flags;
    }
    out.len() as c_int
}

#[no_mangle]
pub extern "C" fn cbm_rs_pipeline_full_plan_step_count_v1(
    mode: c_int,
    worker_count: c_int,
    file_count: c_int,
) -> c_int {
    plan::full_pipeline_top_steps(mode, worker_count, file_count).len() as c_int
}

#[no_mangle]
/// # Safety
///
/// `out` 必須非 null，且指向至少 `cap` 個 `CbmRsPipelineTopStepV1` 的可寫陣列。
/// cap 不足或 out 為 null 時回傳 `-1`；成功時回傳 full pipeline top-level step 數。
/// 此 API 描述 top-level orchestration metadata；pipeline.c 的 opt-in runtime 會讀取
/// metadata/choice，實際 pass side effects 仍由 C 執行。
pub unsafe extern "C" fn cbm_rs_pipeline_full_plan_steps_v1(
    mode: c_int,
    worker_count: c_int,
    file_count: c_int,
    out: *mut CbmRsPipelineTopStepV1,
    cap: c_int,
) -> c_int {
    if out.is_null() || cap < 0 {
        return -1;
    }
    let steps = plan::full_pipeline_top_steps(mode, worker_count, file_count);
    if cap < steps.len() as c_int {
        return -1;
    }
    let out = unsafe { slice::from_raw_parts_mut(out, steps.len()) };
    for (dst, step) in out.iter_mut().zip(steps) {
        dst.kind = step.kind as c_int;
        dst.phase = step.phase as c_int;
        dst.policy = step.policy as c_int;
        dst.gate_flags = step.gate_flags;
        dst.requires_mask = step.requires_mask;
        dst.effect_flags = step.effect_flags;
        dst.nested_plan_kind = step.nested_plan_kind;
    }
    out.len() as c_int
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向有效 NUL 結尾 C 字串。`buf` 可為 null；若非 null，必須
/// 指向至少 `bufsize` bytes 的可寫緩衝區。回傳 canonical path 的完整 byte 長度；
/// `bufsize` 不足時會寫入 NUL 截斷字串。null input 對齊 C helper，輸出空字串。
pub unsafe extern "C" fn cbm_rs_pipeline_route_canon_path_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const c_char,
) -> usize {
    let output = if input.is_null() {
        Vec::new()
    } else {
        pipeline_route::canon_path(unsafe { CStr::from_ptr(input) }.to_bytes())
    };
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `keyword` 必須是 null，或指向有效 NUL 結尾 C 字串；Rust 不保存該指標。
pub unsafe extern "C" fn cbm_rs_pipeline_is_path_keyword_v1(keyword: *const c_char) -> c_int {
    c_int::from(pipeline_route::is_path_keyword(unsafe { c_bytes(keyword) }))
}

#[no_mangle]
/// # Safety
///
/// `input` 必須是 null，或指向有效 NUL 結尾 C 字串；`buf` 若非 null，必須指向至少
/// `bufsize` bytes 的可寫緩衝區。成功時寫入 normalized URL 並回傳 1，否則回傳 0。
pub unsafe extern "C" fn cbm_rs_pipeline_normalize_url_arg_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const c_char,
) -> c_int {
    let Some(output) = pipeline_route::normalize_url_arg(unsafe { c_bytes(input) }, bufsize) else {
        return 0;
    };
    if buf.is_null() || bufsize == 0 {
        return 0;
    }
    unsafe { write_c_output(buf, bufsize, Some(&output)) };
    1
}

#[no_mangle]
/// # Safety
///
/// `segment` 必須是 null，或指向至少 `len` bytes 的可讀記憶體；Rust 不保存該指標。
pub unsafe extern "C" fn cbm_rs_pipeline_is_hash_segment_v1(
    segment: *const u8,
    len: usize,
) -> c_int {
    let segment = if segment.is_null() || len == 0 {
        None
    } else {
        Some(unsafe { std::slice::from_raw_parts(segment, len) })
    };
    c_int::from(pipeline_route::is_hash_segment(segment, 12, 3))
}

#[no_mangle]
/// # Safety
///
/// `qn` 必須是 null，或指向有效 NUL 結尾 C 字串；Rust 不保存該指標。
pub unsafe extern "C" fn cbm_rs_pipeline_is_broker_route_v1(qn: *const c_char) -> c_int {
    c_int::from(pipeline_route::is_broker_route(unsafe { c_bytes(qn) }))
}

#[no_mangle]
/// # Safety
///
/// `file_path` 必須是 null，或指向有效 NUL 結尾 C 字串；Rust 不保存該指標。
pub unsafe extern "C" fn cbm_rs_pipeline_sveltekit_file_kind_v1(file_path: *const c_char) -> c_int {
    pipeline_route::sveltekit_file_kind(unsafe { c_bytes(file_path) })
}

#[cfg(feature = "pipeline-sveltekit-file-kind-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_pipeline_sveltekit_file_kind(file_path: *const c_char) -> c_int {
    unsafe { cbm_rs_pipeline_sveltekit_file_kind_v1(file_path) }
}

const SVELTEKIT_METHOD_GET: &[u8] = b"GET\0";
const SVELTEKIT_METHOD_POST: &[u8] = b"POST\0";
const SVELTEKIT_METHOD_PUT: &[u8] = b"PUT\0";
const SVELTEKIT_METHOD_PATCH: &[u8] = b"PATCH\0";
const SVELTEKIT_METHOD_DELETE: &[u8] = b"DELETE\0";
const SVELTEKIT_METHOD_OPTIONS: &[u8] = b"OPTIONS\0";
const SVELTEKIT_METHOD_HEAD: &[u8] = b"HEAD\0";
const SVELTEKIT_METHOD_ANY: &[u8] = b"ANY\0";

fn sveltekit_method_c_ptr(method: Option<&[u8]>) -> *const c_char {
    match method {
        Some(b"GET") => SVELTEKIT_METHOD_GET.as_ptr().cast(),
        Some(b"POST") => SVELTEKIT_METHOD_POST.as_ptr().cast(),
        Some(b"PUT") => SVELTEKIT_METHOD_PUT.as_ptr().cast(),
        Some(b"PATCH") => SVELTEKIT_METHOD_PATCH.as_ptr().cast(),
        Some(b"DELETE") => SVELTEKIT_METHOD_DELETE.as_ptr().cast(),
        Some(b"OPTIONS") => SVELTEKIT_METHOD_OPTIONS.as_ptr().cast(),
        Some(b"HEAD") => SVELTEKIT_METHOD_HEAD.as_ptr().cast(),
        Some(b"ANY") => SVELTEKIT_METHOD_ANY.as_ptr().cast(),
        _ => ptr::null(),
    }
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向有效 NUL 結尾 C 字串；回傳指標指向 Rust 內部的靜態常數。
pub unsafe extern "C" fn cbm_rs_pipeline_sveltekit_server_method_v1(
    name: *const c_char,
) -> *const c_char {
    sveltekit_method_c_ptr(pipeline_route::sveltekit_server_method(unsafe {
        c_bytes(name)
    }))
}

#[cfg(feature = "pipeline-sveltekit-server-method-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_pipeline_sveltekit_server_method(
    name: *const c_char,
) -> *const c_char {
    unsafe { cbm_rs_pipeline_sveltekit_server_method_v1(name) }
}

#[no_mangle]
/// # Safety
///
/// `json` 與 `key` 必須是 null，或指向有效的 NUL 結尾 C 字串；`buf` 必須指向至少
/// `bufsize` bytes 的可寫緩衝區，且 `bufsize` 必須大於零。此 helper 不進行 JSON escape
/// 解碼。
pub unsafe extern "C" fn cbm_rs_pipeline_extract_json_prop_v1(
    json: *const c_char,
    key: *const c_char,
    buf: *mut c_char,
    bufsize: c_int,
) -> c_int {
    if buf.is_null() || bufsize <= 0 {
        return 0;
    }
    let bufsize = bufsize as usize;
    let Some(output) = pipeline_route::extract_json_prop(
        unsafe { c_bytes(json) },
        unsafe { c_bytes(key) },
        bufsize,
    ) else {
        return 0;
    };
    unsafe { write_c_output(buf, bufsize, Some(&output)) };
    1
}

#[cfg(feature = "pipeline-json-prop-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_pipeline_extract_json_prop(
    json: *const c_char,
    key: *const c_char,
    buf: *mut c_char,
    bufsize: c_int,
) -> bool {
    unsafe { cbm_rs_pipeline_extract_json_prop_v1(json, key, buf, bufsize) != 0 }
}

#[no_mangle]
/// # Safety
///
/// `url` 必須是 null，或指向有效的 NUL 結尾 C 字串；回傳指標指向輸入 buffer 內部，或
/// 指向 Rust 內部的靜態根 path 常數，呼叫端不得釋放。
pub unsafe extern "C" fn cbm_rs_pipeline_url_path_v1(url: *const c_char) -> *const c_char {
    match pipeline_route::url_path_borrowed(unsafe { c_bytes(url) }) {
        None => ptr::null(),
        Some(pipeline_route::UrlPath::Input(output)) => output.as_ptr().cast(),
        Some(pipeline_route::UrlPath::StaticRoot) => {
            static ROOT_PATH: &[u8] = b"/\0";
            ROOT_PATH.as_ptr().cast()
        }
    }
}

#[cfg(feature = "pipeline-url-path-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_pipeline_url_path(url: *const c_char) -> *const c_char {
    unsafe { cbm_rs_pipeline_url_path_v1(url) }
}

#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或指向有效 NUL 結尾 C 字串。回傳 1 表示 path 符合
/// `cbm_is_test_path()` 既有 byte-level classifier；null 或不符合回傳 0。
pub unsafe extern "C" fn cbm_rs_pipeline_is_test_path_v1(path: *const c_char) -> c_int {
    c_int::from(pipeline_test_detect::is_test_path(unsafe { c_bytes(path) }))
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向有效 NUL 結尾 C 字串。回傳 1 表示 function name
/// 符合 `cbm_is_test_func_name()` 既有 byte-level classifier；null 或不符合回傳 0。
pub unsafe extern "C" fn cbm_rs_pipeline_is_test_func_name_v1(name: *const c_char) -> c_int {
    c_int::from(pipeline_test_detect::is_test_func_name(unsafe {
        c_bytes(name)
    }))
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`test_path` 必須是 null，
/// 或指向有效 NUL 結尾 C 字串。成功時寫入 test file 對應的 production path，
/// null input、短 buffer 或無法轉換時回傳 `usize::MAX`。
pub unsafe extern "C" fn cbm_rs_pipeline_test_to_prod_path_v1(
    buf: *mut c_char,
    bufsize: usize,
    test_path: *const c_char,
) -> usize {
    if buf.is_null() || bufsize == 0 {
        return usize::MAX;
    }
    let Some(output) = pipeline_test_detect::test_to_prod_path(unsafe { c_bytes(test_path) })
    else {
        return usize::MAX;
    };
    if output.len() >= bufsize {
        return usize::MAX;
    }
    unsafe {
        if !output.is_empty() {
            ptr::copy_nonoverlapping(output.as_ptr().cast::<c_char>(), buf, output.len());
        }
        *buf.add(output.len()) = 0;
    }
    output.len()
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null，或指向有效 NUL 結尾 C 字串。回傳 1 表示 exception name
/// 符合 `is_checked_exception()` 的 byte-level contract；null 或含 `Error` /
/// `Panic` / `error` / `panic` 的名稱回傳 0。
pub unsafe extern "C" fn cbm_rs_pipeline_is_checked_exception_v1(name: *const c_char) -> c_int {
    c_int::from(pipeline_exception::is_checked_exception(unsafe {
        c_bytes(name)
    }))
}

unsafe fn write_artifact_path_prefix(buf: *mut c_char, bufsize: usize, output: &[u8]) {
    let copy_len = output.len().min(bufsize - 1);
    if copy_len != 0 {
        unsafe {
            ptr::copy_nonoverlapping(output.as_ptr().cast::<c_char>(), buf, copy_len);
        }
    }
    unsafe {
        *buf.add(copy_len) = 0;
    }
}

/// # Safety
///
/// `buf` 必須非 null，且指向至少 `bufsize` bytes 的可寫記憶體；`repo_path` 與 `name`
/// 必須非 null，並指向有效 NUL 結尾 C 字串。將 `<repo_path>/.codebase-memory/<name>`
/// 寫入 `buf`，回傳完整長度；若任何輸入為 null 或 `bufsize == 0`，則回傳
/// `usize::MAX` 且不寫入。若結果長度 `>= bufsize`，會先寫入可容納的 raw bytes 前綴與
/// NUL 結尾，再回傳 `usize::MAX`。此 ABI 只做 byte-level path 組裝，不做任何正規化或 I/O。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_artifact_path_v1(
    buf: *mut c_char,
    bufsize: usize,
    repo_path: *const c_char,
    name: *const c_char,
) -> usize {
    if buf.is_null() || bufsize == 0 {
        return usize::MAX;
    }
    let Some(output) =
        pipeline_artifact::artifact_path(unsafe { c_bytes(repo_path) }, unsafe { c_bytes(name) })
    else {
        return usize::MAX;
    };
    if output.len() >= bufsize {
        unsafe {
            write_artifact_path_prefix(buf, bufsize, output.as_ref());
        }
        return usize::MAX;
    }
    unsafe {
        write_artifact_path_prefix(buf, bufsize, output.as_ref());
    }
    output.len()
}

#[no_mangle]
pub extern "C" fn cbm_rs_gbuf_mutation_command_count_v1() -> c_int {
    graph_mutation::command_metas().len() as c_int
}

#[no_mangle]
/// # Safety
///
/// `out` 必須非 null，且指向至少 `cap` 個 `CbmRsGbufMutationMetaV1` 的可寫陣列。
/// cap 不足或 out 為 null 時回傳 `-1`；成功時回傳實際 command 數。
pub unsafe extern "C" fn cbm_rs_gbuf_mutation_commands_v1(
    out: *mut CbmRsGbufMutationMetaV1,
    cap: c_int,
) -> c_int {
    if out.is_null() || cap < 0 {
        return -1;
    }
    let metas = graph_mutation::command_metas();
    if cap < metas.len() as c_int {
        return -1;
    }
    let out = unsafe { slice::from_raw_parts_mut(out, metas.len()) };
    for (dst, meta) in out.iter_mut().zip(metas) {
        dst.kind = meta.kind as c_int;
        dst.result_kind = meta.result_kind as c_int;
        dst.required_fields = meta.required_fields;
        dst.optional_fields = meta.optional_fields;
        dst.effect_flags = meta.effect_flags;
        dst.validation_flags = meta.validation_flags;
        dst.reserved0 = 0;
        dst.reserved1 = 0;
    }
    out.len() as c_int
}

#[no_mangle]
/// # Safety
///
/// `cmd` 必須非 null，且指向有效的 `CbmRsGbufMutationCmdV1`；`out` 必須非
/// null，且指向可寫的 `CbmRsGbufMutationValidationV1`。此 API 只驗證
/// command shape，不保存 C 指標，也不呼叫 graph-buffer mutation。
pub unsafe extern "C" fn cbm_rs_gbuf_mutation_validate_v1(
    cmd: *const CbmRsGbufMutationCmdV1,
    out: *mut CbmRsGbufMutationValidationV1,
) -> c_int {
    if cmd.is_null() || out.is_null() {
        return -1;
    }
    let cmd = unsafe { &*cmd };
    let validation = graph_mutation::validate(graph_mutation::MutationCommandShape {
        kind: cmd.kind,
        reserved0: cmd.reserved0,
        has_label: !cmd.label.is_null(),
        has_name: !cmd.name.is_null(),
        has_qualified_name: !cmd.qualified_name.is_null(),
        has_file_path: !cmd.file_path.is_null(),
        has_edge_type: !cmd.edge_type.is_null(),
        has_properties_json: !cmd.properties_json.is_null(),
    });
    unsafe {
        (*out).ok = validation.ok as c_int;
        (*out).error_code = validation.error_code;
        (*out).missing_fields = validation.missing_fields;
        (*out).invalid_fields = validation.invalid_fields;
        (*out).normalized_flags = validation.normalized_flags;
        (*out).reserved0 = 0;
    }
    0
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`path` 必須是
/// null，或指向 NUL-terminated C string。此 API 只建構 SQLite immutable
/// URI fallback 字串，不開 SQLite、不執行 pragma、不保存 C 指標。
pub unsafe extern "C" fn cbm_rs_store_build_immutable_uri_v1(
    buf: *mut c_char,
    bufsize: usize,
    path: *const c_char,
) -> usize {
    let output = store_open::immutable_uri(unsafe { c_bytes(path) });
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[cfg(feature = "store-immutable-uri-only")]
#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或指向 NUL 結尾的 C string；`out` 必須是 null，或指向
/// 至少 `out_sz` bytes 的可寫記憶體。成功時寫入 NUL 結尾 URI，不保存任何 C 指標。
pub unsafe extern "C" fn cbm_store_build_immutable_uri(
    path: *const c_char,
    out: *mut c_char,
    out_sz: usize,
) -> bool {
    if out.is_null() || out_sz == 0 {
        return false;
    }
    let Some(output) = store_open::immutable_uri(unsafe { c_bytes(path) }) else {
        return false;
    };
    if output.len() >= out_sz {
        return false;
    }
    unsafe {
        std::ptr::copy_nonoverlapping(output.as_ptr().cast::<c_char>(), out, output.len());
        *out.add(output.len()) = 0;
    }
    true
}

#[no_mangle]
/// # Safety
///
/// `norm_out` 必須是 null，或指向 `norm_sz` bytes 的可寫記憶體；`path` 必須是
/// null，或指向 NUL-terminated C string。回傳寫入的正規化路徑長度（不含 NUL），
/// 或 `usize::MAX` 表示 path 未設定或正規化後為空（對應 C `arch_path_prepare`
/// 回傳 false）。截斷點對齊 C `strncpy(norm_out, path, norm_sz - 1)`：先截斷再去尾端
/// 與折疊 slash。只做 byte-level 路徑正規化，不碰 SQLite。
pub unsafe extern "C" fn cbm_rs_store_normalize_arch_path_v1(
    norm_out: *mut c_char,
    norm_sz: usize,
    path: *const c_char,
) -> usize {
    if norm_out.is_null() || norm_sz == 0 {
        return usize::MAX;
    }
    let Some(output) = store_arch_helpers::normalize_arch_path(unsafe { c_bytes(path) }, norm_sz)
    else {
        return usize::MAX;
    };
    let n = output.len();
    unsafe {
        if n > 0 {
            ptr::copy_nonoverlapping(output.as_ptr().cast::<c_char>(), norm_out, n);
        }
        *norm_out.add(n) = 0;
    }
    n
}

#[cfg(feature = "store-arch-path-scope-only")]
#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或指向 NUL 結尾的 C string；`norm_out` 與 `like_out`
/// 必須指向呼叫端提供的可寫 buffer。成功時寫入正規化 path 與其 `/%` LIKE
/// pattern，不保存任何 C 指標。
pub unsafe extern "C" fn cbm_store_arch_path_prepare(
    path: *const c_char,
    norm_out: *mut c_char,
    norm_sz: usize,
    like_out: *mut c_char,
    like_sz: usize,
) -> bool {
    if norm_out.is_null() || norm_sz == 0 || like_out.is_null() || like_sz == 0 {
        return false;
    }
    let Some(output) = store_arch_helpers::normalize_arch_path(unsafe { c_bytes(path) }, norm_sz)
    else {
        return false;
    };
    unsafe {
        std::ptr::copy_nonoverlapping(output.as_ptr().cast::<c_char>(), norm_out, output.len());
        *norm_out.add(output.len()) = 0;
    }
    let mut like = output;
    like.extend_from_slice(b"/%");
    let copy_len = like.len().min(like_sz - 1);
    unsafe {
        std::ptr::copy_nonoverlapping(like.as_ptr().cast::<c_char>(), like_out, copy_len);
        *like_out.add(copy_len) = 0;
    }
    true
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`path` 必須是 null，或
/// 指向 NUL-terminated C string。將 path 最後一個 `.` 起的副檔名（含 `.`）以 ASCII
/// 小寫寫入 `buf`，回傳長度；無 `.`、副檔名長度 >= `bufsize`（對齊 C `file_ext` 的
/// `len >= sizeof(buf)`）、`buf` 為 null 或 `bufsize == 0` 時回傳 `usize::MAX`。
pub unsafe extern "C" fn cbm_rs_store_file_ext_lower_v1(
    buf: *mut c_char,
    bufsize: usize,
    path: *const c_char,
) -> usize {
    if buf.is_null() || bufsize == 0 {
        return usize::MAX;
    }
    let Some(output) = store_arch_helpers::file_ext_lower(unsafe { c_bytes(path) }, bufsize) else {
        return usize::MAX;
    };
    let n = output.len();
    unsafe {
        if n > 0 {
            ptr::copy_nonoverlapping(output.as_ptr().cast::<c_char>(), buf, n);
        }
        *buf.add(n) = 0;
    }
    n
}

#[no_mangle]
/// # Safety
///
/// `ext` 必須是 null，或指向 NUL-terminated C string。回傳對齊 C
/// `ext_lang_table` 的 language index，未知或 null 回 `-1`；不保存 C 指標。
pub unsafe extern "C" fn cbm_rs_store_ext_lang_kind_v1(ext: *const c_char) -> c_int {
    store_arch_helpers::ext_language_kind(unsafe { c_bytes(ext) }).unwrap_or(-1)
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`pattern` 必須是
/// null，或指向 NUL-terminated C string。此 API 只執行 glob 到 SQL LIKE
/// pattern 的 byte-level 轉換，不碰 SQLite。
pub unsafe extern "C" fn cbm_rs_store_glob_to_like_v1(
    buf: *mut c_char,
    bufsize: usize,
    pattern: *const c_char,
) -> usize {
    let output = store_search_pattern::glob_to_like(unsafe { c_bytes(pattern) });
    unsafe { write_c_output(buf, bufsize, output.as_deref()) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`pattern` 必須是
/// null，或指向 NUL-terminated C string。null pattern 會輸出空字串。
pub unsafe extern "C" fn cbm_rs_store_ensure_case_insensitive_v1(
    buf: *mut c_char,
    bufsize: usize,
    pattern: *const c_char,
) -> usize {
    let output = store_search_pattern::ensure_case_insensitive(unsafe { c_bytes(pattern) });
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`pattern` 必須是
/// null，或指向 NUL-terminated C string。null pattern 會輸出空字串。
pub unsafe extern "C" fn cbm_rs_store_strip_case_flag_v1(
    buf: *mut c_char,
    bufsize: usize,
    pattern: *const c_char,
) -> usize {
    let output = store_search_pattern::strip_case_flag(unsafe { c_bytes(pattern) });
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `pattern` 必須是 null，或指向 NUL-terminated C string。回傳最多 `max_out`
/// 個 LIKE hint 的數量；不配置 Rust-owned output。
pub unsafe extern "C" fn cbm_rs_store_like_hint_count_v1(
    pattern: *const c_char,
    max_out: c_int,
) -> c_int {
    store_search_pattern::like_hints(unsafe { c_bytes(pattern) }, max_out).len() as c_int
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`pattern` 必須是
/// null，或指向 NUL-terminated C string。`index` 不存在時回傳 `SIZE_MAX`。
pub unsafe extern "C" fn cbm_rs_store_like_hint_v1(
    buf: *mut c_char,
    bufsize: usize,
    pattern: *const c_char,
    max_out: c_int,
    index: c_int,
) -> usize {
    if index < 0 {
        return usize::MAX;
    }
    let hints = store_search_pattern::like_hints(unsafe { c_bytes(pattern) }, max_out);
    let output = hints.get(index as usize).map(Vec::as_slice);
    unsafe { write_c_output(buf, bufsize, output) }
}

#[cfg(feature = "store-search-pattern-only")]
unsafe extern "C" {
    fn malloc(size: usize) -> *mut c_void;
}

#[cfg(feature = "store-search-pattern-only")]
unsafe fn malloc_c_string(output: Option<&[u8]>) -> *mut c_char {
    let Some(output) = output else {
        return ptr::null_mut();
    };
    let Some(size) = output.len().checked_add(1) else {
        return ptr::null_mut();
    };
    let out = unsafe { malloc(size) }.cast::<c_char>();
    if out.is_null() {
        return ptr::null_mut();
    }
    unsafe {
        if !output.is_empty() {
            ptr::copy_nonoverlapping(output.as_ptr().cast::<c_char>(), out, output.len());
        }
        *out.add(output.len()) = 0;
    }
    out
}

#[cfg(feature = "store-search-pattern-only")]
#[no_mangle]
/// # Safety
///
/// `pattern` 必須是 null，或指向 NUL-terminated C string。回傳記憶體由 C
/// `malloc` 配置，呼叫端必須以 `free` 釋放。
pub unsafe extern "C" fn cbm_glob_to_like(pattern: *const c_char) -> *mut c_char {
    let output = store_search_pattern::glob_to_like(unsafe { c_bytes(pattern) });
    unsafe { malloc_c_string(output.as_deref()) }
}

#[cfg(feature = "store-search-pattern-only")]
#[no_mangle]
/// # Safety
///
/// `pattern` 必須是 null，或指向 NUL-terminated C string；`out` 必須是 null，
/// 或指向至少 `max_out` 個可寫的 `char*`。每個寫入的字串由 C `malloc` 配置，
/// 呼叫端必須逐一以 `free` 釋放。
pub unsafe extern "C" fn cbm_extract_like_hints(
    pattern: *const c_char,
    out: *mut *mut c_char,
    max_out: c_int,
) -> c_int {
    if pattern.is_null() || out.is_null() || max_out <= 0 {
        return 0;
    }
    let hints = store_search_pattern::like_hints(unsafe { c_bytes(pattern) }, max_out);
    let mut count = 0usize;
    for hint in hints.iter().take(max_out as usize) {
        let value = unsafe { malloc_c_string(Some(hint.as_slice())) };
        if value.is_null() {
            break;
        }
        unsafe {
            *out.add(count) = value;
        }
        count += 1;
    }
    count as c_int
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`qn` 必須是
/// null，或指向 NUL-terminated C string。此 API 只執行 QN segment 的
/// byte-level extraction，不碰 SQLite。
pub unsafe extern "C" fn cbm_rs_store_qn_to_package_v1(
    buf: *mut c_char,
    bufsize: usize,
    qn: *const c_char,
) -> usize {
    let output = store_arch_helpers::qn_to_package(unsafe { c_bytes(qn) });
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`qn` 必須是
/// null，或指向 NUL-terminated C string。此 API 只執行 QN top-level
/// segment 的 byte-level extraction，不碰 SQLite。
pub unsafe extern "C" fn cbm_rs_store_qn_to_top_package_v1(
    buf: *mut c_char,
    bufsize: usize,
    qn: *const c_char,
) -> usize {
    let output = store_arch_helpers::qn_to_top_package(unsafe { c_bytes(qn) });
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `path` 必須是 null，或指向 NUL-terminated C string。此 API 只做
/// case-sensitive byte substring predicate，不保存 C 指標。
pub unsafe extern "C" fn cbm_rs_store_is_test_file_path_v1(path: *const c_char) -> c_int {
    if store_arch_helpers::is_test_file_path(unsafe { c_bytes(path) }) {
        1
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn cbm_rs_store_hop_to_risk_v1(hop: c_int) -> c_int {
    store_arch_helpers::hop_to_risk(hop)
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體。此 API 只將
/// risk enum 值轉為既有 C label 字串。
pub unsafe extern "C" fn cbm_rs_store_risk_label_v1(
    buf: *mut c_char,
    bufsize: usize,
    level: c_int,
) -> usize {
    let output = store_arch_helpers::risk_label(level);
    unsafe { write_c_output(buf, bufsize, Some(output)) }
}

#[cfg(feature = "store-arch-helpers-only")]
mod direct_store_arch_helpers {
    use super::{c_bytes, c_char, c_int, store_arch_helpers};
    use std::cell::UnsafeCell;
    use std::ptr;

    const QN_BUF_SIZE: usize = 256;
    static CRITICAL: &[u8] = b"CRITICAL\0";
    static HIGH: &[u8] = b"HIGH\0";
    static MEDIUM: &[u8] = b"MEDIUM\0";
    static LOW: &[u8] = b"LOW\0";

    thread_local! {
        static PACKAGE_BUF: UnsafeCell<[u8; QN_BUF_SIZE]> = const { UnsafeCell::new([0; QN_BUF_SIZE]) };
        static TOP_PACKAGE_BUF: UnsafeCell<[u8; QN_BUF_SIZE]> = const { UnsafeCell::new([0; QN_BUF_SIZE]) };
    }

    fn write_package(output: &[u8], buf: &UnsafeCell<[u8; QN_BUF_SIZE]>) -> *const c_char {
        let len = if output.len() < QN_BUF_SIZE {
            output.len()
        } else {
            0
        };
        unsafe {
            let dst = (*buf.get()).as_mut_ptr();
            ptr::copy_nonoverlapping(output.as_ptr(), dst, len);
            *dst.add(len) = 0;
            dst.cast()
        }
    }

    #[no_mangle]
    pub extern "C" fn cbm_hop_to_risk(hop: c_int) -> c_int {
        store_arch_helpers::hop_to_risk(hop)
    }

    #[no_mangle]
    pub extern "C" fn cbm_risk_label(level: c_int) -> *const c_char {
        match level {
            0 => CRITICAL.as_ptr().cast(),
            1 => HIGH.as_ptr().cast(),
            2 => MEDIUM.as_ptr().cast(),
            _ => LOW.as_ptr().cast(),
        }
    }

    #[no_mangle]
    /// # Safety
    ///
    /// `qn` 必須是 null，或指向 NUL 結尾的 C string。回傳指標指向目前執行緒
    /// 的固定 buffer，下一次同一 helper 呼叫時可能被覆寫，呼叫端不得 free。
    pub unsafe extern "C" fn cbm_qn_to_package(qn: *const c_char) -> *const c_char {
        let output = store_arch_helpers::qn_to_package(unsafe { c_bytes(qn) });
        PACKAGE_BUF.with(|buf| write_package(&output, buf))
    }

    #[no_mangle]
    /// # Safety
    ///
    /// `qn` 必須是 null，或指向 NUL 結尾的 C string。回傳指標指向目前執行緒
    /// 的固定 buffer，下一次同一 helper 呼叫時可能被覆寫，呼叫端不得 free。
    pub unsafe extern "C" fn cbm_qn_to_top_package(qn: *const c_char) -> *const c_char {
        let output = store_arch_helpers::qn_to_top_package(unsafe { c_bytes(qn) });
        TOP_PACKAGE_BUF.with(|buf| write_package(&output, buf))
    }

    #[no_mangle]
    /// # Safety
    ///
    /// `fp` 必須是 null，或指向 NUL 結尾的 C string。
    pub unsafe extern "C" fn cbm_is_test_file_path(fp: *const c_char) -> bool {
        store_arch_helpers::is_test_file_path(unsafe { c_bytes(fp) })
    }
}

#[cfg(feature = "store-file-ext-only")]
mod direct_store_file_ext {
    use super::{c_bytes, c_char, store_arch_helpers};
    use std::cell::UnsafeCell;
    use std::ptr;

    const EXT_BUF_SIZE: usize = 16;

    thread_local! {
        static EXT_BUF: UnsafeCell<[u8; EXT_BUF_SIZE]> = const { UnsafeCell::new([0; EXT_BUF_SIZE]) };
    }

    fn write_extension(
        output: Option<&[u8]>,
        buf: &UnsafeCell<[u8; EXT_BUF_SIZE]>,
    ) -> *const c_char {
        let Some(output) = output else {
            return ptr::null();
        };
        if output.len() >= EXT_BUF_SIZE {
            return ptr::null();
        }
        unsafe {
            let dst = (*buf.get()).as_mut_ptr();
            ptr::copy_nonoverlapping(output.as_ptr(), dst, output.len());
            *dst.add(output.len()) = 0;
            dst.cast()
        }
    }

    #[no_mangle]
    /// # Safety
    ///
    /// `path` 必須是 null，或指向 NUL 結尾的 C string。回傳指標指向目前
    /// 執行緒的固定 buffer，下一次同一 helper 呼叫時可能被覆寫，呼叫端不得 free。
    pub unsafe extern "C" fn cbm_store_file_ext(path: *const c_char) -> *const c_char {
        let output = store_arch_helpers::file_ext_lower(unsafe { c_bytes(path) }, EXT_BUF_SIZE);
        EXT_BUF.with(|buf| write_extension(output.as_deref(), buf))
    }
}

#[no_mangle]
/// # Safety
///
/// `buf` 必須是 null，或指向 `bufsize` bytes 的可寫記憶體；`input` 必須是
/// null，或指向 NUL-terminated C string。此 API 只固定 Store FTS
/// `cbm_camel_split` 的 test-only byte-level contract，不開 SQLite。
pub unsafe extern "C" fn cbm_rs_store_camel_split_v1(
    buf: *mut c_char,
    bufsize: usize,
    input: *const c_char,
) -> usize {
    let output = tokenization::camel_split_bytes(unsafe { c_bytes(input) });
    unsafe { write_c_output(buf, bufsize, Some(&output)) }
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null，或是有效的 NUL-terminated C string。此 API
/// 只固定 `CBM_SQLITE_MMAP_SIZE` 字串值到 `PRAGMA mmap_size` 數值的
/// test-only contract；不讀 env、不開 SQLite、不改變 production path。
pub unsafe extern "C" fn cbm_rs_store_resolve_mmap_size_value_v1(value: *const c_char) -> i64 {
    store_pragmas::resolve_mmap_size_value(unsafe { c_bytes(value) })
}

#[cfg(feature = "store-mmap-resolver-only")]
#[no_mangle]
pub extern "C" fn cbm_store_resolve_mmap_size() -> i64 {
    let value = platform::safe_getenv_bytes(Some(b"CBM_SQLITE_MMAP_SIZE"), None);
    store_pragmas::resolve_mmap_size_value(value.as_deref())
}

#[no_mangle]
pub extern "C" fn cbm_rs_store_schema_manifest_entry_count_v1() -> c_int {
    schema_manifest::entries().len() as c_int
}

#[no_mangle]
pub extern "C" fn cbm_rs_store_schema_manifest_user_index_count_v1() -> c_int {
    schema_manifest::user_index_count() as c_int
}

#[no_mangle]
/// # Safety
///
/// `out` 必須非 null，且指向至少 `cap` 個 `CbmRsStoreSchemaManifestEntryV1`
/// 的可寫陣列。成功回傳實際 entry 數；out 為 null、cap 為負數或不足時回傳 `-1`。
/// 回傳的 C string 指標指向 Rust static manifest，呼叫端不得 free。
pub unsafe extern "C" fn cbm_rs_store_schema_manifest_entries_v1(
    out: *mut CbmRsStoreSchemaManifestEntryV1,
    cap: c_int,
) -> c_int {
    if out.is_null() || cap < 0 {
        return -1;
    }
    let entries = schema_manifest::entries();
    if cap < entries.len() as c_int {
        return -1;
    }
    let out = unsafe { slice::from_raw_parts_mut(out, entries.len()) };
    for (dst, entry) in out.iter_mut().zip(entries) {
        dst.kind = entry.kind;
        dst.flags = entry.flags;
        dst.name = entry.name.as_ptr().cast::<c_char>();
        dst.table_name = entry.table_name.as_ptr().cast::<c_char>();
        dst.column_name = entry.column_name.as_ptr().cast::<c_char>();
    }
    out.len() as c_int
}

#[no_mangle]
pub extern "C" fn cbm_rs_store_schema_manifest_entry_count_v2() -> c_int {
    schema_manifest::entries_v2().len() as c_int
}

#[no_mangle]
pub extern "C" fn cbm_rs_store_schema_manifest_table_column_count_v2() -> c_int {
    schema_manifest::table_column_count_v2() as c_int
}

#[no_mangle]
pub extern "C" fn cbm_rs_store_schema_manifest_fts_shadow_count_v2() -> c_int {
    schema_manifest::fts_shadow_table_count_v2() as c_int
}

#[no_mangle]
/// # Safety
///
/// `out` 必須非 null，且指向至少 `cap` 個 `CbmRsStoreSchemaManifestEntryV2`
/// 的可寫陣列。成功回傳實際 entry 數；out 為 null、cap 為負數或不足時回傳 `-1`。
/// 回傳的 C string 指標指向 Rust static manifest，呼叫端不得 free。
pub unsafe extern "C" fn cbm_rs_store_schema_manifest_entries_v2(
    out: *mut CbmRsStoreSchemaManifestEntryV2,
    cap: c_int,
) -> c_int {
    if out.is_null() || cap < 0 {
        return -1;
    }
    let entries = schema_manifest::entries_v2();
    if cap < entries.len() as c_int {
        return -1;
    }
    let out = unsafe { slice::from_raw_parts_mut(out, entries.len()) };
    for (dst, entry) in out.iter_mut().zip(entries) {
        dst.kind = entry.kind;
        dst.flags = entry.flags;
        dst.ordinal = entry.ordinal;
        dst.hidden = entry.hidden;
        dst.name = entry.name.as_ptr().cast::<c_char>();
        dst.table_name = entry.table_name.as_ptr().cast::<c_char>();
        dst.column_name = entry.column_name.as_ptr().cast::<c_char>();
        dst.column_type = entry.column_type.as_ptr().cast::<c_char>();
        dst.default_sql = entry.default_sql.as_ptr().cast::<c_char>();
        dst.columns_csv = entry.columns_csv.as_ptr().cast::<c_char>();
        dst.sql_fragment = entry.sql_fragment.as_ptr().cast::<c_char>();
    }
    out.len() as c_int
}

#[no_mangle]
pub extern "C" fn cbm_rs_store_connection_manifest_entry_count_v1() -> c_int {
    schema_manifest::connection_entries_v1().len() as c_int
}

#[no_mangle]
pub extern "C" fn cbm_rs_store_connection_manifest_write_pragma_count_v1() -> c_int {
    schema_manifest::connection_write_pragma_count_v1() as c_int
}

#[no_mangle]
pub extern "C" fn cbm_rs_store_connection_manifest_entry_count_for_mode_v1(
    mode_mask: u32,
) -> c_int {
    schema_manifest::connection_entry_count_for_mode_v1(mode_mask) as c_int
}

#[no_mangle]
pub extern "C" fn cbm_rs_store_connection_manifest_write_entry_count_for_mode_v1(
    mode_mask: u32,
) -> c_int {
    schema_manifest::connection_write_entry_count_for_mode_v1(mode_mask) as c_int
}

#[no_mangle]
/// # Safety
///
/// `out` 必須非 null，且指向至少 `cap` 個 `CbmRsStoreConnectionManifestEntryV1`
/// 的可寫陣列。成功回傳實際 entry 數；out 為 null、cap 為負數或不足時回傳 `-1`。
/// 回傳的 C string 指標指向 Rust static manifest，呼叫端不得 free。
pub unsafe extern "C" fn cbm_rs_store_connection_manifest_entries_v1(
    out: *mut CbmRsStoreConnectionManifestEntryV1,
    cap: c_int,
) -> c_int {
    if out.is_null() || cap < 0 {
        return -1;
    }
    let entries = schema_manifest::connection_entries_v1();
    if cap < entries.len() as c_int {
        return -1;
    }
    let out = unsafe { slice::from_raw_parts_mut(out, entries.len()) };
    for (dst, entry) in out.iter_mut().zip(entries) {
        dst.kind = entry.kind;
        dst.flags = entry.flags;
        dst.mode_mask = entry.mode_mask;
        dst.ordinal = entry.ordinal;
        dst.value_i64 = entry.value_i64;
        dst.name = entry.name.as_ptr().cast::<c_char>();
        dst.sql = entry.sql.as_ptr().cast::<c_char>();
        dst.env_name = entry.env_name.as_ptr().cast::<c_char>();
    }
    out.len() as c_int
}

#[no_mangle]
/// # Safety
///
/// `out` 必須非 null，且指向至少 `cap` 個 `CbmRsStoreConnectionManifestEntryV1`
/// 的可寫陣列。成功回傳符合 `mode_mask` 的 entry 數；out 為 null、cap
/// 為負數或不足時回傳 `-1`。回傳的 C string 指標指向 Rust static manifest，
/// 呼叫端不得 free。
pub unsafe extern "C" fn cbm_rs_store_connection_manifest_entries_for_mode_v1(
    mode_mask: u32,
    out: *mut CbmRsStoreConnectionManifestEntryV1,
    cap: c_int,
) -> c_int {
    if out.is_null() || cap < 0 {
        return -1;
    }
    let count = schema_manifest::connection_entry_count_for_mode_v1(mode_mask);
    if cap < count as c_int {
        return -1;
    }
    let out = unsafe { slice::from_raw_parts_mut(out, count) };
    for (dst, entry) in out
        .iter_mut()
        .zip(schema_manifest::connection_entries_for_mode_v1(mode_mask))
    {
        dst.kind = entry.kind;
        dst.flags = entry.flags;
        dst.mode_mask = entry.mode_mask;
        dst.ordinal = entry.ordinal;
        dst.value_i64 = entry.value_i64;
        dst.name = entry.name.as_ptr().cast::<c_char>();
        dst.sql = entry.sql.as_ptr().cast::<c_char>();
        dst.env_name = entry.env_name.as_ptr().cast::<c_char>();
    }
    out.len() as c_int
}

#[no_mangle]
/// # Safety
///
/// `text` 必須是 null，或指向至少 `len` bytes 的可讀記憶體。此 API
/// 對齊 `cbm_yaml_parse` 的 test-only contract：null 或 `len <= 0`
/// 會回傳空 map handle；呼叫端必須用 `cbm_rs_yaml_free` 釋放。
pub unsafe extern "C" fn cbm_rs_yaml_parse(text: *const u8, len: c_int) -> *mut YamlNode {
    let input = if text.is_null() || len <= 0 {
        None
    } else {
        Some(unsafe { slice::from_raw_parts(text, len as usize) })
    };
    Box::into_raw(Box::new(YamlNode::parse(input, len)))
}

#[no_mangle]
/// # Safety
///
/// `root` 必須是 null，或是 `cbm_rs_yaml_parse` 回傳且尚未 free 的指標。
pub unsafe extern "C" fn cbm_rs_yaml_free(root: *mut YamlNode) {
    if root.is_null() {
        return;
    }
    unsafe {
        drop(Box::from_raw(root));
    }
}

#[no_mangle]
/// # Safety
///
/// `root` 必須是 null，或是 `cbm_rs_yaml_parse` 回傳且仍有效的指標；
/// `path` 必須是 null，或是有效的 NUL-terminated C string。回傳指標
/// 由 YAML tree 擁有，呼叫端不得 free，且只在 `root` free 前有效。
pub unsafe extern "C" fn cbm_rs_yaml_get_str(
    root: *const YamlNode,
    path: *const c_char,
) -> *const c_char {
    if root.is_null() {
        return ptr::null();
    }
    let Some(path) = (unsafe { c_bytes(path) }) else {
        return ptr::null();
    };
    unsafe { &*root }
        .get_str(path)
        .map_or(ptr::null(), |value| value.as_ptr().cast::<c_char>())
}

#[no_mangle]
/// # Safety
///
/// `root` 與 `path` contract 同 `cbm_rs_yaml_get_str`。
pub unsafe extern "C" fn cbm_rs_yaml_get_float(
    root: *const YamlNode,
    path: *const c_char,
    default_val: f64,
) -> f64 {
    if root.is_null() {
        return default_val;
    }
    let Some(path) = (unsafe { c_bytes(path) }) else {
        return default_val;
    };
    unsafe { &*root }.get_float(path, default_val)
}

#[no_mangle]
/// # Safety
///
/// `root` 與 `path` contract 同 `cbm_rs_yaml_get_str`。
pub unsafe extern "C" fn cbm_rs_yaml_get_bool(
    root: *const YamlNode,
    path: *const c_char,
    default_val: bool,
) -> bool {
    if root.is_null() {
        return default_val;
    }
    let Some(path) = (unsafe { c_bytes(path) }) else {
        return default_val;
    };
    unsafe { &*root }.get_bool(path, default_val)
}

#[no_mangle]
/// # Safety
///
/// `root` 與 `path` contract 同 `cbm_rs_yaml_get_str`。`out` 必須是 null，
/// 或指向至少 `max_out` 個 `const char*` 的可寫陣列；null 或 `max_out <= 0`
/// 回傳 0。寫入的字串由 YAML tree 擁有。
pub unsafe extern "C" fn cbm_rs_yaml_get_str_list(
    root: *const YamlNode,
    path: *const c_char,
    out: *mut *const c_char,
    max_out: c_int,
) -> c_int {
    if root.is_null() || out.is_null() || max_out <= 0 {
        return 0;
    }
    let Some(path) = (unsafe { c_bytes(path) }) else {
        return 0;
    };
    let Some(items) = unsafe { &*root }.str_list(path) else {
        return 0;
    };
    let count = items.len().min(max_out as usize);
    let out = unsafe { slice::from_raw_parts_mut(out, count) };
    for (dst, item) in out.iter_mut().zip(items.into_iter().take(count)) {
        *dst = item.as_ptr().cast::<c_char>();
    }
    count as c_int
}

#[no_mangle]
/// # Safety
///
/// `root` 與 `path` contract 同 `cbm_rs_yaml_get_str`。
pub unsafe extern "C" fn cbm_rs_yaml_has(root: *const YamlNode, path: *const c_char) -> bool {
    if root.is_null() {
        return false;
    }
    let Some(path) = (unsafe { c_bytes(path) }) else {
        return false;
    };
    unsafe { &*root }.has(path)
}

#[cfg(feature = "yaml-only")]
#[no_mangle]
/// # Safety
///
/// 參數契約同 `cbm_rs_yaml_parse`；回傳的 handle 必須以 `cbm_yaml_free` 釋放。
pub unsafe extern "C" fn cbm_yaml_parse(text: *const c_char, len: c_int) -> *mut YamlNode {
    unsafe { cbm_rs_yaml_parse(text.cast::<u8>(), len) }
}

#[cfg(feature = "yaml-only")]
#[no_mangle]
/// # Safety
///
/// `root` 必須是 null，或是 direct `cbm_yaml_parse` 回傳且尚未 free 的 handle。
pub unsafe extern "C" fn cbm_yaml_free(root: *mut YamlNode) {
    unsafe { cbm_rs_yaml_free(root) }
}

#[cfg(feature = "yaml-only")]
#[no_mangle]
/// # Safety
///
/// `root` 與 `path` 契約同 `cbm_rs_yaml_get_str`；回傳字串借用 root。
pub unsafe extern "C" fn cbm_yaml_get_str(
    root: *const YamlNode,
    path: *const c_char,
) -> *const c_char {
    unsafe { cbm_rs_yaml_get_str(root, path) }
}

#[cfg(feature = "yaml-only")]
#[no_mangle]
/// # Safety
///
/// `root` 與 `path` 契約同 `cbm_rs_yaml_get_float`。
pub unsafe extern "C" fn cbm_yaml_get_float(
    root: *const YamlNode,
    path: *const c_char,
    default_val: f64,
) -> f64 {
    unsafe { cbm_rs_yaml_get_float(root, path, default_val) }
}

#[cfg(feature = "yaml-only")]
#[no_mangle]
/// # Safety
///
/// `root` 與 `path` 契約同 `cbm_rs_yaml_get_bool`。
pub unsafe extern "C" fn cbm_yaml_get_bool(
    root: *const YamlNode,
    path: *const c_char,
    default_val: bool,
) -> bool {
    unsafe { cbm_rs_yaml_get_bool(root, path, default_val) }
}

#[cfg(feature = "yaml-only")]
#[no_mangle]
/// # Safety
///
/// `root`、`path` 與 `out` 契約同 `cbm_rs_yaml_get_str_list`。
pub unsafe extern "C" fn cbm_yaml_get_str_list(
    root: *const YamlNode,
    path: *const c_char,
    out: *mut *const c_char,
    max_out: c_int,
) -> c_int {
    unsafe { cbm_rs_yaml_get_str_list(root, path, out, max_out) }
}

#[cfg(feature = "yaml-only")]
#[no_mangle]
/// # Safety
///
/// `root` 與 `path` 契約同 `cbm_rs_yaml_has`。
pub unsafe extern "C" fn cbm_yaml_has(root: *const YamlNode, path: *const c_char) -> bool {
    unsafe { cbm_rs_yaml_has(root, path) }
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
/// # Safety
///
/// `qn` 必須是 null，或指向有效 NUL 結尾 C 字串；Rust 不保存該指標。
/// 回傳 1 表示符合 C `is_test_qn()` 的既有 byte-level classifier；null 或不符合回傳 0。
pub unsafe extern "C" fn cbm_rs_registry_is_test_qn_v1(qn: *const c_char) -> c_int {
    let qn = unsafe { c_bytes(qn) };
    c_int::from(qn.is_some_and(crate::pipeline::registry::is_test_qn))
}

#[cfg(feature = "pipeline-registry-test-qn-only")]
#[no_mangle]
/// # Safety
///
/// `qn` 必須是 null，或指向有效 NUL 結尾 C 字串；Rust 不保存該指標。
pub unsafe extern "C" fn cbm_pipeline_registry_is_test_qn(qn: *const c_char) -> bool {
    let qn = unsafe { c_bytes(qn) };
    qn.is_some_and(crate::pipeline::registry::is_test_qn)
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
/// `ram_fraction` 只在第一次 init 生效；後續呼叫為 no-op。
pub extern "C" fn cbm_rs_mem_init(ram_fraction: f64) {
    foundation_mem::init(ram_fraction)
}

#[no_mangle]
/// # Safety
///
/// 回傳目前 RSS（byte）；失敗時回 0。
pub extern "C" fn cbm_rs_mem_rss() -> usize {
    foundation_mem::rss()
}

#[no_mangle]
/// # Safety
///
/// 回傳 peak RSS（byte）；失敗時回傳目前 RSS 的 best-effort 值。
pub extern "C" fn cbm_rs_mem_peak_rss() -> usize {
    foundation_mem::peak_rss()
}

#[no_mangle]
/// # Safety
///
/// 回傳目前 RAM budget（byte）。
pub extern "C" fn cbm_rs_mem_budget() -> usize {
    foundation_mem::budget()
}

#[no_mangle]
/// # Safety
///
/// 回傳目前是否超過 budget；同時更新簡易 pressure hysteresis 狀態。
pub extern "C" fn cbm_rs_mem_over_budget() -> bool {
    foundation_mem::over_budget()
}

#[no_mangle]
/// # Safety
///
/// `num_workers <= 0` 視為 1，直接回傳原始 budget。
pub extern "C" fn cbm_rs_mem_worker_budget(num_workers: c_int) -> usize {
    foundation_mem::worker_budget(num_workers)
}

#[no_mangle]
/// # Safety
///
/// 回收未使用頁面（test-only 目前為 no-op）。
pub extern "C" fn cbm_rs_mem_collect() {
    foundation_mem::collect()
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

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;

    #[test]
    fn traces_http_info_ffi_preserves_url_path() {
        let method_key = CString::new("http.method").expect("有效 C 字串");
        let method = CString::new("GET").expect("有效 C 字串");
        let url_key = CString::new("url.full").expect("有效 C 字串");
        let url = CString::new("https://example.com/api/orders?page=1").expect("有效 C 字串");
        let status_key = CString::new("http.status_code").expect("有效 C 字串");
        let status = CString::new("200").expect("有效 C 字串");
        let start = CString::new("1000000000").expect("有效 C 字串");
        let end = CString::new("1050000000").expect("有效 C 字串");
        let service_name = CString::new("svc").expect("有效 C 字串");
        let attributes = [
            CbmRsTraceAttrV1 {
                key: method_key.as_ptr(),
                string_value: method.as_ptr(),
            },
            CbmRsTraceAttrV1 {
                key: url_key.as_ptr(),
                string_value: url.as_ptr(),
            },
            CbmRsTraceAttrV1 {
                key: status_key.as_ptr(),
                string_value: status.as_ptr(),
            },
        ];
        let span = CbmRsTraceSpanV1 {
            kind: 2,
            attributes: attributes.as_ptr(),
            attr_count: attributes.len() as c_int,
            start_time: start.as_ptr(),
            end_time: end.as_ptr(),
        };
        let mut out = CbmRsHttpSpanInfoV1 {
            service_name: ptr::null(),
            method: ptr::null(),
            path: ptr::null(),
            status_code: ptr::null(),
            span_kind: 0,
            duration_ns: 0,
        };

        let info = unsafe { crate::traces::extract_http_info(trace_attributes(&attributes)) };
        assert!(matches!(
            info.path,
            Some(crate::traces::HttpPath::Url(b"/api/orders"))
        ));

        let ok =
            unsafe { cbm_rs_traces_extract_http_info_v1(&span, service_name.as_ptr(), &mut out) };
        let first_url_byte = unsafe { core::ptr::addr_of!(TRACE_URL_BUF).cast::<u8>().read() };
        assert!(ok, "first_url_byte={first_url_byte}, path={:?}", out.path);
        assert_eq!(unsafe { CStr::from_ptr(out.method) }.to_bytes(), b"GET");
        assert_eq!(
            unsafe { CStr::from_ptr(out.path) }.to_bytes(),
            b"/api/orders"
        );
        assert_eq!(
            unsafe { CStr::from_ptr(out.status_code) }.to_bytes(),
            b"200"
        );
        assert_eq!(out.span_kind, 2);
        assert_eq!(out.duration_ns, 50_000_000);
    }

    #[cfg(feature = "traces-only")]
    #[test]
    fn traces_only_exports_preserve_c_abi() {
        let url = CString::new("https://example.com/api/orders?page=1").expect("有效 C 字串");
        let mut output = [0 as c_char; 32];
        let result =
            unsafe { cbm_extract_path_from_url(url.as_ptr(), output.as_mut_ptr(), output.len()) };
        assert_eq!(unsafe { CStr::from_ptr(result) }.to_bytes(), b"/api/orders");

        let mut values = [40_i64, 10, 50, 20, 30];
        assert_eq!(
            unsafe { cbm_calculate_p99(values.as_mut_ptr(), values.len() as c_int) },
            50
        );
    }
}

#[cfg(feature = "pipeline-project-name-only")]
unsafe extern "C" {
    #[link_name = "malloc"]
    fn cbm_rs_c_malloc(size: usize) -> *mut c_void;
}

/// # Safety
///
/// `abs_path` 必須是 null，或指向有效的 NUL-terminated C string。回傳值由 C
/// `malloc` 配置，呼叫端必須使用 `free()` 釋放。
#[cfg(feature = "pipeline-project-name-only")]
#[no_mangle]
pub unsafe extern "C" fn cbm_project_name_from_path(abs_path: *const c_char) -> *mut c_char {
    let output = pipeline_fqn::project_name_from_path_bytes(unsafe { c_bytes(abs_path) });
    let Some(allocation_size) = output.len().checked_add(1) else {
        return ptr::null_mut();
    };
    let result = unsafe { cbm_rs_c_malloc(allocation_size) }.cast::<c_char>();
    if result.is_null() {
        return ptr::null_mut();
    }
    unsafe {
        ptr::copy_nonoverlapping(output.as_ptr(), result.cast::<u8>(), output.len());
        *result.add(output.len()) = 0;
    }
    result
}

/// # Safety
///
/// `buf`、`repo_path` 與 `name` 必須非 null；`buf` 必須指向至少 `bufsize` bytes 的可寫
/// 記憶體，`repo_path` 與 `name` 必須指向有效的 NUL-terminated C string。輸出放不下時會
/// 寫入可容納的 raw bytes 前綴與 NUL 結尾，但回傳 `false`。
#[cfg(feature = "pipeline-artifact-path-only")]
#[no_mangle]
pub unsafe extern "C" fn cbm_pipeline_artifact_path(
    buf: *mut c_char,
    bufsize: usize,
    repo_path: *const c_char,
    name: *const c_char,
) -> bool {
    if buf.is_null() || bufsize == 0 {
        return false;
    }
    let Some(output) =
        pipeline_artifact::artifact_path(unsafe { c_bytes(repo_path) }, unsafe { c_bytes(name) })
    else {
        return false;
    };
    if output.len() >= bufsize {
        unsafe {
            write_artifact_path_prefix(buf, bufsize, output.as_ref());
        }
        return false;
    }
    unsafe {
        write_artifact_path_prefix(buf, bufsize, output.as_ref());
    }
    true
}

/// # Safety
///
/// `path` 必須是 null，或指向有效的 NUL-terminated C string。
#[cfg(feature = "pipeline-test-detect-only")]
#[no_mangle]
pub unsafe extern "C" fn cbm_is_test_path(path: *const c_char) -> bool {
    pipeline_test_detect::is_test_path(unsafe { c_bytes(path) })
}

/// # Safety
///
/// `name` 必須是 null，或指向有效的 NUL-terminated C string。
#[cfg(feature = "pipeline-test-detect-only")]
#[no_mangle]
pub unsafe extern "C" fn cbm_is_test_func_name(name: *const c_char) -> bool {
    pipeline_test_detect::is_test_func_name(unsafe { c_bytes(name) })
}

#[no_mangle]
/// # Safety
///
/// `a` 與 `b` 必須是 null 或各自指向至少 `CBM_MINHASH_K` 個 `uint32_t` 的有效陣列。
pub unsafe extern "C" fn cbm_rs_simhash_jaccard_v1(a: *const u32, b: *const u32) -> f64 {
    let left = if a.is_null() {
        None
    } else {
        Some(unsafe { std::slice::from_raw_parts(a, crate::simhash::MINHASH_K) })
    };
    let right = if b.is_null() {
        None
    } else {
        Some(unsafe { std::slice::from_raw_parts(b, crate::simhash::MINHASH_K) })
    };
    crate::simhash::jaccard(left, right)
}

#[no_mangle]
/// # Safety
///
/// `fp` 必須是 null 或至少包含 `MINHASH_K` 個 `uint32_t`；`buf` 為可寫的 C buffer。
pub unsafe extern "C" fn cbm_rs_simhash_to_hex_v1(
    fp: *const u32,
    buf: *mut std::os::raw::c_char,
    bufsize: std::os::raw::c_int,
) {
    if !buf.is_null() && bufsize > 0 {
        unsafe { *buf = 0 };
    }
    if fp.is_null() || buf.is_null() || bufsize < 0 {
        return;
    }

    let values = unsafe { std::slice::from_raw_parts(fp, crate::simhash::MINHASH_K) };
    let Some(output) = crate::simhash::to_hex(Some(values), bufsize as usize) else {
        return;
    };
    unsafe {
        std::ptr::copy_nonoverlapping(
            output.as_ptr().cast::<std::os::raw::c_char>(),
            buf,
            output.len(),
        );
    }
}

#[no_mangle]
/// # Safety
///
/// `hex` 必須是 null 或有效的 NUL 結尾 C string；`out` 必須是可寫的 64 槽 `uint32_t` 陣列。
pub unsafe extern "C" fn cbm_rs_simhash_from_hex_v1(
    hex: *const std::os::raw::c_char,
    out: *mut u32,
) -> std::os::raw::c_int {
    if hex.is_null() || out.is_null() {
        return 0;
    }
    let bytes = unsafe { std::ffi::CStr::from_ptr(hex) }.to_bytes();
    let Some(values) = crate::simhash::from_hex(Some(bytes)) else {
        return 0;
    };
    unsafe {
        std::ptr::copy_nonoverlapping(values.as_ptr(), out, crate::simhash::MINHASH_K);
    }
    1
}

#[cfg(feature = "simhash-jaccard-only")]
#[repr(C)]
pub struct CbmMinhashT {
    pub values: [u32; crate::simhash::MINHASH_K],
}

#[cfg(feature = "simhash-jaccard-only")]
#[no_mangle]
/// # Safety
///
/// 與 public C `cbm_minhash_jaccard` 相同契約；direct-only 同名 ABI。
pub unsafe extern "C" fn cbm_minhash_jaccard(a: *const CbmMinhashT, b: *const CbmMinhashT) -> f64 {
    let left = if a.is_null() {
        std::ptr::null()
    } else {
        unsafe { (*a).values.as_ptr() }
    };
    let right = if b.is_null() {
        std::ptr::null()
    } else {
        unsafe { (*b).values.as_ptr() }
    };
    unsafe { cbm_rs_simhash_jaccard_v1(left, right) }
}

#[cfg(feature = "simhash-jaccard-only")]
#[no_mangle]
/// # Safety
///
/// 與 public C `cbm_minhash_to_hex` 相同契約；direct-only 同名 ABI。
pub unsafe extern "C" fn cbm_minhash_to_hex(
    fp: *const CbmMinhashT,
    buf: *mut std::os::raw::c_char,
    bufsize: std::os::raw::c_int,
) {
    let values = if fp.is_null() {
        std::ptr::null()
    } else {
        unsafe { (*fp).values.as_ptr() }
    };
    unsafe { cbm_rs_simhash_to_hex_v1(values, buf, bufsize) }
}

#[cfg(feature = "simhash-jaccard-only")]
#[no_mangle]
/// # Safety
///
/// 與 public C `cbm_minhash_from_hex` 相同契約；direct-only 同名 ABI。
pub unsafe extern "C" fn cbm_minhash_from_hex(
    hex: *const std::os::raw::c_char,
    out: *mut CbmMinhashT,
) -> bool {
    if out.is_null() {
        return false;
    }
    unsafe { cbm_rs_simhash_from_hex_v1(hex, (*out).values.as_mut_ptr()) != 0 }
}

#[no_mangle]
/// # Safety
///
/// `json` 與 `key` 必須是 null 或有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_complexity_json_int_v1(
    json: *const std::os::raw::c_char,
    key: *const std::os::raw::c_char,
    default: std::os::raw::c_int,
) -> std::os::raw::c_int {
    let json = if json.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(json) }.to_bytes())
    };
    let key = if key.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(key) }.to_bytes())
    };
    crate::pipeline_complexity::json_get_int(json, key, default)
}

#[no_mangle]
/// # Safety
///
/// `json` 與 `key` 必須是 null 或有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_complexity_json_bool_v1(
    json: *const std::os::raw::c_char,
    key: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    let json = if json.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(json) }.to_bytes())
    };
    let key = if key.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(key) }.to_bytes())
    };
    crate::pipeline_complexity::json_get_bool(json, key) as std::os::raw::c_int
}

#[no_mangle]
pub extern "C" fn cbm_rs_vmem_round_to_page_v1(size: usize, page_size: usize) -> usize {
    crate::vmem::round_to_page(size, page_size)
}

#[no_mangle]
/// # Safety
///
/// `source` 必須指向至少 `available` 個 byte；`prefix` 必須指向至少 `prefix_len` 個 byte。
pub unsafe extern "C" fn cbm_rs_pipeline_pkgmap_at_prefix_v1(
    source: *const std::os::raw::c_char,
    available: usize,
    prefix: *const std::os::raw::c_char,
    prefix_len: std::os::raw::c_int,
) -> std::os::raw::c_int {
    if source.is_null() || prefix.is_null() || prefix_len < 0 {
        return 0;
    }
    let source = unsafe { std::slice::from_raw_parts(source.cast::<u8>(), available) };
    let prefix = unsafe { std::slice::from_raw_parts(prefix.cast::<u8>(), prefix_len as usize) };
    crate::pipeline_pkgmap::at_prefix(Some(source), Some(prefix), prefix_len as usize) as _
}

#[no_mangle]
/// # Safety
///
/// `source` 必須指向至少 `source_len` 個 byte；`prefix` 必須是有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_pkgmap_find_line_value_offset_v1(
    source: *const std::os::raw::c_char,
    source_len: std::os::raw::c_int,
    prefix: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    if source.is_null() || prefix.is_null() || source_len < 0 {
        return -1;
    }
    let source = unsafe { std::slice::from_raw_parts(source.cast::<u8>(), source_len as usize) };
    let prefix = unsafe { std::ffi::CStr::from_ptr(prefix) }.to_bytes();
    crate::pipeline_pkgmap::find_line_value_offset(Some(source), Some(prefix), source.len())
        .and_then(|offset| std::os::raw::c_int::try_from(offset).ok())
        .unwrap_or(-1)
}

unsafe fn cbm_rs_pipeline_pkgmap_copy_path_output(
    output: &[u8],
    buf: *mut std::os::raw::c_char,
    bufsize: usize,
) -> usize {
    if !buf.is_null() && bufsize > 0 {
        let copy_len = output.len().min(bufsize - 1);
        unsafe {
            std::ptr::copy_nonoverlapping(output.as_ptr().cast(), buf.cast::<u8>(), copy_len);
            *buf.add(copy_len) = 0;
        }
    }
    output.len()
}

#[no_mangle]
/// # Safety
///
/// `path` 必須是 null 或有效的 NUL 結尾 C string；`buf` 在非零大小時必須可寫。
pub unsafe extern "C" fn cbm_rs_pipeline_pkgmap_path_dirname_v1(
    buf: *mut std::os::raw::c_char,
    bufsize: usize,
    path: *const std::os::raw::c_char,
) -> usize {
    let path = if path.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(path) }.to_bytes())
    };
    let output = crate::pipeline_pkgmap::path_dirname(path);
    unsafe { cbm_rs_pipeline_pkgmap_copy_path_output(&output, buf, bufsize) }
}

#[no_mangle]
/// # Safety
///
/// `path` 必須是 null 或有效的 NUL 結尾 C string；`buf` 在非零大小時必須可寫。
pub unsafe extern "C" fn cbm_rs_pipeline_pkgmap_strip_extension_v1(
    buf: *mut std::os::raw::c_char,
    bufsize: usize,
    path: *const std::os::raw::c_char,
) -> usize {
    let path = if path.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(path) }.to_bytes())
    };
    let output = crate::pipeline_pkgmap::strip_extension(path);
    unsafe { cbm_rs_pipeline_pkgmap_copy_path_output(&output, buf, bufsize) }
}

#[no_mangle]
/// # Safety
///
/// `basename` 必須是 null 或有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_configlink_is_manifest_file_v1(
    basename: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    let basename = if basename.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(basename) }.to_bytes())
    };
    crate::pipeline_configlink::is_manifest_file(basename) as std::os::raw::c_int
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null 或有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_configlink_is_dep_section_v1(
    value: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    let value = if value.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(value) }.to_bytes())
    };
    crate::pipeline_configlink::is_dep_section(value) as std::os::raw::c_int
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null 或有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_configlink_is_cargo_dep_section_v1(
    value: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    let value = if value.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(value) }.to_bytes())
    };
    crate::pipeline_configlink::is_cargo_dep_section(value) as std::os::raw::c_int
}

#[no_mangle]
/// # Safety
///
/// 所有非 null 字串指標必須指向有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_configlink_match_dep_to_import_v1(
    target_name: *const std::os::raw::c_char,
    target_qn: *const std::os::raw::c_char,
    dep_lower: *const std::os::raw::c_char,
) -> f64 {
    let target_name = if target_name.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(target_name) }.to_bytes())
    };
    let target_qn = if target_qn.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(target_qn) }.to_bytes())
    };
    let dep_lower = if dep_lower.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(dep_lower) }.to_bytes())
    };
    crate::pipeline_configlink::match_dep_to_import(target_name, target_qn, dep_lower)
}

unsafe extern "C" {
    #[link_name = "malloc"]
    fn cbm_rs_malloc(size: usize) -> *mut std::ffi::c_void;
}

#[no_mangle]
/// # Safety
///
/// `cmd` 必須是有效的 NUL 結尾 C string；`out` 必須能寫入至少 `max_out` 個指標。
pub unsafe extern "C" fn cbm_rs_pipeline_split_command_v1(
    cmd: *const std::os::raw::c_char,
    out: *mut *mut std::os::raw::c_char,
    max_out: std::os::raw::c_int,
) -> std::os::raw::c_int {
    if cmd.is_null() || out.is_null() || max_out <= 0 {
        return 0;
    }
    let command = unsafe { std::ffi::CStr::from_ptr(cmd) }.to_bytes();
    let tokens = crate::pipeline_compile_commands::split_command(Some(command), max_out as usize);
    for (index, token) in tokens.iter().enumerate() {
        let Some(size) = token.len().checked_add(1) else {
            continue;
        };
        let allocated = unsafe { cbm_rs_malloc(size) }.cast::<u8>();
        if allocated.is_null() {
            unsafe { *out.add(index) = std::ptr::null_mut() };
            continue;
        }
        unsafe {
            std::ptr::copy_nonoverlapping(token.as_ptr(), allocated, token.len());
            *allocated.add(token.len()) = 0;
            *out.add(index) = allocated.cast();
        }
    }
    tokens.len() as std::os::raw::c_int
}

#[cfg(feature = "pipeline-split-command-only")]
#[no_mangle]
/// # Safety
///
/// `cmd` 必須是有效的 NUL 結尾 C string；`out` 必須能寫入至少 `max_out` 個指標。
pub unsafe extern "C" fn cbm_split_command(
    cmd: *const std::os::raw::c_char,
    out: *mut *mut std::os::raw::c_char,
    max_out: std::os::raw::c_int,
) -> std::os::raw::c_int {
    unsafe { cbm_rs_pipeline_split_command_v1(cmd, out, max_out) }
}

#[cfg(test)]
mod pipeline_split_command_ffi_tests {
    use super::cbm_rs_pipeline_split_command_v1;
    use std::ffi::{c_void, CStr};
    use std::os::raw::c_char;
    use std::ptr;

    unsafe extern "C" {
        #[link_name = "free"]
        fn cbm_rs_test_free(ptr: *mut c_void);
    }

    #[test]
    fn split_command_rejects_null_and_nonpositive_arguments() {
        let command = b"argument\0";
        let mut output = [ptr::null_mut::<c_char>(); 1];

        unsafe {
            assert_eq!(
                cbm_rs_pipeline_split_command_v1(ptr::null(), output.as_mut_ptr(), 1),
                0
            );
            assert_eq!(
                cbm_rs_pipeline_split_command_v1(command.as_ptr().cast(), ptr::null_mut(), 1),
                0
            );
            assert_eq!(
                cbm_rs_pipeline_split_command_v1(command.as_ptr().cast(), output.as_mut_ptr(), 0),
                0
            );
            assert_eq!(
                cbm_rs_pipeline_split_command_v1(command.as_ptr().cast(), output.as_mut_ptr(), -1),
                0
            );
        }
    }

    #[test]
    fn split_command_stops_at_first_nul() {
        let command = b"first second\0ignored third";
        let mut output = [ptr::null_mut::<c_char>(); 3];
        let count = unsafe {
            cbm_rs_pipeline_split_command_v1(command.as_ptr().cast(), output.as_mut_ptr(), 3)
        };

        assert_eq!(count, 2);
        let first = unsafe { CStr::from_ptr(output[0]).to_bytes().to_vec() };
        let second = unsafe { CStr::from_ptr(output[1]).to_bytes().to_vec() };
        for value in output.iter().take(count as usize) {
            unsafe { cbm_rs_test_free((*value).cast()) };
        }

        assert_eq!(first, b"first");
        assert_eq!(second, b"second");
    }
}

#[no_mangle]
/// # Safety
///
/// `json` 與 `key` 必須是 null 或有效的 NUL 結尾 C string；`buf` 在非零大小時必須可寫。
pub unsafe extern "C" fn cbm_rs_pipeline_cross_repo_json_str_prop_v1(
    json: *const std::os::raw::c_char,
    key: *const std::os::raw::c_char,
    buf: *mut std::os::raw::c_char,
    bufsz: usize,
) -> std::os::raw::c_int {
    let json = if json.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(json) }.to_bytes())
    };
    let key = if key.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(key) }.to_bytes())
    };
    let Some(output) = crate::pipeline_cross_repo::json_str_prop(json, key, bufsz) else {
        return 0;
    };
    if buf.is_null() {
        return 0;
    }
    unsafe {
        std::ptr::copy_nonoverlapping(output.as_ptr(), buf.cast::<u8>(), output.len());
        *buf.add(output.len()) = 0;
    }
    1
}

unsafe fn cbm_rs_pipeline_enrichment_write_tokens(
    tokens: Vec<Vec<u8>>,
    out: *mut *mut std::os::raw::c_char,
) -> std::os::raw::c_int {
    for (index, token) in tokens.iter().enumerate() {
        let Some(size) = token.len().checked_add(1) else {
            continue;
        };
        let allocated = unsafe { cbm_rs_malloc(size) }.cast::<u8>();
        if allocated.is_null() {
            unsafe { *out.add(index) = std::ptr::null_mut() };
            continue;
        }
        unsafe {
            std::ptr::copy_nonoverlapping(token.as_ptr(), allocated, token.len());
            *allocated.add(token.len()) = 0;
            *out.add(index) = allocated.cast();
        }
    }
    tokens.len() as std::os::raw::c_int
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null 或有效的 NUL 結尾 C string；`out` 必須能寫入至少 `max_out` 個指標。
pub unsafe extern "C" fn cbm_rs_pipeline_split_camel_case_v1(
    value: *const std::os::raw::c_char,
    out: *mut *mut std::os::raw::c_char,
    max_out: std::os::raw::c_int,
) -> std::os::raw::c_int {
    if value.is_null() || out.is_null() || max_out <= 0 {
        return 0;
    }
    let value = unsafe { std::ffi::CStr::from_ptr(value) }.to_bytes();
    let tokens = crate::pipeline_enrichment::split_camel_case(Some(value), max_out as usize);
    unsafe { cbm_rs_pipeline_enrichment_write_tokens(tokens, out) }
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null 或有效的 NUL 結尾 C string；`out` 必須能寫入至少 `max_out` 個指標。
pub unsafe extern "C" fn cbm_rs_pipeline_tokenize_decorator_v1(
    value: *const std::os::raw::c_char,
    out: *mut *mut std::os::raw::c_char,
    max_out: std::os::raw::c_int,
) -> std::os::raw::c_int {
    if value.is_null() || out.is_null() || max_out <= 0 {
        return 0;
    }
    let value = unsafe { std::ffi::CStr::from_ptr(value) }.to_bytes();
    let tokens = crate::pipeline_enrichment::tokenize_decorator(Some(value), max_out as usize);
    unsafe { cbm_rs_pipeline_enrichment_write_tokens(tokens, out) }
}

#[no_mangle]
/// # Safety
///
/// `json` 必須是 null 或有效的 NUL 結尾 C string；`buf` 在非零大小時必須可寫。
pub unsafe extern "C" fn cbm_rs_pipeline_calls_extract_local_name_v1(
    buf: *mut std::os::raw::c_char,
    bufsize: usize,
    json: *const std::os::raw::c_char,
) -> usize {
    let json = if json.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(json) }.to_bytes())
    };
    let Some(output) = crate::pipeline_calls::extract_local_name(json) else {
        return usize::MAX;
    };
    if !buf.is_null() && bufsize > 0 {
        let copy_len = output.len().min(bufsize - 1);
        unsafe {
            std::ptr::copy_nonoverlapping(output.as_ptr(), buf.cast::<u8>(), copy_len);
            *buf.add(copy_len) = 0;
        }
    }
    output.len()
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null 或有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_envscan_is_dockerfile_name_v1(
    name: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    let name = if name.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(name) }.to_bytes())
    };
    crate::pipeline_envscan::is_dockerfile_name(name) as std::os::raw::c_int
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null 或有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_envscan_is_env_file_name_v1(
    name: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    let name = if name.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(name) }.to_bytes())
    };
    crate::pipeline_envscan::is_env_file_name(name) as std::os::raw::c_int
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null 或有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_envscan_is_secret_file_v1(
    name: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    let name = if name.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(name) }.to_bytes())
    };
    crate::pipeline_envscan::is_secret_file(name) as std::os::raw::c_int
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null 或有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_envscan_is_ignored_dir_v1(
    name: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    let name = if name.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(name) }.to_bytes())
    };
    crate::pipeline_envscan::is_ignored_dir(name) as std::os::raw::c_int
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null 或有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_envscan_detect_file_type_v1(
    name: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    let name = if name.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(name) }.to_bytes())
    };
    crate::pipeline_envscan::detect_file_type(name)
}

#[cfg(feature = "pipeline-envscan-classifiers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_envscan_is_dockerfile_name(
    name: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    unsafe { cbm_rs_pipeline_envscan_is_dockerfile_name_v1(name) }
}

#[cfg(feature = "pipeline-envscan-classifiers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_envscan_is_env_file_name(
    name: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    unsafe { cbm_rs_pipeline_envscan_is_env_file_name_v1(name) }
}

#[cfg(feature = "pipeline-envscan-classifiers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_envscan_is_secret_file(
    name: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    unsafe { cbm_rs_pipeline_envscan_is_secret_file_v1(name) }
}

#[cfg(feature = "pipeline-envscan-classifiers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_envscan_is_ignored_dir(
    name: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    unsafe { cbm_rs_pipeline_envscan_is_ignored_dir_v1(name) }
}

#[cfg(feature = "pipeline-envscan-classifiers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_envscan_detect_file_type(
    name: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    unsafe { cbm_rs_pipeline_envscan_detect_file_type_v1(name) }
}

#[no_mangle]
/// # Safety
///
/// `rel_path` 必須是 null 或有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_lsp_ts_modes_v1(
    language: std::os::raw::c_int,
    rel_path: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    let rel_path = if rel_path.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(rel_path) }.to_bytes())
    };
    crate::pipeline_lsp_cross::ts_modes(language, rel_path)
}

#[cfg(feature = "pipeline-lsp-classifiers-only")]
#[no_mangle]
/// # Safety
///
/// rel_path 必須是 null 或有效的 NUL 結尾 C string；三個輸出指標都必須可寫。
/// null 輸出指標維持既有 C API 的未定義行為。
pub unsafe extern "C" fn cbm_pxc_ts_modes(
    language: c_int,
    rel_path: *const c_char,
    out_js: *mut bool,
    out_jsx: *mut bool,
    out_dts: *mut bool,
) {
    let rel_path = if rel_path.is_null() {
        None
    } else {
        Some(unsafe { CStr::from_ptr(rel_path) }.to_bytes())
    };
    let modes = crate::pipeline_lsp_cross::ts_modes(language, rel_path);
    unsafe {
        std::ptr::write(out_js, modes & crate::pipeline_lsp_cross::MODE_JS != 0);
        std::ptr::write(out_jsx, modes & crate::pipeline_lsp_cross::MODE_JSX != 0);
        std::ptr::write(out_dts, modes & crate::pipeline_lsp_cross::MODE_DTS != 0);
    }
}

#[no_mangle]
pub extern "C" fn cbm_rs_pipeline_lsp_has_cross_lsp_v1(
    language: std::os::raw::c_int,
) -> std::os::raw::c_int {
    crate::pipeline_lsp_cross::has_cross_lsp(language) as std::os::raw::c_int
}

#[cfg(feature = "pipeline-lsp-classifiers-only")]
#[no_mangle]
pub extern "C" fn cbm_pxc_has_cross_lsp(language: c_int) -> bool {
    crate::pipeline_lsp_cross::has_cross_lsp(language)
}

#[no_mangle]
/// # Safety
///
/// `label` 必須是 null 或有效的 NUL 結尾 C string。
pub unsafe extern "C" fn cbm_rs_pipeline_lsp_map_label_allowed_v1(
    label: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    let label = if label.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(label) }.to_bytes())
    };
    crate::pipeline_lsp_cross::map_label_allowed(label) as std::os::raw::c_int
}

#[cfg(feature = "pipeline-lsp-classifiers-only")]
#[no_mangle]
/// # Safety
///
/// label 必須是 null 或有效的 NUL 結尾 C string。接受時回傳輸入的借用指標；
/// 呼叫端必須維持其生命週期。
pub unsafe extern "C" fn cbm_pxc_map_label(label: *const c_char) -> *const c_char {
    if label.is_null() {
        return std::ptr::null();
    }
    let label_bytes = unsafe { CStr::from_ptr(label) }.to_bytes();
    if crate::pipeline_lsp_cross::map_label_allowed(Some(label_bytes)) {
        label
    } else {
        std::ptr::null()
    }
}

#[no_mangle]
/// # Safety
///
/// `json` 與 `key` 必須是 null 或有效的 NUL 結尾 C string；`buf` 在非零大小時必須可寫。
pub unsafe extern "C" fn cbm_rs_pipeline_semantic_json_str_value_v1(
    json: *const std::os::raw::c_char,
    key: *const std::os::raw::c_char,
    buf: *mut std::os::raw::c_char,
    bufsize: std::os::raw::c_int,
) -> std::os::raw::c_int {
    if bufsize <= 0 || buf.is_null() {
        return 0;
    }
    let json = if json.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(json) }.to_bytes())
    };
    let key = if key.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(key) }.to_bytes())
    };
    let Some(output) = crate::pipeline_semantic_edges::json_str_value(json, key, bufsize as usize)
    else {
        return 0;
    };
    unsafe {
        std::ptr::copy_nonoverlapping(output.as_ptr(), buf.cast::<u8>(), output.len());
        *buf.add(output.len()) = 0;
    }
    1
}

#[no_mangle]
/// # Safety
///
/// `json` 與 `key` 必須是 null 或有效的 NUL 結尾 C string；`out` 必須能寫入 `max_out` 指標。
pub unsafe extern "C" fn cbm_rs_pipeline_semantic_json_str_array_v1(
    json: *const std::os::raw::c_char,
    key: *const std::os::raw::c_char,
    out: *mut *mut std::os::raw::c_char,
    max_out: std::os::raw::c_int,
) -> std::os::raw::c_int {
    if out.is_null() || max_out <= 0 {
        return 0;
    }
    let json = if json.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(json) }.to_bytes())
    };
    let key = if key.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(key) }.to_bytes())
    };
    let values = crate::pipeline_semantic_edges::json_str_array(json, key, max_out as usize);
    for (index, value) in values.iter().enumerate() {
        let Some(size) = value.len().checked_add(1) else {
            continue;
        };
        let allocated = unsafe { cbm_rs_malloc(size) }.cast::<u8>();
        if allocated.is_null() {
            unsafe { *out.add(index) = std::ptr::null_mut() };
            continue;
        }
        unsafe {
            std::ptr::copy_nonoverlapping(value.as_ptr(), allocated, value.len());
            *allocated.add(value.len()) = 0;
            *out.add(index) = allocated.cast();
        }
    }
    values.len() as std::os::raw::c_int
}

#[no_mangle]
/// # Safety
///
/// `dir` 與 `entry` 必須是 null 或有效的 NUL 結尾 C string；`buf` 在非零大小時必須可寫。
pub unsafe extern "C" fn cbm_rs_pipeline_pkgmap_join_and_strip_v1(
    buf: *mut std::os::raw::c_char,
    bufsize: usize,
    dir: *const std::os::raw::c_char,
    entry: *const std::os::raw::c_char,
) -> usize {
    let dir = if dir.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(dir) }.to_bytes())
    };
    let entry = if entry.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(entry) }.to_bytes())
    };
    let Some(output) = crate::pipeline_pkgmap::join_and_strip(dir, entry) else {
        return usize::MAX;
    };
    unsafe { cbm_rs_pipeline_pkgmap_copy_path_output(&output, buf, bufsize) }
}

#[no_mangle]
/// # Safety
///
/// `rel_path` 與 `suffix` 必須是 null 或有效的 NUL 結尾 C string；`buf` 在非零大小時必須可寫。
pub unsafe extern "C" fn cbm_rs_pipeline_pkgmap_build_entry_path_v1(
    buf: *mut std::os::raw::c_char,
    bufsize: usize,
    rel_path: *const std::os::raw::c_char,
    suffix: *const std::os::raw::c_char,
) -> usize {
    let rel_path = if rel_path.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(rel_path) }.to_bytes())
    };
    let suffix = if suffix.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(suffix) }.to_bytes())
    };
    let output = crate::pipeline_pkgmap::build_entry_path(rel_path, suffix);
    unsafe { cbm_rs_pipeline_pkgmap_copy_path_output(&output, buf, bufsize) }
}

#[no_mangle]
/// # Safety
///
/// `value` 必須是 null 或有效的 NUL 結尾 C string；`buf` 在非零大小時必須可寫。
pub unsafe extern "C" fn cbm_rs_pipeline_configlink_lowercase_into_v1(
    value: *const std::os::raw::c_char,
    buf: *mut std::os::raw::c_char,
    bufsize: usize,
) {
    if buf.is_null() || bufsize == 0 {
        return;
    }
    let value = if value.is_null() {
        None
    } else {
        Some(unsafe { std::ffi::CStr::from_ptr(value) }.to_bytes())
    };
    let output = crate::pipeline_configlink::lowercase_into(value, bufsize);
    unsafe {
        std::ptr::copy_nonoverlapping(output.as_ptr(), buf.cast::<u8>(), output.len());
        *buf.add(output.len()) = 0;
    }
}

#[cfg(feature = "pipeline-configlink-helpers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_configlink_is_manifest_file(
    basename: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    unsafe { cbm_rs_pipeline_configlink_is_manifest_file_v1(basename) }
}

#[cfg(feature = "pipeline-configlink-helpers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_configlink_is_dep_section(
    value: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    unsafe { cbm_rs_pipeline_configlink_is_dep_section_v1(value) }
}

#[cfg(feature = "pipeline-configlink-helpers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_configlink_is_cargo_dep_section(
    qn: *const std::os::raw::c_char,
) -> std::os::raw::c_int {
    unsafe { cbm_rs_pipeline_configlink_is_cargo_dep_section_v1(qn) }
}

#[cfg(feature = "pipeline-configlink-helpers-only")]
#[no_mangle]
/// # Safety
///
/// Public C 參數順序 (buf, bufsize, src)；內部轉調 v1 (src, buf, bufsize)。
pub unsafe extern "C" fn cbm_pipeline_configlink_lowercase_into(
    buf: *mut std::os::raw::c_char,
    bufsize: usize,
    src: *const std::os::raw::c_char,
) {
    unsafe { cbm_rs_pipeline_configlink_lowercase_into_v1(src, buf, bufsize) }
}

#[cfg(feature = "pipeline-configlink-helpers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_configlink_match_dep_to_import(
    target_name: *const std::os::raw::c_char,
    target_qn: *const std::os::raw::c_char,
    dep_lower: *const std::os::raw::c_char,
) -> f64 {
    unsafe { cbm_rs_pipeline_configlink_match_dep_to_import_v1(target_name, target_qn, dep_lower) }
}

#[no_mangle]
/// # Safety
///
/// 所有非 null 字串指標必須指向有效的 NUL 結尾 C string；`buf` 在非零大小時必須可寫。
pub unsafe extern "C" fn cbm_rs_pipeline_cross_repo_build_props_v1(
    buf: *mut std::os::raw::c_char,
    bufsz: usize,
    target_project: *const std::os::raw::c_char,
    target_function: *const std::os::raw::c_char,
    target_file: *const std::os::raw::c_char,
    url_or_channel: *const std::os::raw::c_char,
    extra_key: *const std::os::raw::c_char,
    extra_val: *const std::os::raw::c_char,
) -> usize {
    let read = |value: *const std::os::raw::c_char| {
        if value.is_null() {
            None
        } else {
            Some(unsafe { std::ffi::CStr::from_ptr(value) }.to_bytes())
        }
    };
    let output = crate::pipeline_cross_repo::build_cross_props(
        read(target_project),
        read(target_function),
        read(target_file),
        read(url_or_channel),
        read(extra_key),
        read(extra_val),
        bufsz,
    );
    if !buf.is_null() && bufsz > 0 {
        unsafe {
            std::ptr::copy_nonoverlapping(output.as_ptr(), buf.cast::<u8>(), output.len());
            *buf.add(output.len()) = 0;
        }
    }
    output.len()
}

#[no_mangle]
/// # Safety
///
/// `path` 與 `directory` 必須是 null 或有效的 NUL 結尾 C string；`buf` 在非零大小時必須可寫。
pub unsafe extern "C" fn cbm_rs_pipeline_resolve_compile_path_v1(
    buf: *mut std::os::raw::c_char,
    bufsize: usize,
    path: *const std::os::raw::c_char,
    directory: *const std::os::raw::c_char,
) -> usize {
    let read = |value: *const std::os::raw::c_char| {
        if value.is_null() {
            None
        } else {
            Some(unsafe { std::ffi::CStr::from_ptr(value) }.to_bytes())
        }
    };
    let Some(output) = crate::pipeline_compile_commands::resolve_path(read(path), read(directory))
    else {
        return usize::MAX;
    };
    if !buf.is_null() && bufsize > 0 {
        let copy_len = output.len().min(bufsize - 1);
        unsafe {
            std::ptr::copy_nonoverlapping(output.as_ptr(), buf.cast::<u8>(), copy_len);
            *buf.add(copy_len) = 0;
        }
    }
    output.len()
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_definitions_json_escape_char(
    output: *mut c_char,
    available: usize,
    byte: c_int,
) -> c_int {
    let (length, encoded) = crate::pipeline_definitions::json_escape_char(byte as u8);
    if !output.is_null() && available >= length {
        unsafe {
            std::ptr::copy_nonoverlapping(encoded.as_ptr().cast::<c_char>(), output, length);
        }
    }
    length as c_int
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_definitions_json_escaped_len(
    value: *const c_char,
) -> usize {
    crate::pipeline_definitions::json_escaped_len(unsafe { c_bytes(value) })
}

#[cfg(feature = "pipeline-definitions-json-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守與 v1 相同的 C ABI 指標與緩衝區契約。
pub unsafe extern "C" fn cbm_pipeline_definitions_json_escape_char(
    output: *mut c_char,
    available: usize,
    byte: c_int,
) -> c_int {
    unsafe { cbm_rs_pipeline_definitions_json_escape_char(output, available, byte) }
}

#[cfg(feature = "pipeline-definitions-json-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守與 v1 相同的 nullable NUL 字串契約。
pub unsafe extern "C" fn cbm_pipeline_definitions_json_escaped_len(value: *const c_char) -> usize {
    unsafe { cbm_rs_pipeline_definitions_json_escaped_len(value) }
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_similarity_parse_fp_v1(
    props_json: *const c_char,
    output: *mut u32,
) -> c_int {
    if props_json.is_null() || output.is_null() {
        return 0;
    }
    let Some(values) = crate::simhash::parse_fp_from_props(unsafe { c_bytes(props_json) }) else {
        return 0;
    };
    unsafe {
        std::ptr::copy_nonoverlapping(values.as_ptr(), output, crate::simhash::MINHASH_K);
    }
    1
}

#[cfg(feature = "pipeline-similarity-fp-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須提供至少 64 個 `uint32_t` 的可寫輸出陣列。
pub unsafe extern "C" fn cbm_pipeline_similarity_parse_fp(
    props_json: *const c_char,
    output: *mut u32,
) -> c_int {
    unsafe { cbm_rs_pipeline_similarity_parse_fp_v1(props_json, output) }
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_cli_archive_tar_end_v1(header: *const u8) -> c_int {
    if header.is_null() {
        return 0;
    }
    let bytes = unsafe { std::slice::from_raw_parts(header, crate::cli_archive::TAR_BLOCK_SIZE) };
    crate::cli_archive::is_tar_end_of_archive(Some(bytes)) as c_int
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_cli_archive_zip_read_u16_v1(data: *const u8, offset: c_int) -> u16 {
    if data.is_null() || offset < 0 {
        return 0;
    }
    let bytes = unsafe { std::slice::from_raw_parts(data.add(offset as usize), 2) };
    crate::cli_archive::zip_read_u16(Some(bytes), 0).unwrap_or(0)
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_cli_archive_zip_read_u32_v1(data: *const u8, offset: c_int) -> u32 {
    if data.is_null() || offset < 0 {
        return 0;
    }
    let bytes = unsafe { std::slice::from_raw_parts(data.add(offset as usize), 4) };
    crate::cli_archive::zip_read_u32(Some(bytes), 0).unwrap_or(0)
}

#[cfg(feature = "cli-archive-helpers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 匯出同名公開 bridge。
pub unsafe extern "C" fn cbm_cli_is_tar_end_of_archive(hdr: *const u8) -> bool {
    unsafe { cbm_rs_cli_archive_tar_end_v1(hdr) != 0 }
}

#[cfg(feature = "cli-archive-helpers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 匯出同名公開 bridge。
pub unsafe extern "C" fn cbm_cli_zip_read_u16(data: *const u8, off: c_int) -> u16 {
    unsafe { cbm_rs_cli_archive_zip_read_u16_v1(data, off) }
}

#[cfg(feature = "cli-archive-helpers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 匯出同名公開 bridge。
pub unsafe extern "C" fn cbm_cli_zip_read_u32(data: *const u8, off: c_int) -> u32 {
    unsafe { cbm_rs_cli_archive_zip_read_u32_v1(data, off) }
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_k8s_basename_offset_v1(path: *const c_char) -> usize {
    crate::pipeline_k8s::basename_offset(unsafe { c_bytes(path) })
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_k8s_indent_v1(line: *const c_char) -> c_int {
    crate::pipeline_k8s::indent(unsafe { c_bytes(line) }) as c_int
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_k8s_split_kv_v1(
    text: *const c_char,
    key: *mut c_char,
    key_size: usize,
    value: *mut c_char,
    value_size: usize,
) -> c_int {
    if key.is_null() || value.is_null() || key_size == 0 || value_size == 0 {
        return 0;
    }
    unsafe {
        *key = 0;
        *value = 0;
    }
    let Some((key_bytes, value_bytes)) =
        crate::pipeline_k8s::split_kv(unsafe { c_bytes(text) }, key_size, value_size)
    else {
        return 0;
    };
    unsafe {
        std::ptr::copy_nonoverlapping(key_bytes.as_ptr().cast::<c_char>(), key, key_bytes.len());
        *key.add(key_bytes.len()) = 0;
        std::ptr::copy_nonoverlapping(
            value_bytes.as_ptr().cast::<c_char>(),
            value,
            value_bytes.len(),
        );
        *value.add(value_bytes.len()) = 0;
    }
    1
}

#[cfg(feature = "pipeline-k8s-helpers-only")]
#[no_mangle]
/// # Safety
///
/// path 可為 null；回傳 path 內 basename 指標（不配置）或 null。
pub unsafe extern "C" fn cbm_pipeline_k8s_basename(path: *const c_char) -> *const c_char {
    if path.is_null() {
        return std::ptr::null();
    }
    let off = crate::pipeline_k8s::basename_offset(unsafe { c_bytes(path) });
    unsafe { path.add(off) }
}

#[cfg(feature = "pipeline-k8s-helpers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 匯出同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_k8s_indent(line: *const c_char) -> c_int {
    unsafe { cbm_rs_pipeline_k8s_indent_v1(line) }
}

#[cfg(feature = "pipeline-k8s-helpers-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同契約；direct-only 匯出同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_k8s_split_kv(
    text: *const c_char,
    key: *mut c_char,
    key_sz: usize,
    val: *mut c_char,
    val_sz: usize,
) -> c_int {
    unsafe { cbm_rs_pipeline_k8s_split_kv_v1(text, key, key_sz, val, val_sz) }
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1(
    edge_type: *const c_char,
) -> c_int {
    crate::pipeline_incremental::edge_type_is_recomputed(unsafe { c_bytes(edge_type) }) as c_int
}

#[cfg(feature = "pipeline-incremental-edge-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同的 nullable NUL 字串契約；direct-only 匯出同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_incremental_edge_type_is_recomputed(
    edge_type: *const c_char,
) -> bool {
    unsafe { cbm_rs_pipeline_incremental_edge_type_is_recomputed_v1(edge_type) != 0 }
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_parallel_json_escape_char(
    output: *mut c_char,
    available: usize,
    byte: c_int,
) -> c_int {
    let (length, encoded) = crate::pipeline_json::json_escape_char(byte as u8);
    if !output.is_null() && available >= length {
        unsafe {
            std::ptr::copy_nonoverlapping(encoded.as_ptr().cast::<c_char>(), output, length);
        }
    }
    length as c_int
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_parallel_json_escaped_len(value: *const c_char) -> usize {
    crate::pipeline_json::json_escaped_len(unsafe { c_bytes(value) })
}

#[cfg(feature = "pipeline-parallel-json-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_pipeline_parallel_json_escape_char(
    output: *mut c_char,
    available: usize,
    byte: c_int,
) -> c_int {
    unsafe { cbm_rs_pipeline_parallel_json_escape_char(output, available, byte) }
}

#[cfg(feature = "pipeline-parallel-json-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_pipeline_parallel_json_escaped_len(value: *const c_char) -> usize {
    unsafe { cbm_rs_pipeline_parallel_json_escaped_len(value) }
}

#[no_mangle]
/// 回傳不可釋放的 NUL 結尾語言名稱；呼叫端應比較內容，不可依賴指標位址。
pub extern "C" fn cbm_rs_discover_language_name_v1(language: c_int) -> *const c_char {
    crate::language_name::language_name_ptr(language)
}

#[cfg(any(
    feature = "discover-language-only",
    feature = "discover-language-name-only"
))]
#[no_mangle]
/// direct-only 同名公開 ABI。
pub extern "C" fn cbm_language_name(language: c_int) -> *const c_char {
    cbm_rs_discover_language_name_v1(language)
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_discover_language_marker_v1(
    buffer: *const c_char,
    kind: c_int,
) -> c_int {
    crate::discover::language_marker(unsafe { c_bytes(buffer) }, kind) as c_int
}

#[cfg(feature = "discover-language-markers-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守既有 C ABI 的 nullable NUL 字串契約。
pub unsafe extern "C" fn cbm_discover_language_marker(buffer: *const c_char, kind: c_int) -> c_int {
    unsafe { cbm_rs_discover_language_marker_v1(buffer, kind) }
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_incremental_label_is_registry_symbol_v1(
    label: *const c_char,
) -> c_int {
    crate::pipeline_incremental::label_is_registry_symbol(unsafe { c_bytes(label) }) as c_int
}

#[cfg(feature = "pipeline-incremental-registry-label-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同的 nullable NUL 字串契約；direct-only 匯出同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_incremental_label_is_registry_symbol(
    label: *const c_char,
) -> bool {
    unsafe { cbm_rs_pipeline_incremental_label_is_registry_symbol_v1(label) != 0 }
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_semantic_fp_ends_with_v1(
    file_path: *const c_char,
    suffix: *const c_char,
) -> c_int {
    crate::discover::ends_with(unsafe { c_bytes(file_path) }, unsafe { c_bytes(suffix) }) as c_int
}

#[cfg(feature = "pipeline-semantic-fp-suffix-only")]
#[no_mangle]
/// # Safety
///
/// 與 v1 相同的 nullable NUL 字串契約；direct-only 匯出同名公開 bridge。
pub unsafe extern "C" fn cbm_pipeline_semantic_fp_ends_with(
    file_path: *const c_char,
    suffix: *const c_char,
) -> bool {
    unsafe { cbm_rs_pipeline_semantic_fp_ends_with_v1(file_path, suffix) != 0 }
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_store_arch_json_prop_len_v1(
    json: *const c_char,
    key: *const c_char,
    key_len: c_int,
) -> usize {
    if key_len < 0 {
        return usize::MAX;
    }
    crate::store::arch_helpers::extract_json_string_prop(
        unsafe { c_bytes(json) },
        unsafe { c_bytes(key) },
        key_len as usize,
    )
    .map_or(usize::MAX, |value| value.len())
}

/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
#[no_mangle]
pub unsafe extern "C" fn cbm_rs_store_arch_json_prop_copy_v1(
    output: *mut c_char,
    output_size: usize,
    json: *const c_char,
    key: *const c_char,
    key_len: c_int,
) -> usize {
    if output.is_null() || output_size == 0 || key_len < 0 {
        return usize::MAX;
    }
    let Some(value) = crate::store::arch_helpers::extract_json_string_prop(
        unsafe { c_bytes(json) },
        unsafe { c_bytes(key) },
        key_len as usize,
    ) else {
        return usize::MAX;
    };
    if value.len() >= output_size {
        return usize::MAX;
    }
    unsafe {
        std::ptr::copy_nonoverlapping(value.as_ptr().cast::<c_char>(), output, value.len());
        *output.add(value.len()) = 0;
    }
    value.len()
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_pipeline_test_node_is_test_v1(
    properties_json: *const c_char,
) -> c_int {
    c_int::from(pipeline_test_detect::node_is_test(unsafe {
        c_bytes(properties_json)
    }))
}

#[cfg(feature = "pipeline-test-node-is-test-only")]
#[no_mangle]
/// # Safety
///
/// 非 NULL 的 properties_json 必須為 NUL 結尾字串。
pub unsafe extern "C" fn cbm_pipeline_test_node_is_test(properties_json: *const c_char) -> bool {
    unsafe { cbm_rs_pipeline_test_node_is_test_v1(properties_json) != 0 }
}

#[no_mangle]
pub extern "C" fn cbm_rs_ui_layout_stellar_color_v1(degree: c_int) -> u32 {
    crate::ui_layout::stellar_color(degree)
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_ui_layout_size_for_label_v1(label: *const c_char) -> f32 {
    crate::ui_layout::size_for_label(unsafe { c_bytes(label) })
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_ui_layout_fnv1a_v1(value: *const c_char) -> u32 {
    crate::ui_layout::fnv1a(unsafe { c_bytes(value) })
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_ui_layout_rand_float_v1(seed: *mut u32) -> f32 {
    let Some(seed) = (unsafe { seed.as_mut() }) else {
        return 0.0;
    };
    crate::ui_layout::rand_float(seed)
}

#[no_mangle]
pub extern "C" fn cbm_rs_ui_layout_octant_v1(
    ox: f32,
    oy: f32,
    oz: f32,
    x: f32,
    y: f32,
    z: f32,
) -> c_int {
    crate::ui_layout::octant(ox, oy, oz, x, y, z)
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_ui_layout_child_center_v1(
    ox: f32,
    oy: f32,
    oz: f32,
    half: f32,
    child: c_int,
    cx: *mut f32,
    cy: *mut f32,
    cz: *mut f32,
) {
    let (x, y, z) = crate::ui_layout::child_center(ox, oy, oz, half, child);
    if let Some(cx) = unsafe { cx.as_mut() } {
        *cx = x;
    }
    if let Some(cy) = unsafe { cy.as_mut() } {
        *cy = y;
    }
    if let Some(cz) = unsafe { cz.as_mut() } {
        *cz = z;
    }
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_ui_layout_find_node_index_v1(
    map: *const crate::ui_layout::NodeIdEntry,
    count: c_int,
    id: i64,
) -> c_int {
    if map.is_null() || count <= 0 {
        return -1;
    }
    let entries = unsafe { std::slice::from_raw_parts(map, count as usize) };
    crate::ui_layout::find_node_index(entries, id)
}

#[no_mangle]
pub extern "C" fn cbm_rs_ui_layout_clamp_max_nodes_v1(requested: c_int, cap: c_int) -> c_int {
    crate::ui_layout::clamp_max_nodes(requested, cap)
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_ui_httpd_header_name_is_v1(
    line: *const c_char,
    name_len: usize,
    name: *const c_char,
) -> c_int {
    c_int::from(crate::ui_httpd::header_name_is(
        unsafe { c_bytes(line) },
        name_len,
        unsafe { c_bytes(name) },
    ))
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_ui_httpd_copy_header_value_v1(
    value: *const c_char,
    value_len: usize,
    out: *mut c_char,
    out_size: usize,
) -> c_int {
    if value.is_null() || (out.is_null() && out_size != 0) {
        return 0;
    }
    let input = unsafe { std::slice::from_raw_parts(value.cast::<u8>(), value_len) };
    let output = if out.is_null() {
        &mut []
    } else {
        unsafe { std::slice::from_raw_parts_mut(out.cast::<u8>(), out_size) }
    };
    c_int::from(crate::ui_httpd::copy_header_value(input, output))
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_ast_profile_kind_flags_v1(kind: *const c_char) -> u32 {
    crate::ast_profile_classifiers::kind_flags(unsafe { c_bytes(kind) })
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_ast_profile_halstead_insert_v1(
    set: *mut u32,
    key: *const c_char,
) -> c_int {
    if set.is_null() {
        return 0;
    }
    let set = unsafe { std::slice::from_raw_parts_mut(set, 512) };
    c_int::from(crate::ast_profile_classifiers::halstead_insert(
        set,
        unsafe { c_bytes(key) },
    ))
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_ast_profile_is_param_name_v1(
    ident: *const c_char,
    param_names: *const *const c_char,
    param_count: c_int,
) -> c_int {
    if param_names.is_null() || param_count <= 0 {
        return 0;
    }
    let raw_names = unsafe {
        std::slice::from_raw_parts(param_names.cast::<*const u8>(), param_count as usize)
    };
    let names = raw_names
        .iter()
        .map(|pointer| unsafe { c_bytes((*pointer).cast::<c_char>()) })
        .collect::<Vec<_>>();
    c_int::from(crate::ast_profile_classifiers::is_param_name(
        unsafe { c_bytes(ident) },
        &names,
    ))
}

#[cfg(feature = "ast-profile-classifiers-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_ast_profile_kind_flags(kind: *const c_char) -> u32 {
    crate::ast_profile_classifiers::kind_flags(unsafe { c_bytes(kind) })
}

#[cfg(feature = "ast-profile-classifiers-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須提供至少 512 個元素的可寫入集合。
pub unsafe extern "C" fn cbm_ast_profile_halstead_insert(
    set: *mut u32,
    key: *const c_char,
) -> bool {
    if set.is_null() {
        return false;
    }
    let set = unsafe { std::slice::from_raw_parts_mut(set, 512) };
    crate::ast_profile_classifiers::halstead_insert(set, unsafe { c_bytes(key) })
}

#[cfg(feature = "ast-profile-classifiers-only")]
#[no_mangle]
/// # Safety
///
/// 呼叫端必須提供至少 `param_count` 個名稱指標的陣列。
pub unsafe extern "C" fn cbm_ast_profile_is_param_name(
    ident: *const c_char,
    param_names: *const *const c_char,
    param_count: c_int,
) -> bool {
    if param_names.is_null() || param_count <= 0 {
        return false;
    }
    let raw_names = unsafe {
        std::slice::from_raw_parts(param_names.cast::<*const u8>(), param_count as usize)
    };
    let names = raw_names
        .iter()
        .map(|pointer| unsafe { c_bytes((*pointer).cast::<c_char>()) })
        .collect::<Vec<_>>();
    crate::ast_profile_classifiers::is_param_name(unsafe { c_bytes(ident) }, &names)
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_pipeline_usages_local_name_len_v1(json: *const c_char) -> usize {
    crate::pipeline_usages::extract_local_name(unsafe { c_bytes(json) })
        .map_or(usize::MAX, |value| value.len())
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_pipeline_usages_local_name_copy_v1(
    json: *const c_char,
    out: *mut c_char,
    out_size: usize,
) -> usize {
    let Some(value) = crate::pipeline_usages::extract_local_name(unsafe { c_bytes(json) }) else {
        return usize::MAX;
    };
    if out.is_null() || out_size <= value.len() {
        return usize::MAX;
    }
    unsafe {
        std::ptr::copy_nonoverlapping(value.as_ptr(), out.cast::<u8>(), value.len());
        *out.add(value.len()) = 0;
    }
    value.len()
}

#[no_mangle]
pub extern "C" fn cbm_rs_semantic_is_token_delim_v1(byte: c_int) -> c_int {
    c_int::from(crate::semantic_token_classifiers::is_token_delim(
        byte as u8,
    ))
}

#[cfg(feature = "semantic-token-delim-only")]
#[no_mangle]
/// direct-only 同名公開 bridge。
pub extern "C" fn cbm_semantic_is_token_delim(byte: u8) -> bool {
    crate::semantic_token_classifiers::is_token_delim(byte)
}

#[no_mangle]
/// # Safety
///
/// 呼叫端必須遵守此 C ABI 所定義的指標與緩衝區契約。
pub unsafe extern "C" fn cbm_rs_semantic_is_camel_break_v1(
    name: *const c_char,
    index: c_int,
) -> c_int {
    if index < 0 {
        return 0;
    }
    c_int::from(crate::semantic_token_classifiers::is_camel_break(
        unsafe { c_bytes(name) },
        index as usize,
    ))
}

#[cfg(feature = "semantic-camel-break-only")]
#[no_mangle]
/// # Safety
///
/// 非 NULL 的 name 必須指向 NUL 結尾字串。
pub unsafe extern "C" fn cbm_semantic_is_camel_break(name: *const c_char, index: c_int) -> bool {
    unsafe { cbm_rs_semantic_is_camel_break_v1(name, index) != 0 }
}
