//! 對齊 `src/pipeline/pass_usages.c` 的 local-name JSON 純擷取 helper。
//!
//! import map、graph lookup、字串 allocation 與 edge ownership 仍由 C 負責。

fn find_subslice(haystack: &[u8], needle: &[u8]) -> Option<usize> {
    haystack
        .windows(needle.len())
        .position(|window| window == needle)
}

#[must_use]
pub fn extract_local_name(json: Option<&[u8]>) -> Option<Vec<u8>> {
    let json = json?;
    let prefix = b"\"local_name\":\"";
    let start = find_subslice(json, prefix)? + prefix.len();
    let end = json[start..].iter().position(|byte| *byte == b'"')? + start;
    if end <= start {
        return None;
    }
    Some(json[start..end].to_vec())
}

#[cfg(test)]
mod tests {
    use super::extract_local_name;

    #[test]
    fn extracts_local_name_like_c_parser() {
        assert_eq!(
            extract_local_name(Some(b"{\"local_name\":\"thing\"}")),
            Some(b"thing".to_vec())
        );
        assert_eq!(
            extract_local_name(Some(b"{\"x\":1,\"local_name\":\"Thing\"}")),
            Some(b"Thing".to_vec())
        );
        assert_eq!(extract_local_name(Some(b"{\"local_name\":\"\"}")), None);
        assert_eq!(extract_local_name(Some(b"{}")), None);
        assert_eq!(extract_local_name(None), None);
        assert_eq!(
            extract_local_name(Some(b"{\"local_name\":\"a\\\"b\"}")),
            Some(b"a\\".to_vec())
        );
    }
}
