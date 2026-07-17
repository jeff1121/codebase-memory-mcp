//! `src/pipeline/pass_definitions.c` 的 JSON escape 純 helper parity。
//!
//! definitions 的檔案讀取、Tree-sitter 結果、JSON field atomic append、graph
//! mutation 與 pipeline side effects 仍由 C 負責。

#[must_use]
pub fn json_escape_char(byte: u8) -> (usize, [u8; 2]) {
    let escaped = match byte {
        b'"' => Some(b'"'),
        b'\\' => Some(b'\\'),
        b'\n' => Some(b'n'),
        b'\r' => Some(b'r'),
        b'\t' => Some(b't'),
        _ => None,
    };
    if let Some(escaped) = escaped {
        (2, [b'\\', escaped])
    } else {
        (1, [if byte < 0x20 { b' ' } else { byte }, 0])
    }
}

#[must_use]
pub fn json_escaped_len(value: Option<&[u8]>) -> usize {
    value
        .unwrap_or_default()
        .iter()
        .map(|byte| json_escape_char(*byte).0)
        .sum()
}

#[cfg(test)]
mod tests {
    use super::{json_escape_char, json_escaped_len};

    #[test]
    fn escape_char_matches_c_contract() {
        assert_eq!(json_escape_char(b'"'), (2, [b'\\', b'"']));
        assert_eq!(json_escape_char(b'\\'), (2, [b'\\', b'\\']));
        assert_eq!(json_escape_char(b'\n'), (2, [b'\\', b'n']));
        assert_eq!(json_escape_char(0x0c), (1, [b' ', 0]));
        assert_eq!(json_escape_char(b'A'), (1, [b'A', 0]));
    }

    #[test]
    fn escaped_length_matches_c_contract() {
        assert_eq!(json_escaped_len(Some(b"plain")), 5);
        assert_eq!(json_escaped_len(Some(b"a\"b\\c\nd")), 10);
        assert_eq!(json_escaped_len(None), 0);
    }
}
