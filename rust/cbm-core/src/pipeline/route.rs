fn is_route_ident_char(c: u8) -> bool {
    c.is_ascii_alphanumeric() || c == b'_'
}

pub fn canon_path(input: &[u8]) -> Vec<u8> {
    let mut out = Vec::with_capacity(input.len());
    let mut i = 0;
    while i < input.len() {
        let c = input[i];
        let at_seg_start = out.is_empty() || out.last() == Some(&b'/');
        let mut is_param = false;

        if c == b':'
            && at_seg_start
            && input
                .get(i + 1)
                .is_some_and(|next| is_route_ident_char(*next))
        {
            i += 1;
            while i < input.len() && is_route_ident_char(input[i]) {
                i += 1;
            }
            is_param = true;
        } else if c == b'{' {
            i += 1;
            while i < input.len() && input[i] != b'}' && input[i] != b'/' {
                i += 1;
            }
            if i < input.len() && input[i] == b'}' {
                i += 1;
            }
            is_param = true;
        } else if c == b'<' {
            i += 1;
            while i < input.len() && input[i] != b'>' && input[i] != b'/' {
                i += 1;
            }
            if i < input.len() && input[i] == b'>' {
                i += 1;
            }
            is_param = true;
        } else if c == b'$' && input.get(i + 1) == Some(&b'{') {
            i += 2;
            while i < input.len() && input[i] != b'}' && input[i] != b'/' {
                i += 1;
            }
            if i < input.len() && input[i] == b'}' {
                i += 1;
            }
            is_param = true;
        }

        if is_param {
            out.extend_from_slice(b"{}");
        } else {
            out.push(c);
            i += 1;
        }
    }
    out
}

pub fn is_path_keyword(input: Option<&[u8]>) -> bool {
    matches!(
        input,
        Some(b"prefix")
            | Some(b"path")
            | Some(b"route")
            | Some(b"pattern")
            | Some(b"url")
            | Some(b"endpoint")
            | Some(b"rule")
            | Some(b"mount_path")
            | Some(b"route_path")
            | Some(b"url_path")
    )
}

pub fn is_junk_url(input: &[u8]) -> bool {
    for (index, byte) in input.iter().copied().enumerate() {
        if matches!(
            byte,
            b'\\' | b'^' | b'$' | b'*' | b'+' | b'(' | b')' | b'[' | b']' | b'|' | b' '
        ) {
            return true;
        }
        if byte == b'/' && index > 0 && input[index - 1] == b'/' {
            return true;
        }
    }
    false
}

pub fn normalize_url_arg(input: Option<&[u8]>, max_size: usize) -> Option<Vec<u8>> {
    let input = input?;
    if max_size < 2 {
        return None;
    }
    let mut pos = 0;
    let mut output = Vec::new();
    let mut start = input;
    if matches!(start.first(), Some(b'`' | b'"' | b'\'')) {
        start = &start[1..];
    }
    if start.first() != Some(&b'/') {
        return None;
    }
    while pos < start.len() && output.len() < max_size - 2 {
        if start[pos] == b'$' && start.get(pos + 1) == Some(&b'{') {
            output.push(b':');
            pos += 2;
            while pos < start.len() && start[pos] != b'}' && output.len() < max_size - 2 {
                output.push(start[pos]);
                pos += 1;
            }
            if start.get(pos) == Some(&b'}') {
                pos += 1;
            }
        } else if matches!(start[pos], b'`' | b'"' | b'\'' | b'?') {
            break;
        } else {
            output.push(start[pos]);
            pos += 1;
        }
    }
    if output.len() < 4 || !output[1..].contains(&b'/') || is_junk_url(&output) {
        return None;
    }
    Some(output)
}

pub fn is_hash_segment(input: Option<&[u8]>, max_len: usize, min_word_len: usize) -> bool {
    let Some(input) = input else {
        return false;
    };
    if input.is_empty() || input.len() > max_len {
        return false;
    }
    let mut has_letter = false;
    for byte in input {
        if !byte.is_ascii_digit() && !byte.is_ascii_lowercase() {
            return false;
        }
        has_letter |= byte.is_ascii_lowercase();
    }
    !(has_letter && input.len() >= min_word_len)
}

pub fn is_broker_route(input: Option<&[u8]>) -> bool {
    let Some(input) = input else {
        return false;
    };
    [
        b"__route__infra__".as_slice(),
        b"__route__pubsub__".as_slice(),
        b"__route__cloud_tasks__".as_slice(),
        b"__route__async__".as_slice(),
        b"__route__cloud_scheduler__".as_slice(),
        b"__route__kafka__".as_slice(),
        b"__route__sqs__".as_slice(),
    ]
    .iter()
    .any(|prefix| input.starts_with(prefix))
}

