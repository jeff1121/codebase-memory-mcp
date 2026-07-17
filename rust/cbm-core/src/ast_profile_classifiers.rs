//! 對齊 `src/semantic/ast_profile.c` 的 AST node-kind 純分類。
//!
//! Tree-sitter traversal、profile output 與所有 buffer ownership 仍由 C 負責。

pub const IF: u32 = 1 << 0;
pub const FOR: u32 = 1 << 1;
pub const WHILE: u32 = 1 << 2;
pub const SWITCH: u32 = 1 << 3;
pub const TRY: u32 = 1 << 4;
pub const RETURN: u32 = 1 << 5;
pub const COMPARISON: u32 = 1 << 6;
pub const ARITHMETIC: u32 = 1 << 7;
pub const ASSIGNMENT: u32 = 1 << 8;
pub const STRING_LITERAL: u32 = 1 << 9;
pub const NUMBER_LITERAL: u32 = 1 << 10;
pub const BOOL_LITERAL: u32 = 1 << 11;
pub const OPERATOR: u32 = 1 << 12;
pub const IDENTIFIER: u32 = 1 << 13;

fn is_any(kind: &[u8], names: &[&[u8]]) -> bool {
    names.contains(&kind)
}

#[must_use]
pub fn kind_flags(kind: Option<&[u8]>) -> u32 {
    let Some(kind) = kind else {
        return 0;
    };
    let mut flags = 0;
    if is_any(kind, &[b"if_statement", b"if_expression", b"elif_clause"]) {
        flags |= IF;
    }
    if is_any(
        kind,
        &[
            b"for_statement",
            b"for_range_loop",
            b"for_expression",
            b"for_in_clause",
        ],
    ) {
        flags |= FOR;
    }
    if is_any(
        kind,
        &[b"while_statement", b"while_expression", b"do_statement"],
    ) {
        flags |= WHILE;
    }
    if is_any(
        kind,
        &[
            b"switch_statement",
            b"switch_expression",
            b"match_expression",
            b"type_switch_statement",
        ],
    ) {
        flags |= SWITCH;
    }
    if is_any(
        kind,
        &[
            b"try_statement",
            b"try_expression",
            b"catch_clause",
            b"except_clause",
        ],
    ) {
        flags |= TRY;
    }
    if is_any(kind, &[b"return_statement", b"return_expression"]) {
        flags |= RETURN;
    }
    if is_any(
        kind,
        &[
            b"binary_expression",
            b"comparison_operator",
            b"boolean_operator",
        ],
    ) {
        flags |= COMPARISON;
    }
    if is_any(kind, &[b"unary_expression", b"update_expression"]) {
        flags |= ARITHMETIC;
    }
    if is_any(
        kind,
        &[
            b"assignment_expression",
            b"assignment_statement",
            b"augmented_assignment",
            b"short_var_declaration",
        ],
    ) {
        flags |= ASSIGNMENT;
    }
    if is_any(
        kind,
        &[
            b"string",
            b"string_literal",
            b"interpreted_string_literal",
            b"raw_string_literal",
        ],
    ) {
        flags |= STRING_LITERAL;
    }
    if is_any(
        kind,
        &[
            b"number",
            b"integer",
            b"float",
            b"integer_literal",
            b"float_literal",
        ],
    ) {
        flags |= NUMBER_LITERAL;
    }
    if is_any(kind, &[b"true", b"false"]) {
        flags |= BOOL_LITERAL;
    }
    if is_any(
        kind,
        &[
            b"identifier",
            b"field_identifier",
            b"property_identifier",
            b"type_identifier",
        ],
    ) {
        flags |= IDENTIFIER;
    }
    if flags & (IF | FOR | WHILE | SWITCH | TRY | RETURN | COMPARISON | ARITHMETIC | ASSIGNMENT)
        != 0
        || is_any(
            kind,
            &[
                b"call_expression",
                b"member_expression",
                b"subscript_expression",
            ],
        )
    {
        flags |= OPERATOR;
    }
    flags
}

#[cfg(test)]
mod tests {
    use super::{kind_flags, ASSIGNMENT, BOOL_LITERAL, IDENTIFIER, IF, OPERATOR, STRING_LITERAL};

    #[test]
    fn classifies_control_and_literal_kinds() {
        assert_ne!(kind_flags(Some(b"if_statement")) & IF, 0);
        assert_ne!(kind_flags(Some(b"assignment_expression")) & ASSIGNMENT, 0);
        assert_ne!(kind_flags(Some(b"string_literal")) & STRING_LITERAL, 0);
        assert_ne!(kind_flags(Some(b"true")) & BOOL_LITERAL, 0);
        assert_ne!(kind_flags(Some(b"identifier")) & IDENTIFIER, 0);
        assert_ne!(kind_flags(Some(b"call_expression")) & OPERATOR, 0);
        assert_eq!(kind_flags(None), 0);
    }
}

#[must_use]
pub fn halstead_insert(set: &mut [u32], key: Option<&[u8]>) -> bool {
    if set.len() < 512 {
        return false;
    }
    let Some(key) = key else {
        return false;
    };
    let mut hash = 0u32;
    for byte in key {
        hash = hash.wrapping_mul(31).wrapping_add(u32::from(*byte));
    }
    let index = (hash & 511) as usize;
    for probe in 0..512usize {
        let slot = (index + probe) & 511;
        if set[slot] == 0 {
            set[slot] = hash | 1;
            return true;
        }
        if set[slot] == (hash | 1) {
            return false;
        }
    }
    false
}

#[must_use]
pub fn is_param_name(ident: Option<&[u8]>, param_names: &[Option<&[u8]>]) -> bool {
    let Some(ident) = ident else {
        return false;
    };
    if param_names.is_empty() {
        return false;
    }
    param_names.iter().flatten().any(|param| *param == ident)
}

#[cfg(test)]
mod halstead_tests {
    use super::halstead_insert;

    #[test]
    fn inserts_unique_hashes_and_rejects_duplicates() {
        let mut set = [0u32; 512];
        assert!(halstead_insert(&mut set, Some(b"if_statement")));
        assert!(!halstead_insert(&mut set, Some(b"if_statement")));
        assert!(!halstead_insert(&mut set[..8], Some(b"other")));
        assert!(!halstead_insert(&mut set, None));
    }
}

#[cfg(test)]
mod param_tests {
    use super::is_param_name;

    #[test]
    fn matches_parameter_names() {
        let names = [Some(b"count".as_slice()), Some(b"value".as_slice())];
        assert!(is_param_name(Some(b"value"), &names));
        assert!(!is_param_name(Some(b"other"), &names));
        assert!(!is_param_name(None, &names));
    }
}
