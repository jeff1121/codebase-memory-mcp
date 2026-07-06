//! 階段性 Rust migration 的 C ABI exports。
//!
//! 這些 symbols 由 `make -f Makefile.cbm rust-ffi-test` 驗證；只有明確 opt-in
//! 的 C wrapper 會呼叫它們。

#![allow(unsafe_code)]

use std::alloc::{alloc, dealloc, Layout};
use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_long, c_void};
use std::path::Path;
use std::ptr;
use std::slice;

use crate::cypher;
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
use crate::mcp;
use crate::pipeline::fqn as pipeline_fqn;
use crate::pipeline::graph_mutation;
use crate::pipeline::plan::{self, PlanKind};
use crate::pipeline::registry::{Entry as RegistryEntry, Registry};
use crate::store::{
    arch_helpers as store_arch_helpers, open as store_open, pragmas as store_pragmas,
    schema_manifest, search_pattern as store_search_pattern, tokenization,
};

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
/// 此 API 僅描述 orchestration metadata，不供 C runtime dispatch 使用。
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
