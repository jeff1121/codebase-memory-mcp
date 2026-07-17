//! `src/pipeline/pipeline_incremental.c` 的純 edge predicate parity。
//!
//! Incremental purge、SQLite persistence、graph-buffer traversal、registry seed 與
//! dispatch lifecycle 仍由 C 負責。

#[must_use]
pub fn edge_type_is_recomputed(edge_type: Option<&[u8]>) -> bool {
    matches!(
        edge_type,
        Some(b"SIMILAR_TO")
            | Some(b"SEMANTICALLY_RELATED")
            | Some(b"FILE_CHANGES_WITH")
            | Some(b"DATA_FLOWS")
    )
}

#[must_use]
pub fn label_is_registry_symbol(label: Option<&[u8]>) -> bool {
    matches!(
        label,
        Some(b"Function")
            | Some(b"Method")
            | Some(b"Class")
            | Some(b"Struct")
            | Some(b"Interface")
            | Some(b"Enum")
            | Some(b"Type")
            | Some(b"Trait")
            | Some(b"Variable")
            | Some(b"Field")
    )
}

#[cfg(test)]
mod tests {
    use super::{edge_type_is_recomputed, label_is_registry_symbol};

    #[test]
    fn recomputed_edge_types_match_c_contract() {
        for edge_type in [
            b"SIMILAR_TO".as_slice(),
            b"SEMANTICALLY_RELATED".as_slice(),
            b"FILE_CHANGES_WITH".as_slice(),
            b"DATA_FLOWS".as_slice(),
        ] {
            assert!(edge_type_is_recomputed(Some(edge_type)));
        }
        assert!(!edge_type_is_recomputed(Some(b"CALLS")));
        assert!(!edge_type_is_recomputed(None));
    }

    #[test]
    fn registry_labels_match_c_contract() {
        for label in [
            b"Function".as_slice(),
            b"Method".as_slice(),
            b"Class".as_slice(),
            b"Struct".as_slice(),
            b"Interface".as_slice(),
            b"Enum".as_slice(),
            b"Type".as_slice(),
            b"Trait".as_slice(),
            b"Variable".as_slice(),
            b"Field".as_slice(),
        ] {
            assert!(label_is_registry_symbol(Some(label)));
        }
        assert!(!label_is_registry_symbol(Some(b"Module")));
        assert!(!label_is_registry_symbol(None));
    }
}
