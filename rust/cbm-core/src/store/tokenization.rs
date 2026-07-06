//! `src/store/store.c` FTS camelCase tokenization 的 test-only parity helper。
//!
//! C UDF 會輸出「原 identifier + 空白 + split 後 identifier」，讓 FTS5 同時
//! 建立 exact token 與 camelCase 拆字 token。

const CAMEL_SPLIT_BUF: usize = 2048;
const CAMEL_BUF_GUARD: usize = 2;

#[must_use]
pub fn camel_split_bytes(input: Option<&[u8]>) -> Vec<u8> {
    let Some(input) = input else {
        return Vec::new();
    };
    if input.is_empty() {
        return Vec::new();
    }
    let Some(initial_len) = input.len().checked_add(1) else {
        return input.to_vec();
    };
    if initial_len >= CAMEL_SPLIT_BUF {
        return input.to_vec();
    }

    let mut out = Vec::with_capacity(CAMEL_SPLIT_BUF.min(input.len().saturating_mul(2)));
    out.extend_from_slice(input);
    out.push(b' ');

    for i in 0..input.len() {
        if out.len() >= CAMEL_SPLIT_BUF - CAMEL_BUF_GUARD {
            break;
        }
        if camel_should_split(input, i) {
            out.push(b' ');
        }
        out.push(input[i]);
    }
    out
}

fn camel_should_split(input: &[u8], i: usize) -> bool {
    if i == 0 {
        return false;
    }
    let curr = input[i];
    let prev = input[i - 1];
    let next = input.get(i + 1).copied().unwrap_or(0);
    if curr.is_ascii_uppercase() && prev.is_ascii_lowercase() {
        return true;
    }
    curr.is_ascii_uppercase() && prev.is_ascii_uppercase() && next.is_ascii_lowercase()
}

#[cfg(test)]
mod tests {
    use super::*;

    fn split(input: Option<&[u8]>) -> Vec<u8> {
        camel_split_bytes(input)
    }

    #[test]
    fn mirrors_c_camel_boundaries() {
        assert_eq!(
            split(Some(b"updateCloudClient")),
            b"updateCloudClient update Cloud Client"
        );
        assert_eq!(split(Some(b"XMLParser")), b"XMLParser XML Parser");
        assert_eq!(split(Some(b"HTTPRequest")), b"HTTPRequest HTTP Request");
        assert_eq!(split(Some(b"snake_case")), b"snake_case snake_case");
    }

    #[test]
    fn handles_null_empty_and_non_ascii_bytes() {
        assert_eq!(split(None), b"");
        assert_eq!(split(Some(b"")), b"");
        assert_eq!(
            split(Some(b"caf\xc3\xa9HTTP")),
            b"caf\xc3\xa9HTTP caf\xc3\xa9HTTP"
        );
    }

    #[test]
    fn falls_back_or_truncates_like_c_buffer_contract() {
        let fallback = vec![b'a'; CAMEL_SPLIT_BUF - 1];
        assert_eq!(split(Some(&fallback)), fallback);

        let near_limit = vec![b'a'; CAMEL_SPLIT_BUF - 2];
        let out = split(Some(&near_limit));
        assert_eq!(out.len(), CAMEL_SPLIT_BUF - 1);
        assert!(out.iter().take(near_limit.len()).all(|byte| *byte == b'a'));
        assert_eq!(out[near_limit.len()], b' ');
    }
}
