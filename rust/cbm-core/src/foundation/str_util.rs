//! 對齊 `src/foundation/str_util.c` 的 string 與 path helpers。

/// 組合兩個以 slash 分隔的 path components。
pub fn path_join(base: Option<&str>, name: Option<&str>) -> Option<String> {
    path_join_bytes(base.map(str::as_bytes), name.map(str::as_bytes))
        .map(|bytes| String::from_utf8_lossy(&bytes).into_owned())
}

pub fn path_join_bytes(base: Option<&[u8]>, name: Option<&[u8]>) -> Option<Vec<u8>> {
    let mut base = base?;
    let mut name = name?;

    if base.is_empty() {
        return Some(name.to_vec());
    }
    if name.is_empty() {
        return Some(base.to_vec());
    }

    while base.ends_with(b"/") {
        base = &base[..base.len() - 1];
    }
    while name.starts_with(b"/") {
        name = &name[1..];
    }

    match (base.is_empty(), name.is_empty()) {
        (true, _) => Some(name.to_vec()),
        (_, true) => Some(base.to_vec()),
        (false, false) => {
            let mut out = Vec::with_capacity(base.len() + 1 + name.len());
            out.extend_from_slice(base);
            out.push(b'/');
            out.extend_from_slice(name);
            Some(out)
        }
    }
}

/// 組合 N 個以 slash 分隔的 path components。
pub fn path_join_n(parts: Option<&[&str]>) -> String {
    let Some(parts) = parts else {
        return String::new();
    };
    let Some((first, rest)) = parts.split_first() else {
        return String::new();
    };

    let mut joined = (*first).to_owned();
    for part in rest {
        joined = path_join(Some(&joined), Some(part)).unwrap_or_default();
    }
    joined
}

/// 回傳不含 dot 的副檔名。
pub fn path_ext(path: Option<&str>) -> &str {
    let Some(path) = path else {
        return "";
    };
    let Some(dot) = path.rfind('.') else {
        return "";
    };
    if path.rfind('/').is_some_and(|slash| dot < slash) {
        return "";
    }
    &path[dot + 1..]
}

pub fn path_ext_offset(path: Option<&[u8]>) -> Option<usize> {
    let path = path?;
    let dot = path.iter().rposition(|byte| *byte == b'.')?;
    if path
        .iter()
        .rposition(|byte| *byte == b'/')
        .is_some_and(|slash| dot < slash)
    {
        return None;
    }
    Some(dot + 1)
}

/// 回傳最後一個 slash 後的 base name。
pub fn path_base(path: Option<&str>) -> &str {
    let Some(path) = path else {
        return "";
    };
    path.rsplit_once('/').map_or(path, |(_, base)| base)
}

pub fn path_base_offset(path: Option<&[u8]>) -> Option<usize> {
    let path = path?;
    Some(
        path.iter()
            .rposition(|byte| *byte == b'/')
            .map_or(0, |slash| slash + 1),
    )
}

/// 回傳最後一個 slash 前的 directory part。
pub fn path_dir(path: Option<&str>) -> String {
    let Some(path) = path else {
        return ".".to_owned();
    };
    path.rsplit_once('/')
        .map_or_else(|| ".".to_owned(), |(dir, _)| dir.to_owned())
}

pub fn path_dir_bytes(path: Option<&[u8]>) -> Vec<u8> {
    let Some(path) = path else {
        return b".".to_vec();
    };
    path.iter()
        .rposition(|byte| *byte == b'/')
        .map_or_else(|| b".".to_vec(), |slash| path[..slash].to_vec())
}

pub fn starts_with(s: Option<&str>, prefix: Option<&str>) -> bool {
    match (s, prefix) {
        (Some(s), Some(prefix)) => starts_with_bytes(Some(s.as_bytes()), Some(prefix.as_bytes())),
        _ => false,
    }
}

pub fn ends_with(s: Option<&str>, suffix: Option<&str>) -> bool {
    match (s, suffix) {
        (Some(s), Some(suffix)) => ends_with_bytes(Some(s.as_bytes()), Some(suffix.as_bytes())),
        _ => false,
    }
}

pub fn contains(s: Option<&str>, sub: Option<&str>) -> bool {
    match (s, sub) {
        (Some(s), Some(sub)) => contains_bytes(Some(s.as_bytes()), Some(sub.as_bytes())),
        _ => false,
    }
}