#[cfg(test)]
mod route_node_classifier_tests {
    use super::{is_broker_route, is_hash_segment};

    #[test]
    fn hash_segment_matches_c_contract_boundaries() {
        assert!(!is_hash_segment(None, 12, 3));
        assert!(!is_hash_segment(Some(b""), 12, 3));
        assert!(is_hash_segment(Some(b"123456789012"), 12, 3));
        assert!(!is_hash_segment(Some(b"1234567890123"), 12, 3));
        assert!(is_hash_segment(Some(b"ab"), 12, 3));
        assert!(!is_hash_segment(Some(b"word"), 12, 3));
        assert!(!is_hash_segment(Some(b"Word"), 12, 3));
        assert!(!is_hash_segment(Some(b"ab-c"), 12, 3));
        assert!(!is_hash_segment(Some(b"ab\0c"), 12, 3));
        assert!(!is_hash_segment(Some(b"\xc3\xa9"), 12, 3));
    }

    #[test]
    fn broker_route_matches_c_contract_boundaries() {
        for route in [
            b"__route__infra__topic".as_slice(),
            b"__route__pubsub__topic",
            b"__route__cloud_tasks__queue",
            b"__route__async__job",
            b"__route__cloud_scheduler__job",
            b"__route__kafka__topic",
            b"__route__sqs__queue",
        ] {
            assert!(is_broker_route(Some(route)));
        }

        assert!(!is_broker_route(None));
        assert!(!is_broker_route(Some(b"__route__GET__")));
        assert!(!is_broker_route(Some(b"__route__INFRA__topic")));
        assert!(!is_broker_route(Some(b"x__route__infra__topic")));
    }
}

#[must_use]
pub fn sveltekit_file_kind(file_path: Option<&[u8]>) -> std::os::raw::c_int {
    let Some(file_path) = file_path else {
        return 0;
    };
    if !file_path
        .windows(b"/routes/".len())
        .any(|window| window == b"/routes/")
    {
        return 0;
    }

    let basename = file_path
        .rsplit(|byte| *byte == b'/')
        .next()
        .unwrap_or(file_path);
    match basename {
        b"+server.ts" | b"+server.js" => 1,
        b"+page.server.ts" | b"+page.server.js" => 2,
        b"+layout.server.ts" | b"+layout.server.js" => 3,
        _ => 0,
    }
}

#[must_use]
pub fn sveltekit_server_method(name: Option<&[u8]>) -> Option<&'static [u8]> {
    match name {
        Some(b"GET") => Some(b"GET"),
        Some(b"POST") => Some(b"POST"),
        Some(b"PUT") => Some(b"PUT"),
        Some(b"PATCH") => Some(b"PATCH"),
        Some(b"DELETE") => Some(b"DELETE"),
        Some(b"OPTIONS") => Some(b"OPTIONS"),
        Some(b"HEAD") => Some(b"HEAD"),
        Some(b"fallback") => Some(b"ANY"),
        _ => None,
    }
}

const JSON_PROP_MAX_KEY_LEN: usize = 59;

#[must_use]
pub fn extract_json_prop(
    json: Option<&[u8]>,
    key: Option<&[u8]>,
    output_size: usize,
) -> Option<Vec<u8>> {
    let (Some(json), Some(key)) = (json, key) else {
        return None;
    };
    if output_size == 0 || key.len() > JSON_PROP_MAX_KEY_LEN {
        return None;
    }

    let mut pattern = Vec::with_capacity(key.len() + 5);
    pattern.extend_from_slice(b"\"");
    pattern.extend_from_slice(key);
    pattern.extend_from_slice(b"\":\"");
    let start = json
        .windows(pattern.len())
        .position(|window| window == pattern)?
        + pattern.len();
    let end = json[start..].iter().position(|byte| *byte == b'"')? + start;
    if end <= start || end - start >= output_size {
        return None;
    }
    Some(json[start..end].to_vec())
}

#[derive(Debug, PartialEq, Eq)]
pub(crate) enum UrlPath<'a> {
    Input(&'a [u8]),
    StaticRoot,
}

