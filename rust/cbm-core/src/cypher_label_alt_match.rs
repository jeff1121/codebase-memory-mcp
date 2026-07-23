//! `src/cypher/cypher.c` label alternative 比對的無配置 parity helper。
//!
//! FFI 邊界負責處理 null 與 NUL-terminated C string；本模組只掃描 first-NUL
//! 以前的 bytes，不配置、不修改輸入，也不進行 I/O 或 locale 相關操作。

#[must_use]
pub fn matches(actual: &[u8], pat: &[u8]) -> bool {
    if !pat.contains(&b'|') {
        return actual == pat;
    }

    let mut segment_start = 0;
    while segment_start < pat.len() {
        let remaining = &pat[segment_start..];
        let Some(separator_offset) = remaining.iter().position(|&byte| byte == b'|') else {
            return remaining == actual;
        };
        let segment_end = segment_start + separator_offset;
        if actual == &pat[segment_start..segment_end] {
            return true;
        }
        segment_start = segment_end + 1;
    }

    false
}

#[cfg(test)]
mod tests {
    use std::os::raw::c_char;
    use std::ptr;

    use super::matches;

    #[test]
    fn label_alt_matches_c_contract() {
        let alpha = b"Alpha\0";
        let alpha_lower = b"alpha\0";
        let empty = b"\0";
        let dangling_actual = ptr::NonNull::<c_char>::dangling().as_ptr();

        assert!(unsafe {
            super::super::cbm_rs_cypher_label_alt_matches_v1(dangling_actual, ptr::null())
        });
        assert!(unsafe {
            super::super::cbm_rs_cypher_label_alt_matches_v1(ptr::null(), ptr::null())
        });
        assert!(!unsafe {
            super::super::cbm_rs_cypher_label_alt_matches_v1(ptr::null(), alpha.as_ptr().cast())
        });

        assert!(matches(b"", b""));
        assert!(!matches(b"Alpha", b""));
        assert!(unsafe {
            super::super::cbm_rs_cypher_label_alt_matches_v1(
                alpha.as_ptr().cast(),
                alpha.as_ptr().cast(),
            )
        });
        assert!(!unsafe {
            super::super::cbm_rs_cypher_label_alt_matches_v1(
                alpha_lower.as_ptr().cast(),
                alpha.as_ptr().cast(),
            )
        });
        let bridge = b"Bridge\0";
        let bridge_or_other = b"Bridge|Other\0";
        assert!(unsafe {
            super::super::cbm_rs_cypher_label_alt_matches_v1(
                bridge.as_ptr().cast(),
                bridge_or_other.as_ptr().cast(),
            )
        });
        assert!(unsafe {
            super::super::cbm_rs_cypher_label_alt_matches_v1(
                empty.as_ptr().cast(),
                empty.as_ptr().cast(),
            )
        });

        assert!(matches(b"A", b"A|B"));
        assert!(matches(b"B", b"A|B"));
        assert!(!matches(b"A|B", b"A|B"));
        assert!(matches(b"", b"|A"));
        assert!(matches(b"", b"A||B"));
        assert!(matches(b"", b"|"));
        assert!(!matches(b"", b"A|"));
        assert!(matches(b"A", b"A|"));
        assert!(!matches(b"A", b"||"));

        let before_first_nul_pat = b"A\0|B\0";
        let before_first_nul_actual = b"A\0ignored\0";
        assert!(unsafe {
            super::super::cbm_rs_cypher_label_alt_matches_v1(
                before_first_nul_actual.as_ptr().cast(),
                before_first_nul_pat.as_ptr().cast(),
            )
        });

        let high_byte_pat = [0xff, b'|', b'A', 0];
        let high_byte_actual = [0xff, 0];
        assert!(unsafe {
            super::super::cbm_rs_cypher_label_alt_matches_v1(
                high_byte_actual.as_ptr().cast(),
                high_byte_pat.as_ptr().cast(),
            )
        });
    }
}
