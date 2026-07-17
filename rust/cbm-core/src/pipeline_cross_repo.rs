//! `pass_cross_repo.c` 的 bounded JSON string property copier。

#[must_use]
pub fn json_str_prop(
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
    pattern.truncate(127);
    let start = json
        .windows(pattern.len())
        .position(|window| window == pattern)?
        + pattern.len();
    let end = json[start..].iter().position(|byte| *byte == b'"')? + start;
    let copy_len = (end - start).min(output_size - 1);
    Some(json[start..start + copy_len].to_vec())
}

#[must_use]
pub fn build_cross_props(
    target_project: Option<&[u8]>,
    target_function: Option<&[u8]>,
    target_file: Option<&[u8]>,
    url_or_channel: Option<&[u8]>,
    extra_key: Option<&[u8]>,
    extra_value: Option<&[u8]>,
    output_size: usize,
) -> Vec<u8> {
    if output_size == 0 {
        return Vec::new();
    }
    let mut output = Vec::new();
    output.extend_from_slice(b"{\"target_project\":\"");
    output.extend_from_slice(target_project.unwrap_or_default());
    output.extend_from_slice(b"\",\"target_function\":\"");
    output.extend_from_slice(target_function.unwrap_or_default());
    output.extend_from_slice(b"\",\"target_file\":\"");
    output.extend_from_slice(target_file.unwrap_or_default());
    output.extend_from_slice(b"\"");
    if url_or_channel.is_some_and(|value| !value.is_empty()) {
        output.extend_from_slice(b",\"");
        output.extend_from_slice(extra_key.unwrap_or(b"url_path"));
        output.extend_from_slice(b"\":\"");
        output.extend_from_slice(url_or_channel.unwrap_or_default());
        output.extend_from_slice(b"\"");
    }
    if extra_value.is_some_and(|value| !value.is_empty()) {
        output.extend_from_slice(b",\"");
        output.extend_from_slice(if extra_key.is_some() {
            b"transport"
        } else {
            b"method"
        });
        output.extend_from_slice(b"\":\"");
        output.extend_from_slice(extra_value.unwrap_or_default());
        output.extend_from_slice(b"\"");
    }
    output.push(b'}');
    output.truncate(output_size - 1);
    output
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn copies_json_string_and_truncates_to_output_buffer() {
        assert_eq!(
            json_str_prop(
                Some(br#"{"url_path":"/api/items","method":"GET"}"#),
                Some(b"url_path"),
                32
            ),
            Some(b"/api/items".to_vec())
        );
        assert_eq!(
            json_str_prop(Some(br#"{"url_path":"/api/items"}"#), Some(b"url_path"), 5),
            Some(b"/api".to_vec())
        );
    }

    #[test]
    fn returns_none_for_missing_or_null_inputs() {
        assert_eq!(json_str_prop(Some(b"{}"), Some(b"url_path"), 16), None);
        assert_eq!(json_str_prop(None, Some(b"url_path"), 16), None);
        assert_eq!(json_str_prop(Some(b"{}"), None, 16), None);
    }

    #[test]
    fn builds_cross_repo_properties_with_optional_fields() {
        assert_eq!(
            build_cross_props(
                Some(b"target"), Some(b"handler"), Some(b"src.rs"), Some(b"/api"),
                Some(b"url_path"), Some(b"GET"), 128
            ),
            br#"{"target_project":"target","target_function":"handler","target_file":"src.rs","url_path":"/api","transport":"GET"}"#.to_vec()
        );
    }
}
