//! `src/store/store.c` search pattern helpers 的 Rust parity/opt-in helper。
//!
//! 這裡只處理 byte-level 字串轉換；SQLite query building、binding 與 execution
//! 仍由 C store runtime 負責。

const MIN_HINT_LEN: usize = 3;
const HINT_BUF_MAX: usize = 255;

#[must_use]
pub fn glob_to_like(pattern: Option<&[u8]>) -> Option<Vec<u8>> {
    let pattern = pattern?;
    let mut out = Vec::with_capacity(pattern.len().saturating_mul(2));
    let mut i = 0;
    while i < pattern.len() {
        if pattern[i] == b'*' && pattern.get(i + 1) == Some(&b'*') {
            if out.last() == Some(&b'/') {
                out.pop();
            }
            out.push(b'%');
            i += 1;
            if pattern.get(i + 1) == Some(&b'/') {
                i += 1;
            }
        } else if pattern[i] == b'*' {
            out.push(b'%');
        } else if pattern[i] == b'?' {
            out.push(b'_');
        } else {
            out.push(pattern[i]);
        }
        i += 1;
    }
    Some(out)
}

#[must_use]
pub fn ensure_case_insensitive(pattern: Option<&[u8]>) -> Vec<u8> {
    let Some(pattern) = pattern else {
        return Vec::new();
    };
    if pattern.starts_with(b"(?i)") {
        pattern.to_vec()
    } else {
        let mut out = Vec::with_capacity(pattern.len() + 4);
        out.extend_from_slice(b"(?i)");
        out.extend_from_slice(pattern);
        out
    }
}

#[must_use]
pub fn strip_case_flag(pattern: Option<&[u8]>) -> Vec<u8> {
    let Some(pattern) = pattern else {
        return Vec::new();
    };
    if pattern.starts_with(b"(?i)") {
        pattern[4..].to_vec()
    } else {
        pattern.to_vec()
    }
}

#[must_use]
pub fn like_hints(pattern: Option<&[u8]>, max_out: i32) -> Vec<Vec<u8>> {
    let Some(pattern) = pattern else {
        return Vec::new();
    };
    if max_out <= 0 || pattern.contains(&b'|') {
        return Vec::new();
    }

    let mut hints = Vec::new();
    let mut buf = Vec::with_capacity(HINT_BUF_MAX);
    let mut i = 0;
    while i < pattern.len() {
        let ch = pattern[i];
        match ch {
            b'\\' => {
                if let Some(next) = pattern.get(i + 1).copied() {
                    push_hint_byte(&mut buf, next);
                    i += 2;
                } else {
                    i += 1;
                }
            }
            b'.' | b'*' | b'+' | b'?' | b'^' | b'$' | b'(' | b')' | b'[' | b']' | b'{' | b'}' => {
                flush_hint(&mut hints, &mut buf, max_out);
                i += 1;
            }
            _ => {
                push_hint_byte(&mut buf, ch);
                i += 1;
            }
        }
    }
    flush_hint(&mut hints, &mut buf, max_out);
    hints
}

fn push_hint_byte(buf: &mut Vec<u8>, byte: u8) {
    if buf.len() < HINT_BUF_MAX {
        buf.push(byte);
    }
}

fn flush_hint(hints: &mut Vec<Vec<u8>>, buf: &mut Vec<u8>, max_out: i32) {
    if buf.len() >= MIN_HINT_LEN && hints.len() < max_out as usize {
        hints.push(buf.clone());
    }
    buf.clear();
}

#[cfg(test)]
mod tests {
    use super::*;

    fn bytes(input: Option<&[u8]>) -> String {
        String::from_utf8(input.unwrap().to_vec()).unwrap()
    }

    #[test]
    fn glob_to_like_matches_c_contract() {
        assert_eq!(bytes(glob_to_like(Some(b"**/*.py")).as_deref()), "%%.py");
        assert_eq!(bytes(glob_to_like(Some(b"**/dir/**")).as_deref()), "%dir%");
        assert_eq!(bytes(glob_to_like(Some(b"*.go")).as_deref()), "%.go");
        assert_eq!(
            bytes(glob_to_like(Some(b"src/[abc]/*.ts")).as_deref()),
            "src/[abc]/%.ts"
        );
        assert_eq!(
            bytes(glob_to_like(Some(b"f???.txt")).as_deref()),
            "f___.txt"
        );
        assert_eq!(bytes(glob_to_like(Some(b"")).as_deref()), "");
        assert!(glob_to_like(None).is_none());
    }

    #[test]
    fn case_helpers_match_c_contract() {
        assert_eq!(ensure_case_insensitive(Some(b"handler")), b"(?i)handler");
        assert_eq!(
            ensure_case_insensitive(Some(b"(?i)handler")),
            b"(?i)handler"
        );
        assert_eq!(ensure_case_insensitive(Some(b"")), b"(?i)");
        assert_eq!(ensure_case_insensitive(None), b"");

        assert_eq!(strip_case_flag(Some(b"(?i)handler")), b"handler");
        assert_eq!(strip_case_flag(Some(b"handler")), b"handler");
        assert_eq!(strip_case_flag(Some(b"(?i)(?i)double")), b"(?i)double");
        assert_eq!(strip_case_flag(None), b"");
    }

    #[test]
    fn like_hints_match_regex_prefilter_contract() {
        assert_eq!(
            like_hints(Some(b".*handler.*"), 16),
            vec![b"handler".to_vec()]
        );
        assert_eq!(
            like_hints(Some(b".*Order.*Handler.*"), 16),
            vec![b"Order".to_vec(), b"Handler".to_vec()]
        );
        assert_eq!(like_hints(Some(b"handler"), 16), vec![b"handler".to_vec()]);
        assert_eq!(
            like_hints(Some(b"^handleRequest$"), 16),
            vec![b"handleRequest".to_vec()]
        );
        assert!(like_hints(Some(b".*"), 16).is_empty());
        assert!(like_hints(Some(b".*ab.*cd.*"), 16).is_empty());
        assert_eq!(like_hints(Some(b".*abc.*"), 16), vec![b"abc".to_vec()]);
        assert!(like_hints(Some(b".*foo|.*bar"), 16).is_empty());
        assert!(like_hints(Some(b"foo\\|bar"), 16).is_empty());
        assert_eq!(
            like_hints(Some(b".*test_.*helper.*"), 16),
            vec![b"test_".to_vec(), b"helper".to_vec()]
        );
        assert_eq!(
            like_hints(Some(b"\\.handler"), 16),
            vec![b".handler".to_vec()]
        );
        assert_eq!(
            like_hints(Some(b"foo\\.bar"), 16),
            vec![b"foo.bar".to_vec()]
        );
        assert_eq!(
            like_hints(Some(b".*aaa.*bbb.*ccc.*ddd.*"), 2),
            vec![b"aaa".to_vec(), b"bbb".to_vec()]
        );
        assert!(like_hints(Some(b".*handler.*"), 0).is_empty());
        assert!(like_hints(Some(b".*handler.*"), -1).is_empty());
        assert!(like_hints(None, 16).is_empty());
    }

    #[test]
    fn like_hints_truncate_long_literal_segments() {
        let pattern = vec![b'a'; 300];
        let hints = like_hints(Some(&pattern), 16);
        assert_eq!(hints.len(), 1);
        assert_eq!(hints[0].len(), HINT_BUF_MAX);
        assert!(hints[0].iter().all(|&byte| byte == b'a'));
    }
}
