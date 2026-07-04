//! graph-buffer mutation command boundary 的 parity fixture。
//!
//! 此模組只描述與驗證 C graph buffer mutation 呼叫的 command shape；不持有
//! `cbm_gbuf_t`，也不呼叫任何 C graph-buffer 實作。

#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum MutationCommandKind {
    UpsertNode = 1,
    InsertEdge = 2,
    DeleteByFile = 3,
}

#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum MutationResultKind {
    NodeId = 1,
    EdgeId = 2,
    Count = 3,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct MutationMeta {
    pub kind: MutationCommandKind,
    pub result_kind: MutationResultKind,
    pub required_fields: u32,
    pub optional_fields: u32,
    pub effect_flags: u32,
    pub validation_flags: u32,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct MutationCommandShape {
    pub kind: i32,
    pub reserved0: i32,
    // numeric 欄位是固定寬度 ABI slot，shape validation 會視為已存在。
    // C adapter 會把 endpoint lookup 與語意 ID 檢查保留在 graph-buffer 層。
    pub has_label: bool,
    pub has_name: bool,
    pub has_qualified_name: bool,
    pub has_file_path: bool,
    pub has_edge_type: bool,
    pub has_properties_json: bool,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct MutationValidation {
    pub ok: bool,
    pub error_code: i32,
    pub missing_fields: u32,
    pub invalid_fields: u32,
    pub normalized_flags: u32,
}

pub const FIELD_LABEL: u32 = 1 << 0;
pub const FIELD_NAME: u32 = 1 << 1;
pub const FIELD_QUALIFIED_NAME: u32 = 1 << 2;
pub const FIELD_FILE_PATH: u32 = 1 << 3;
pub const FIELD_START_LINE: u32 = 1 << 4;
pub const FIELD_END_LINE: u32 = 1 << 5;
pub const FIELD_SOURCE_ID: u32 = 1 << 6;
pub const FIELD_TARGET_ID: u32 = 1 << 7;
pub const FIELD_EDGE_TYPE: u32 = 1 << 8;
pub const FIELD_PROPERTIES_JSON: u32 = 1 << 9;
pub const FIELD_KIND: u32 = 1 << 30;
pub const FIELD_RESERVED: u32 = 1 << 31;

pub const EFFECT_MUTATES_GRAPH: u32 = 1 << 0;
// Effect flags 描述 command 可能造成的副作用；dedup 命中時 InsertEdge 會回傳既有 ID。
pub const EFFECT_ALLOCATES_ID: u32 = 1 << 1;
pub const EFFECT_DEDUPS: u32 = 1 << 2;
pub const EFFECT_CASCADE_EDGES: u32 = 1 << 3;
pub const EFFECT_UPDATES_EXISTING: u32 = 1 << 4;

pub const VALIDATION_REQUIRES_NON_NULL_CSTR: u32 = 1 << 0;
pub const VALIDATION_ALLOWS_EMPTY_CSTR: u32 = 1 << 1;
pub const VALIDATION_NO_ENDPOINT_LOOKUP: u32 = 1 << 2;
pub const VALIDATION_NO_JSON_PARSE: u32 = 1 << 3;

pub const NORMALIZED_OPTIONAL_NULL_CSTR: u32 = 1 << 0;

pub const ERROR_OK: i32 = 0;
pub const ERROR_UNKNOWN_KIND: i32 = 1;
pub const ERROR_MISSING_FIELD: i32 = 2;
pub const ERROR_INVALID_FIELD: i32 = 3;

const COMMON_VALIDATION: u32 = VALIDATION_REQUIRES_NON_NULL_CSTR
    | VALIDATION_ALLOWS_EMPTY_CSTR
    | VALIDATION_NO_ENDPOINT_LOOKUP
    | VALIDATION_NO_JSON_PARSE;

const METAS: [MutationMeta; 3] = [
    MutationMeta {
        kind: MutationCommandKind::UpsertNode,
        result_kind: MutationResultKind::NodeId,
        required_fields: FIELD_QUALIFIED_NAME,
        optional_fields: FIELD_LABEL
            | FIELD_NAME
            | FIELD_FILE_PATH
            | FIELD_START_LINE
            | FIELD_END_LINE
            | FIELD_PROPERTIES_JSON,
        effect_flags: EFFECT_MUTATES_GRAPH | EFFECT_ALLOCATES_ID | EFFECT_UPDATES_EXISTING,
        validation_flags: COMMON_VALIDATION,
    },
    MutationMeta {
        kind: MutationCommandKind::InsertEdge,
        result_kind: MutationResultKind::EdgeId,
        required_fields: FIELD_SOURCE_ID | FIELD_TARGET_ID | FIELD_EDGE_TYPE,
        optional_fields: FIELD_PROPERTIES_JSON,
        effect_flags: EFFECT_MUTATES_GRAPH
            | EFFECT_ALLOCATES_ID
            | EFFECT_DEDUPS
            | EFFECT_UPDATES_EXISTING,
        validation_flags: COMMON_VALIDATION,
    },
    MutationMeta {
        kind: MutationCommandKind::DeleteByFile,
        result_kind: MutationResultKind::Count,
        required_fields: FIELD_FILE_PATH,
        optional_fields: 0,
        effect_flags: EFFECT_MUTATES_GRAPH | EFFECT_CASCADE_EDGES,
        validation_flags: COMMON_VALIDATION,
    },
];

impl MutationCommandKind {
    pub fn from_i32(value: i32) -> Option<Self> {
        match value {
            1 => Some(Self::UpsertNode),
            2 => Some(Self::InsertEdge),
            3 => Some(Self::DeleteByFile),
            _ => None,
        }
    }
}

pub fn command_metas() -> &'static [MutationMeta] {
    &METAS
}

pub fn validate(shape: MutationCommandShape) -> MutationValidation {
    let Some(kind) = MutationCommandKind::from_i32(shape.kind) else {
        return MutationValidation {
            ok: false,
            error_code: ERROR_UNKNOWN_KIND,
            missing_fields: 0,
            invalid_fields: FIELD_KIND,
            normalized_flags: 0,
        };
    };

    let invalid_fields = if shape.reserved0 == 0 {
        0
    } else {
        FIELD_RESERVED
    };
    let missing_fields = match kind {
        MutationCommandKind::UpsertNode => missing(!shape.has_qualified_name, FIELD_QUALIFIED_NAME),
        MutationCommandKind::InsertEdge => missing(!shape.has_edge_type, FIELD_EDGE_TYPE),
        MutationCommandKind::DeleteByFile => missing(!shape.has_file_path, FIELD_FILE_PATH),
    };
    let normalized_flags = optional_null_flags(kind, shape);

    let ok = missing_fields == 0 && invalid_fields == 0;
    let error_code = if invalid_fields != 0 {
        ERROR_INVALID_FIELD
    } else if missing_fields != 0 {
        ERROR_MISSING_FIELD
    } else {
        ERROR_OK
    };

    MutationValidation {
        ok,
        error_code,
        missing_fields,
        invalid_fields,
        normalized_flags,
    }
}

fn missing(condition: bool, field: u32) -> u32 {
    if condition {
        field
    } else {
        0
    }
}

fn optional_null_flags(kind: MutationCommandKind, shape: MutationCommandShape) -> u32 {
    let has_optional_null = match kind {
        MutationCommandKind::UpsertNode => {
            !shape.has_label
                || !shape.has_name
                || !shape.has_file_path
                || !shape.has_properties_json
        }
        MutationCommandKind::InsertEdge => !shape.has_properties_json,
        MutationCommandKind::DeleteByFile => false,
    };
    if has_optional_null {
        NORMALIZED_OPTIONAL_NULL_CSTR
    } else {
        0
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn command_table_matches_initial_graph_buffer_boundary() {
        let metas = command_metas();
        assert_eq!(metas.len(), 3);
        assert_eq!(metas[0].kind, MutationCommandKind::UpsertNode);
        assert_eq!(metas[0].result_kind, MutationResultKind::NodeId);
        assert_eq!(metas[0].required_fields, FIELD_QUALIFIED_NAME);
        assert_eq!(
            metas[0].effect_flags,
            EFFECT_MUTATES_GRAPH | EFFECT_ALLOCATES_ID | EFFECT_UPDATES_EXISTING
        );

        assert_eq!(metas[1].kind, MutationCommandKind::InsertEdge);
        assert_eq!(metas[1].result_kind, MutationResultKind::EdgeId);
        assert_eq!(
            metas[1].required_fields,
            FIELD_SOURCE_ID | FIELD_TARGET_ID | FIELD_EDGE_TYPE
        );
        assert_eq!(
            metas[1].effect_flags,
            EFFECT_MUTATES_GRAPH | EFFECT_ALLOCATES_ID | EFFECT_DEDUPS | EFFECT_UPDATES_EXISTING
        );

        assert_eq!(metas[2].kind, MutationCommandKind::DeleteByFile);
        assert_eq!(metas[2].result_kind, MutationResultKind::Count);
        assert_eq!(metas[2].required_fields, FIELD_FILE_PATH);
        assert_eq!(
            metas[2].effect_flags,
            EFFECT_MUTATES_GRAPH | EFFECT_CASCADE_EDGES
        );
    }

    #[test]
    fn validation_requires_only_c_contract_required_strings() {
        let upsert_empty_qn = MutationCommandShape {
            kind: MutationCommandKind::UpsertNode as i32,
            reserved0: 0,
            has_label: false,
            has_name: false,
            has_qualified_name: true,
            has_file_path: false,
            has_edge_type: false,
            has_properties_json: false,
        };
        let validation = validate(upsert_empty_qn);
        assert!(validation.ok);
        assert_eq!(validation.error_code, ERROR_OK);
        assert_eq!(validation.missing_fields, 0);
        assert_eq!(validation.normalized_flags, NORMALIZED_OPTIONAL_NULL_CSTR);

        let missing_qn = MutationCommandShape {
            has_qualified_name: false,
            ..upsert_empty_qn
        };
        let validation = validate(missing_qn);
        assert!(!validation.ok);
        assert_eq!(validation.error_code, ERROR_MISSING_FIELD);
        assert_eq!(validation.missing_fields, FIELD_QUALIFIED_NAME);

        let missing_edge_type = MutationCommandShape {
            kind: MutationCommandKind::InsertEdge as i32,
            reserved0: 0,
            has_label: false,
            has_name: false,
            has_qualified_name: false,
            has_file_path: false,
            has_edge_type: false,
            has_properties_json: true,
        };
        let validation = validate(missing_edge_type);
        assert!(!validation.ok);
        assert_eq!(validation.missing_fields, FIELD_EDGE_TYPE);

        let delete_file = MutationCommandShape {
            kind: MutationCommandKind::DeleteByFile as i32,
            has_file_path: true,
            ..missing_edge_type
        };
        assert!(validate(delete_file).ok);
    }

    #[test]
    fn validation_rejects_unknown_kind_and_reserved_bits() {
        let unknown = MutationCommandShape {
            kind: 99,
            reserved0: 0,
            has_label: false,
            has_name: false,
            has_qualified_name: false,
            has_file_path: false,
            has_edge_type: false,
            has_properties_json: false,
        };
        let validation = validate(unknown);
        assert!(!validation.ok);
        assert_eq!(validation.error_code, ERROR_UNKNOWN_KIND);
        assert_eq!(validation.invalid_fields, FIELD_KIND);

        let reserved = MutationCommandShape {
            kind: MutationCommandKind::DeleteByFile as i32,
            reserved0: 1,
            has_file_path: true,
            ..unknown
        };
        let validation = validate(reserved);
        assert!(!validation.ok);
        assert_eq!(validation.error_code, ERROR_INVALID_FIELD);
        assert_eq!(validation.invalid_fields, FIELD_RESERVED);
    }
}
