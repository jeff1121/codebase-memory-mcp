//! 對齊 `src/semantic/semantic.c` 的 token boundary 純 predicate。
//!
//! token buffer、abbreviation table、字串 allocation 與輸出 ownership 仍由 C 負責。

#[must_use]
pub fn is_token_delim(byte: u8) -> bool {
    matches!(
        byte,
        b'.' | b'/' | b'_' | b'-' | b' ' | b'(' | b')' | b',' | b':'
    )
}

#[must_use]
pub fn is_camel_break(name: Option<&[u8]>, index: usize) -> bool {
    let Some(name) = name else {
        return false;
    };
    if index == 0 || index >= name.len() {
        return false;
    }
    name[index].is_ascii_uppercase() && name[index - 1].is_ascii_lowercase()
}

#[cfg(test)]
mod tests {
    use super::{is_camel_break, is_token_delim};

    #[test]
    fn recognizes_token_delimiters() {
        for byte in b"./_- (),:" {
            assert!(is_token_delim(*byte));
        }
        assert!(!is_token_delim(b'A'));
        assert!(!is_token_delim(0x80));
        assert!(!is_token_delim(0xff));
    }

    #[test]
    fn recognizes_lower_to_upper_transitions() {
        assert!(is_camel_break(Some(b"fooBar"), 3));
        assert!(is_camel_break(Some(b"FooBar"), 3));
        assert!(!is_camel_break(Some(b"foobar"), 3));
        assert!(!is_camel_break(Some(b"fooBar"), 0));
        assert!(!is_camel_break(Some(b"fooBar"), 6));
        assert!(!is_camel_break(None, 1));
    }
}