pub fn starts_with_bytes(s: Option<&[u8]>, prefix: Option<&[u8]>) -> bool {
    match (s, prefix) {
        (Some(s), Some(prefix)) => s.starts_with(prefix),
        _ => false,
    }
}

pub fn ends_with_bytes(s: Option<&[u8]>, suffix: Option<&[u8]>) -> bool {
    match (s, suffix) {
        (Some(s), Some(suffix)) => s.ends_with(suffix),
        _ => false,
    }
}

pub fn contains_bytes(s: Option<&[u8]>, sub: Option<&[u8]>) -> bool {
    match (s, sub) {
        (Some(s), Some(sub)) => sub.is_empty() || s.windows(sub.len()).any(|window| window == sub),
        _ => false,
    }
}

pub fn to_lower(s: Option<&str>) -> Option<String> {
    to_lower_bytes(s.map(str::as_bytes)).map(|bytes| String::from_utf8_lossy(&bytes).into_owned())
}

pub fn to_lower_bytes(s: Option<&[u8]>) -> Option<Vec<u8>> {
    s.map(|value| value.iter().map(u8::to_ascii_lowercase).collect())
}

pub fn replace_char(s: Option<&str>, from: char, to: char) -> Option<String> {
    replace_char_bytes(s.map(str::as_bytes), from as u8, to as u8)
        .map(|bytes| String::from_utf8_lossy(&bytes).into_owned())
}

pub fn replace_char_bytes(s: Option<&[u8]>, from: u8, to: u8) -> Option<Vec<u8>> {
    s.map(|value| {
        value
            .iter()
            .copied()
            .map(|byte| if byte == from { to } else { byte })
            .collect()
    })
}

pub fn strip_ext(path: Option<&str>) -> Option<String> {
    strip_ext_bytes(path.map(str::as_bytes))
        .map(|bytes| String::from_utf8_lossy(&bytes).into_owned())
}

pub fn strip_ext_bytes(path: Option<&[u8]>) -> Option<Vec<u8>> {
    let path = path?;
    let Some(dot) = path.iter().rposition(|byte| *byte == b'.') else {
        return Some(path.to_vec());
    };
    if path
        .iter()
        .rposition(|byte| *byte == b'/')
        .is_some_and(|slash| dot < slash)
    {
        return Some(path.to_vec());
    }
    Some(path[..dot].to_vec())
}

pub fn split(s: Option<&str>, delim: char) -> Option<Vec<String>> {
    s.map(|value| value.split(delim).map(str::to_owned).collect())
}

pub fn split_bytes(s: Option<&[u8]>, delim: u8) -> Option<Vec<&[u8]>> {
    let s = s?;
    Some(s.split(|byte| *byte == delim).collect())
}

pub fn validate_shell_arg(s: Option<&str>) -> bool {
    validate_shell_arg_bytes(s.map(str::as_bytes))
}

pub fn validate_shell_arg_bytes(s: Option<&[u8]>) -> bool {
    let Some(s) = s else {
        return false;
    };

    s.iter().all(|byte| match *byte {
        b'\'' | b'"' | b';' | b'|' | b'&' | b'$' | b'`' | b'<' | b'>' | b'\n' | b'\r' => false,
        b'\\' if !cfg!(windows) => false,
        _ => true,
    })
}

pub fn validate_project_name(name: Option<&str>) -> bool {
    validate_project_name_bytes(name.map(str::as_bytes))
}

pub fn validate_project_name_bytes(name: Option<&[u8]>) -> bool {
    let Some(name) = name else {
        return false;
    };
    if name.is_empty()
        || name == b".."
        || name.windows(2).any(|window| window == b"..")
        || name.contains(&b'/')
        || name.contains(&b'\\')
        || name.starts_with(b".")
    {
        return false;
    }

    name.iter()
        .copied()
        .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_' | b'.'))
}

