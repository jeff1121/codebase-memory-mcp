#![allow(unsafe_code)]

//! `graph_buffer_mutation.c` 的 Rust direct-only ABI 轉接層。
//!
//! Rust 只承接 command dispatch；graph buffer 的儲存、配置與副作用仍由
//! `graph_buffer.c` 提供。這樣可以先移除一個 C translation unit，而不改變
//! graph buffer 的 ownership 或 SQLite dump 邊界。

use core::ffi::{c_char, c_int, c_void};

#[repr(C)]
pub struct CbmGbufMutationCmd {
    kind: c_int,
    reserved0: c_int,
    label: *const c_char,
    name: *const c_char,
    qualified_name: *const c_char,
    file_path: *const c_char,
    start_line: c_int,
    end_line: c_int,
    source_id: i64,
    target_id: i64,
    edge_type: *const c_char,
    properties_json: *const c_char,
}

unsafe extern "C" {
    fn cbm_gbuf_upsert_node(
        gb: *mut c_void,
        label: *const c_char,
        name: *const c_char,
        qualified_name: *const c_char,
        file_path: *const c_char,
        start_line: c_int,
        end_line: c_int,
        properties_json: *const c_char,
    ) -> i64;
    fn cbm_gbuf_insert_edge(
        gb: *mut c_void,
        source_id: i64,
        target_id: i64,
        edge_type: *const c_char,
        properties_json: *const c_char,
    ) -> i64;
    fn cbm_gbuf_delete_by_file(gb: *mut c_void, file_path: *const c_char) -> c_int;
}

const MUTATION_UPSERT_NODE: c_int = 1;
const MUTATION_INSERT_EDGE: c_int = 2;
const MUTATION_DELETE_BY_FILE: c_int = 3;
const MUTATION_ERROR: i64 = -1;

#[no_mangle]
/// # Safety
///
/// `gb` 必須是有效的 graph buffer 指標；`cmd` 必須是 null 或指向完整且仍有效的
/// `cbm_gbuf_mutation_cmd_t`。所有字串只在本次呼叫期間借用，底層 C API 負責複製
/// 需要保存的資料。
pub unsafe extern "C" fn cbm_gbuf_apply_mutation(
    gb: *mut c_void,
    cmd: *const CbmGbufMutationCmd,
) -> i64 {
    if cmd.is_null() {
        return MUTATION_ERROR;
    }

    let cmd = unsafe { &*cmd };
    if cmd.reserved0 != 0 {
        return MUTATION_ERROR;
    }

    match cmd.kind {
        MUTATION_UPSERT_NODE => unsafe {
            cbm_gbuf_upsert_node(
                gb,
                cmd.label,
                cmd.name,
                cmd.qualified_name,
                cmd.file_path,
                cmd.start_line,
                cmd.end_line,
                cmd.properties_json,
            )
        },
        MUTATION_INSERT_EDGE => unsafe {
            cbm_gbuf_insert_edge(
                gb,
                cmd.source_id,
                cmd.target_id,
                cmd.edge_type,
                cmd.properties_json,
            )
        },
        MUTATION_DELETE_BY_FILE => unsafe { cbm_gbuf_delete_by_file(gb, cmd.file_path) as i64 },
        _ => MUTATION_ERROR,
    }
}

#[no_mangle]
/// # Safety
///
/// `gb` 與字串指標必須符合 `cbm_gbuf_apply_mutation` 的生命週期契約。
pub unsafe extern "C" fn cbm_gbuf_apply_upsert_node(
    gb: *mut c_void,
    label: *const c_char,
    name: *const c_char,
    qualified_name: *const c_char,
    file_path: *const c_char,
    start_line: c_int,
    end_line: c_int,
    properties_json: *const c_char,
) -> i64 {
    let cmd = CbmGbufMutationCmd {
        kind: MUTATION_UPSERT_NODE,
        reserved0: 0,
        label,
        name,
        qualified_name,
        file_path,
        start_line,
        end_line,
        source_id: 0,
        target_id: 0,
        edge_type: core::ptr::null(),
        properties_json,
    };
    unsafe { cbm_gbuf_apply_mutation(gb, &cmd) }
}

#[no_mangle]
/// # Safety
///
/// `gb` 與字串指標必須符合 `cbm_gbuf_apply_mutation` 的生命週期契約。
pub unsafe extern "C" fn cbm_gbuf_apply_insert_edge(
    gb: *mut c_void,
    source_id: i64,
    target_id: i64,
    edge_type: *const c_char,
    properties_json: *const c_char,
) -> i64 {
    let cmd = CbmGbufMutationCmd {
        kind: MUTATION_INSERT_EDGE,
        reserved0: 0,
        label: core::ptr::null(),
        name: core::ptr::null(),
        qualified_name: core::ptr::null(),
        file_path: core::ptr::null(),
        start_line: 0,
        end_line: 0,
        source_id,
        target_id,
        edge_type,
        properties_json,
    };
    unsafe { cbm_gbuf_apply_mutation(gb, &cmd) }
}

#[no_mangle]
/// # Safety
///
/// `gb` 與 `file_path` 必須符合 `cbm_gbuf_apply_mutation` 的生命週期契約。
pub unsafe extern "C" fn cbm_gbuf_apply_delete_by_file(
    gb: *mut c_void,
    file_path: *const c_char,
) -> c_int {
    let cmd = CbmGbufMutationCmd {
        kind: MUTATION_DELETE_BY_FILE,
        reserved0: 0,
        label: core::ptr::null(),
        name: core::ptr::null(),
        qualified_name: core::ptr::null(),
        file_path,
        start_line: 0,
        end_line: 0,
        source_id: 0,
        target_id: 0,
        edge_type: core::ptr::null(),
        properties_json: core::ptr::null(),
    };
    unsafe { cbm_gbuf_apply_mutation(gb, &cmd) as c_int }
}
