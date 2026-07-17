//! `pass_calls.c` 的 import JSON local-name extractor。

#[must_use]
pub fn extract_local_name(json: Option<&[u8]>) -> Option<Vec<u8>> {
    let json = json?;
    let pattern = b"\"local_name\":\"";
    let start = json
        .windows(pattern.len())
        .position(|window| window == pattern)?
        + pattern.len();
    let end = json[start..].iter().position(|byte| *byte == b'"')? + start;
    if end <= start {
        return None;
    }
    Some(json[start..end].to_vec())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn extracts_nonempty_local_name() {
        assert_eq!(
            extract_local_name(Some(br#"{"local_name":"client","module":"pkg"}"#)),
            Some(b"client".to_vec())
        );
        assert_eq!(extract_local_name(Some(br#"{"local_name":""}"#)), None);
        assert_eq!(extract_local_name(Some(b"{}")), None);
    }
}