#[must_use]
pub(crate) fn url_path_borrowed(url: Option<&[u8]>) -> Option<UrlPath<'_>> {
    let url = url?;
    let scheme_end = url
        .windows(b"://".len())
        .position(|window| window == b"://");
    let Some(scheme_end) = scheme_end else {
        return Some(UrlPath::Input(url));
    };
    let host_start = scheme_end + 3;
    url[host_start..]
        .iter()
        .position(|byte| *byte == b'/')
        .map(|offset| UrlPath::Input(&url[host_start + offset..]))
        .or(Some(UrlPath::StaticRoot))
}

#[must_use]
pub fn url_path(url: Option<&[u8]>) -> Option<&[u8]> {
    match url_path_borrowed(url) {
        Some(UrlPath::Input(path)) => Some(path),
        Some(UrlPath::StaticRoot) => Some(b"/"),
        None => None,
    }
}

#[cfg(test)]
mod tests {
    use super::{
        canon_path, extract_json_prop, is_broker_route, is_hash_segment, is_junk_url,
        is_path_keyword, normalize_url_arg, sveltekit_file_kind, sveltekit_server_method, url_path,
        url_path_borrowed, UrlPath, JSON_PROP_MAX_KEY_LEN,
    };

    fn s(input: &str) -> String {
        String::from_utf8(canon_path(input.as_bytes())).unwrap()
    }

    #[test]
    fn canonicalizes_route_parameters() {
        assert_eq!(s("/products/categories"), "/products/categories");
        assert_eq!(s("/players/:id"), "/players/{}");
        assert_eq!(s("/players/{id}"), "/players/{}");
        assert_eq!(s("/users/<int:id>"), "/users/{}");
        assert_eq!(s("/players/${playerId}"), "/players/{}");
        assert_eq!(s("/orders/{id}/items/{itemIndex}"), "/orders/{}/items/{}");
        assert_eq!(s("/a/b:c"), "/a/b:c");
        assert_eq!(s(""), "");
    }

    #[test]
    fn mirrors_unclosed_parameter_contract() {
        assert_eq!(s("/a/{id"), "/a/{}");
        assert_eq!(s("/a/<id"), "/a/{}");
        assert_eq!(s("/a/${id"), "/a/{}");
        assert_eq!(s("/a/{id/next"), "/a/{}/next");
    }

    #[test]
    fn handles_non_utf8_bytes() {
        assert_eq!(canon_path(b"/files/:id/\xff"), b"/files/{}/\xff");
    }

    #[test]
    fn classifies_path_keywords() {
        assert!(is_path_keyword(Some(b"route_path")));
        assert!(is_path_keyword(Some(b"url")));
        assert!(!is_path_keyword(Some(b"handler")));
        assert!(!is_path_keyword(None));
    }

    #[test]
    fn rejects_junk_urls() {
        assert!(!is_junk_url(b"/api/v1"));
        assert!(is_junk_url(b"/api//v1"));
        assert!(is_junk_url(b"/api/[id]"));
    }

    #[test]
    fn normalizes_template_urls() {
        assert_eq!(
            normalize_url_arg(Some(b"`/users/${id}/posts`"), 128),
            Some(b"/users/:id/posts".to_vec())
        );
        assert!(normalize_url_arg(Some(b"users/${id}"), 128).is_none());
        assert!(normalize_url_arg(Some(b"/api"), 128).is_none());
    }

    #[test]
    fn classifies_route_node_segments() {
        assert!(is_hash_segment(Some(b"12345"), 12, 3));
        assert!(is_hash_segment(Some(b"a1"), 12, 3));
        assert!(!is_hash_segment(Some(b"api"), 12, 3));
        assert!(!is_hash_segment(Some(b"ABC"), 12, 3));
        assert!(is_broker_route(Some(b"__route__kafka__topic")));
        assert!(!is_broker_route(Some(b"__route__GET__/api")));
        assert!(!is_broker_route(None));
    }

    #[test]
    fn classifies_sveltekit_server_files() {
        assert_eq!(
            sveltekit_file_kind(Some(b"apps/x/src/routes/+server.ts")),
            1
        );
        assert_eq!(
            sveltekit_file_kind(Some(b"apps/x/src/routes/foo/+server.js")),
            1
        );
        assert_eq!(
            sveltekit_file_kind(Some(b"apps/x/src/routes/foo/+page.server.js")),
            2
        );
        assert_eq!(
            sveltekit_file_kind(Some(b"apps/x/src/routes/foo/+page.server.ts")),
            2
        );
        assert_eq!(
            sveltekit_file_kind(Some(b"apps/x/src/routes/foo/+layout.server.ts")),
            3
        );
        assert_eq!(
            sveltekit_file_kind(Some(b"apps/x/src/routes/foo/+layout.server.js")),
            3
        );
        assert_eq!(sveltekit_file_kind(Some(b"apps/x/src/+server.ts")), 0);
        assert_eq!(sveltekit_file_kind(Some(b"apps/x/src/routes/+page.ts")), 0);
        assert_eq!(
            sveltekit_file_kind(Some(b"apps/x/src/routes/+server.jsx")),
            0
        );
        assert_eq!(sveltekit_file_kind(None), 0);
    }

