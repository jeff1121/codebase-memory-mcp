//! `src/pipeline/pass_k8s.c` 的純文字 helper parity。
//!
//! K8s manifest scanning、YAML path stack、檔案 I/O、graph mutation 與 pipeline
//! side effects 仍由 C 負責。

#[must_use]
pub fn basename_offset(path: Option<&[u8]>) -> usize {
    path.and_then(|path| path.iter().rposition(|byte| *byte == b'/'))
        .map_or(0, |index| index + 1)
}

#[must_use]
pub fn indent(line: Option<&[u8]>) -> usize {
    line.unwrap_or_default()
        .iter()
        .take_while(|byte| **byte == b' ')
        .count()
}

#[must_use]
pub fn split_kv(
    text: Option<&[u8]>,
    key_size: usize,
    value_size: usize,
) -> Option<(Vec<u8>, Vec<u8>)> {
    let text = text?;
    if key_size == 0 || matches!(text.first().copied(), Some(b'#' | b'-' | 0)) {
        return None;
    }
    let colon = text.iter().position(|byte| *byte == b':')?;
    if colon == 0 || colon >= key_size {
        return None;
    }

    let mut value = &text[colon + 1..];
    while matches!(value.first().copied(), Some(b' ' | b'\t')) {
        value = &value[1..];
    }
    let value_end = value
        .iter()
        .position(|byte| matches!(byte, b'\r' | b'\n' | b'#'))
        .unwrap_or(value.len());
    let max_value_len = value_size.saturating_sub(1);
    let mut value = value[..value_end.min(max_value_len)].to_vec();
    while matches!(value.last().copied(), Some(b' ' | b'\t')) {
        value.pop();
    }
    if value.len() >= 2 && matches!(value[0], b'"' | b'\'') && value[value.len() - 1] == value[0] {
        value.remove(0);
        value.pop();
    }
    Some((text[..colon].to_vec(), value))
}

#[cfg(test)]
mod tests {
    use super::{basename_offset, indent, split_kv};

    #[test]
    fn basename_and_indent_match_c_contract() {
        assert_eq!(basename_offset(Some(b"k8s/apps/deploy.yaml")), 9);
        assert_eq!(basename_offset(Some(b"deploy.yaml")), 0);
        assert_eq!(basename_offset(None), 0);
        assert_eq!(indent(Some(b"  name: app")), 2);
        assert_eq!(indent(Some(b"\tname")), 0);
        assert_eq!(indent(None), 0);
    }

    #[test]
    fn split_kv_matches_c_contract() {
        assert_eq!(
            split_kv(Some(b"name: \"frontend\" # comment"), 32, 32),
            Some((b"name".to_vec(), b"frontend".to_vec()))
        );
        assert_eq!(
            split_kv(Some(b"image:"), 32, 32),
            Some((b"image".to_vec(), Vec::new()))
        );
        assert_eq!(split_kv(Some(b"# comment"), 32, 32), None);
        assert_eq!(split_kv(Some(b"- item"), 32, 32), None);
        assert_eq!(split_kv(Some(b"verylong: value"), 4, 32), None);
        assert_eq!(
            split_kv(Some(b"key: abcdef"), 32, 5),
            Some((b"key".to_vec(), b"abcd".to_vec()))
        );
    }
}