/// 將 bytes escape 成 JSON，並限制在指定 buffer size 內。
///
/// `bufsize` 包含 C NUL terminator slot，因此回傳 bytes 最多是
/// `bufsize - 1` 長度。
pub fn json_escape(src: Option<&[u8]>, bufsize: usize) -> (Vec<u8>, usize) {
    if bufsize == 0 {
        return (Vec::new(), 0);
    }
    let Some(src) = src else {
        return (Vec::new(), 0);
    };

    let limit = bufsize - 1;
    let mut out = Vec::with_capacity(limit.min(src.len()));
    for &byte in src {
        let escaped: &[u8] = match byte {
            b'"' => br#"\""#,
            b'\\' => br#"\\"#,
            b'\n' => br#"\n"#,
            b'\r' => br#"\r"#,
            b'\t' => br#"\t"#,
            0x00..=0x1f => {
                let encoded = format!("\\u{byte:04x}");
                if out.len() + encoded.len() > limit {
                    break;
                }
                out.extend_from_slice(encoded.as_bytes());
                continue;
            }
            _ => {
                if out.len() + 1 > limit {
                    break;
                }
                out.push(byte);
                continue;
            }
        };

        if out.len() + escaped.len() > limit {
            break;
        }
        out.extend_from_slice(escaped);
    }

    let len = out.len();
    (out, len)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn path_join_matches_c_cases() {
        assert_eq!(
            path_join(Some("src"), Some("main.c")).as_deref(),
            Some("src/main.c")
        );
        assert_eq!(
            path_join(Some("src/"), Some("main.c")).as_deref(),
            Some("src/main.c")
        );
        assert_eq!(
            path_join(Some("src"), Some("/main.c")).as_deref(),
            Some("src/main.c")
        );
        assert_eq!(
            path_join(Some(""), Some("main.c")).as_deref(),
            Some("main.c")
        );
        assert_eq!(path_join(Some("src"), Some("")).as_deref(), Some("src"));
        assert_eq!(
            path_join(Some("src///"), Some("///main.c")).as_deref(),
            Some("src/main.c")
        );
        assert_eq!(
            path_join(Some("/"), Some("main.c")).as_deref(),
            Some("main.c")
        );
        assert_eq!(path_join(Some("/"), Some("/")).as_deref(), Some(""));
        assert_eq!(path_join(Some("src"), Some("///")).as_deref(), Some("src"));
        assert_eq!(path_join(None, Some("main.c")), None);
        assert_eq!(path_join(Some("src"), None), None);
    }

    #[test]
    fn path_join_n_matches_c_cases() {
        assert_eq!(path_join_n(Some(&["a", "b", "c", "d.txt"])), "a/b/c/d.txt");
        assert_eq!(path_join_n(Some(&["single"])), "single");
        assert_eq!(path_join_n(Some(&[])), "");
        assert_eq!(path_join_n(None), "");
    }

    #[test]
    fn path_component_helpers_match_c_cases() {
        assert_eq!(path_ext(Some("foo.go")), "go");
        assert_eq!(path_ext(Some("foo.tar.gz")), "gz");
        assert_eq!(path_ext(Some("Makefile")), "");
        assert_eq!(path_ext(Some(".gitignore")), "gitignore");
        assert_eq!(path_ext(Some("dir.name/file")), "");
        assert_eq!(path_ext(Some("file.test.spec.ts")), "ts");
        assert_eq!(path_ext(Some("file.")), "");
        assert_eq!(path_ext(None), "");

        assert_eq!(path_base(Some("src/main.c")), "main.c");
        assert_eq!(path_base(Some("main.c")), "main.c");
        assert_eq!(path_base(Some("/a/b/c")), "c");
        assert_eq!(path_base(Some("dir/")), "");
        assert_eq!(path_base(None), "");

        assert_eq!(path_dir(Some("src/main.c")), "src");
        assert_eq!(path_dir(Some("main.c")), ".");
        assert_eq!(path_dir(Some("/a/b/c")), "/a/b");
        assert_eq!(path_dir(Some("/")), "");
        assert_eq!(path_dir(Some("dir/")), "dir");
        assert_eq!(path_dir(None), ".");
    }

    #[test]
    fn string_predicates_match_c_cases() {
        assert!(starts_with(Some("hello world"), Some("hello")));
        assert!(!starts_with(Some("hello"), Some("hello world")));
        assert!(starts_with(Some("hello"), Some("")));
        assert!(!starts_with(None, Some("hello")));
        assert!(!starts_with(Some("hello"), None));

        assert!(ends_with(Some("hello world"), Some("world")));
        assert!(!ends_with(Some("hello"), Some("hello world")));
        assert!(ends_with(Some("hello"), Some("")));
        assert!(!ends_with(None, Some("world")));
        assert!(!ends_with(Some("world"), None));

        assert!(contains(Some("hello world"), Some("lo wo")));
        assert!(!contains(Some("hello"), Some("xyz")));
        assert!(contains(Some("hello"), Some("")));
        assert!(!contains(None, Some("sub")));
        assert!(!contains(Some("hello"), None));
    }

    #[test]
    fn string_transform_helpers_match_c_cases() {
        assert_eq!(
            to_lower(Some("Hello World")).as_deref(),
            Some("hello world")
        );
        assert_eq!(to_lower(Some("already")).as_deref(), Some("already"));
        assert_eq!(to_lower(Some("")).as_deref(), Some(""));
        assert_eq!(to_lower(None), None);

        assert_eq!(
            replace_char(Some("a/b/c"), '/', '.').as_deref(),
            Some("a.b.c")
        );
        assert_eq!(
            replace_char(Some("no-change"), '/', '.').as_deref(),
            Some("no-change")
        );
        assert_eq!(replace_char(None, 'a', 'b'), None);

        assert_eq!(strip_ext(Some("foo.go")).as_deref(), Some("foo"));
        assert_eq!(strip_ext(Some("foo.tar.gz")).as_deref(), Some("foo.tar"));
        assert_eq!(strip_ext(Some("Makefile")).as_deref(), Some("Makefile"));
        assert_eq!(strip_ext(Some(".gitignore")).as_deref(), Some(""));
        assert_eq!(
            strip_ext(Some("dir.name/file")).as_deref(),
            Some("dir.name/file")
        );
        assert_eq!(strip_ext(Some("file.")).as_deref(), Some("file"));
        assert_eq!(strip_ext(None), None);

        assert_eq!(
            split(Some("a/b/c/d"), '/'),
            Some(vec!["a".into(), "b".into(), "c".into(), "d".into()])
        );
        assert_eq!(split(Some(""), '/'), Some(vec!["".into()]));
        assert_eq!(split(Some("hello"), '/'), Some(vec!["hello".into()]));
        assert_eq!(
            split(Some("/a//b/"), '/'),
            Some(vec![
                "".into(),
                "a".into(),
                "".into(),
                "b".into(),
                "".into()
            ])
        );
        assert_eq!(split(None, '/'), None);
    }

    #[test]
    fn validate_shell_arg_matches_c_cases() {
        assert!(!validate_shell_arg(None));
        assert!(validate_shell_arg(Some("hello-world_123")));
        assert!(validate_shell_arg(Some("")));
        assert!(validate_shell_arg(Some("hello world with spaces")));
        for value in [
            "it's bad",
            "\"bad\"",
            "cmd; rm -rf",
            "cmd | evil",
            "cmd & bg",
            "$HOME",
            "`whoami`",
            "line1\nline2",
            "line1\rline2",
            "out>file",
            "in<file",
        ] {
            assert!(!validate_shell_arg(Some(value)), "{value:?}");
        }
        assert_eq!(validate_shell_arg(Some(r"path\to\file")), cfg!(windows));
    }

    #[test]
    fn validate_project_name_matches_c_policy() {
        assert!(!validate_project_name(None));
        assert!(!validate_project_name(Some("")));
        assert!(!validate_project_name(Some("..")));
        assert!(!validate_project_name(Some("a..b")));
        assert!(!validate_project_name(Some("dir/name")));
        assert!(!validate_project_name(Some(r"dir\name")));
        assert!(!validate_project_name(Some(".hidden")));
        assert!(!validate_project_name(Some("bad name")));
        assert!(validate_project_name(Some("project-name_1.2")));
    }

    #[test]
    fn json_escape_matches_c_cases() {
        let (escaped, len) = json_escape(Some(b"A\x01B\n"), 64);
        assert_eq!(escaped, b"A\\u0001B\\n");
        assert_eq!(len, 10);

        let (escaped, len) = json_escape(Some(br#"quote" slash\"#), 64);
        assert_eq!(escaped, br#"quote\" slash\\"#);
        assert_eq!(len, escaped.len());

        let (escaped, len) = json_escape(None, 64);
        assert!(escaped.is_empty());
        assert_eq!(len, 0);

        let (escaped, len) = json_escape(Some(b"hello world"), 8);
        assert_eq!(escaped, b"hello w");
        assert_eq!(len, 7);
    }
}
