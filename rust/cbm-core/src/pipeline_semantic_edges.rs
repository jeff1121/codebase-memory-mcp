//! `pass_semantic_edges.c` 的 flat JSON metadata readers。

#[must_use]
pub fn json_str_value(
    json: Option<&[u8]>,
    key: Option<&[u8]>,
    output_size: usize,
) -> Option<Vec<u8>> {
    let (Some(json), Some(key)) = (json, key) else {
        return None;
    };
    if output_size == 0 {
        return None;
    }
    let mut pattern = Vec::with_capacity(key.len() + 4);
    pattern.extend_from_slice(b"\"");
    pattern.extend_from_slice(key);
    pattern.extend_from_slice(b"\":\"");
    pattern.truncate(63);
    let start = json
        .windows(pattern.len())
        .position(|window| window == pattern)?
        + pattern.len();
    let end = json[start..].iter().position(|byte| *byte == b'"')? + start;
    let copy_len = (end - start).min(output_size - 1);
    Some(json[start..start + copy_len].to_vec())
}

#[must_use]
pub fn json_str_array(json: Option<&[u8]>, key: Option<&[u8]>, max_output: usize) -> Vec<Vec<u8>> {
    let (Some(json), Some(key)) = (json, key) else {
        return Vec::new();
    };
    if max_output == 0 {
        return Vec::new();
    }
    let mut pattern = Vec::with_capacity(key.len() + 4);
    pattern.extend_from_slice(b"\"");
    pattern.extend_from_slice(key);
    pattern.extend_from_slice(b"\":[");
    pattern.truncate(63);
    let Some(mut offset) = json
        .windows(pattern.len())
        .position(|window| window == pattern)
    else {
        return Vec::new();
    };
    offset += pattern.len();

    let mut output = Vec::new();
    while offset < json.len() && json[offset] != b']' && output.len() < max_output {
        if json[offset] != b'"' {
            offset += 1;
            continue;
        }
        offset += 1;
        let start = offset;
        let Some(relative_end) = json[start..].iter().position(|byte| *byte == b'"') else {
            break;
        };
        let end = start + relative_end;
        output.push(json[start..end].to_vec());
        offset = end + 1;
    }
    output
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn reads_scalar_and_array_values_without_escape_decoding() {
        let json = br#"{"bt":"alpha","tokens":["one","two"]}"#;
        assert_eq!(
            json_str_value(Some(json), Some(b"bt"), 16),
            Some(b"alpha".to_vec())
        );
        assert_eq!(
            json_str_array(Some(json), Some(b"tokens"), 8),
            vec![b"one".to_vec(), b"two".to_vec()]
        );
    }

    #[test]
    fn respects_missing_keys_and_output_limits() {
        assert!(json_str_array(Some(b"{}"), Some(b"tokens"), 8).is_empty());
        assert_eq!(
            json_str_value(Some(br#"{"bt":"abcdef"}"#), Some(b"bt"), 4),
            Some(b"abc".to_vec())
        );
    }
}