    #[test]
    fn classifies_sveltekit_server_methods() {
        assert_eq!(
            sveltekit_server_method(Some(b"GET")),
            Some(b"GET".as_slice())
        );
        assert_eq!(
            sveltekit_server_method(Some(b"fallback")),
            Some(b"ANY".as_slice())
        );
        assert_eq!(sveltekit_server_method(Some(b"CONNECT")), None);
        assert_eq!(sveltekit_server_method(None), None);
    }

    #[test]
    fn extracts_route_json_properties() {
        assert_eq!(
            extract_json_prop(
                Some(br#"{"route_path":"/api/items","route_method":"GET"}"#),
                Some(b"route_path"),
                64
            ),
            Some(b"/api/items".to_vec())
        );
        assert_eq!(
            extract_json_prop(
                Some(br#"{"route_path":"/api/items"}"#),
                Some(b"route_path"),
                5
            ),
            None
        );
        assert_eq!(
            extract_json_prop(Some(b"{}"), Some(b"route_path"), 64),
            None
        );
        assert_eq!(extract_json_prop(None, Some(b"route_path"), 64), None);
    }

    #[test]
    fn extracts_route_json_property_contract_boundaries() {
        assert_eq!(
            extract_json_prop(Some(br#"{"":"v"}"#), Some(b""), 2),
            Some(b"v".to_vec())
        );
        assert_eq!(
            extract_json_prop(Some(br#"{"empty":""}"#), Some(b"empty"), 1),
            None
        );
        assert_eq!(
            extract_json_prop(Some(br#"{"key":"abc"}"#), Some(b"key"), 4),
            Some(b"abc".to_vec())
        );
        assert_eq!(
            extract_json_prop(Some(br#"{"key":"abc"}"#), Some(b"key"), 3),
            None
        );

        let key_59 = vec![b'k'; JSON_PROP_MAX_KEY_LEN];
        let mut json_59 = b"{\"".to_vec();
        json_59.extend_from_slice(&key_59);
        json_59.extend_from_slice(b"\":\"v\"}");
        assert_eq!(
            extract_json_prop(Some(&json_59), Some(&key_59), 2),
            Some(b"v".to_vec())
        );

        let key_60 = vec![b'k'; JSON_PROP_MAX_KEY_LEN + 1];
        let mut json_60 = b"{\"".to_vec();
        json_60.extend_from_slice(&key_60);
        json_60.extend_from_slice(b"\":\"v\"}");
        assert_eq!(extract_json_prop(Some(&json_60), Some(&key_60), 2), None);

        assert_eq!(
            extract_json_prop(Some(br#"{"raw":"a\"b"}"#), Some(b"raw"), 8),
            Some(b"a\\".to_vec())
        );
        assert_eq!(
            extract_json_prop(Some(br#"{"raw" :"value"}"#), Some(b"raw"), 8),
            None
        );
    }

    #[test]
    fn extracts_url_paths() {
        assert_eq!(
            url_path(Some(b"https://example.test/api/items")),
            Some(b"/api/items".as_slice())
        );
        assert_eq!(
            url_path(Some(b"https://example.test")),
            Some(b"/".as_slice())
        );
        assert_eq!(
            url_path(Some(b"/api/items")),
            Some(b"/api/items".as_slice())
        );
        assert_eq!(url_path(None), None);

        assert_eq!(
            url_path_borrowed(Some(b"https://example.test/")),
            Some(UrlPath::Input(b"/"))
        );
        assert_eq!(
            url_path_borrowed(Some(b"https://example.test")),
            Some(UrlPath::StaticRoot)
        );
        assert_eq!(
            url_path_borrowed(Some(b"https:/example.test/path")),
            Some(UrlPath::Input(b"https:/example.test/path"))
        );
        assert_eq!(
            url_path_borrowed(Some(b"://example.test/path")),
            Some(UrlPath::Input(b"/path"))
        );
    }
}
