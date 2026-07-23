//! `src/cypher/cypher.c` regex 形狀判斷的無配置 parity helper。
//!
//! FFI 邊界負責處理 null 與 NUL-terminated C string；本模組僅以 byte 形式判斷
//! 固定的 regex 線索，不配置、不修改輸入，也不進行 I/O 或 locale 相關操作。

#[must_use]
pub fn looks_like_regex(bytes: &[u8]) -> bool {
    bytes.windows(2).any(|pair| pair == b".*" || pair == b".+")
        || bytes
            .iter()
            .any(|&byte| matches!(byte, b'[' | b'(' | b'|' | b'^' | b'$'))
}

#[cfg(test)]
mod tests {
    use super::looks_like_regex;

    #[test]
    fn looks_like_regex_matches_c_contract() {
        assert!(!looks_like_regex(b"literal"));
        assert!(!looks_like_regex(b"dot."));
        assert!(!looks_like_regex(b"star*"));
        assert!(looks_like_regex(b"prefix.*suffix"));
        assert!(looks_like_regex(b"prefix.+suffix"));
        assert!(looks_like_regex(b"[class]"));
        assert!(looks_like_regex(b"(group)"));
        assert!(looks_like_regex(b"left|right"));
        assert!(looks_like_regex(b"^start"));
        assert!(looks_like_regex(b"end$"));

        let before_first_nul = b"literal\0[ignored\0";
        assert!(!unsafe {
            super::super::cbm_rs_cypher_looks_like_regex_v1(before_first_nul.as_ptr().cast())
        });
        assert!(!unsafe { super::super::cbm_rs_cypher_looks_like_regex_v1(std::ptr::null()) });

        let high_byte_literal = [0xff, b'a', 0];
        assert!(!unsafe {
            super::super::cbm_rs_cypher_looks_like_regex_v1(high_byte_literal.as_ptr().cast())
        });
        let high_byte_regex = [0xff, b'.', b'*', 0];
        assert!(unsafe {
            super::super::cbm_rs_cypher_looks_like_regex_v1(high_byte_regex.as_ptr().cast())
        });
    }
}
