//! `pass_pkgmap.c` 的 bounded manifest text scan helpers。

#[must_use]
pub fn at_prefix(source: Option<&[u8]>, prefix: Option<&[u8]>, prefix_len: usize) -> bool {
    let (Some(source), Some(prefix)) = (source, prefix) else {
        return false;
    };
    prefix_len <= source.len()
        && prefix_len <= prefix.len()
        && source[..prefix_len] == prefix[..prefix_len]
}

#[must_use]
pub fn find_line_value_offset(
    source: Option<&[u8]>,
    prefix: Option<&[u8]>,
    source_len: usize,
) -> Option<usize> {
    let (Some(source), Some(prefix)) = (source, prefix) else {
        return None;
    };
    let source = source.get(..source_len.min(source.len()))?;
    let mut offset = 0;
    while offset < source.len() {
        while offset < source.len() && matches!(source[offset], b' ' | b'\t') {
            offset += 1;
        }
        if source
            .get(offset..offset.saturating_add(prefix.len()))
            .is_some_and(|value| value == prefix)
        {
            return Some(offset + prefix.len());
        }
        while offset < source.len() && source[offset] != b'\n' {
            offset += 1;
        }
        if offset < source.len() {
            offset += 1;
        }
    }
    None
}

#[must_use]
pub fn path_dirname(path: Option<&[u8]>) -> Vec<u8> {
    let Some(path) = path else {
        return Vec::new();
    };
    path.iter()
        .rposition(|byte| *byte == b'/')
        .map_or_else(Vec::new, |index| path[..index].to_vec())
}

#[must_use]
pub fn strip_extension(path: Option<&[u8]>) -> Vec<u8> {
    let Some(path) = path else {
        return Vec::new();
    };
    let start = path
        .iter()
        .rposition(|byte| *byte == b'/')
        .map_or(0, |index| index + 1);
    path[start..]
        .iter()
        .rposition(|byte| *byte == b'.')
        .map_or_else(|| path.to_vec(), |index| path[..start + index].to_vec())
}

#[must_use]
pub fn join_and_strip(dir: Option<&[u8]>, entry: Option<&[u8]>) -> Option<Vec<u8>> {
    let mut entry = entry?;
    if entry.is_empty() {
        return None;
    }
    if entry.starts_with(b"./") {
        entry = &entry[2..];
    }
    let dir = dir.unwrap_or_default();
    let mut combined = Vec::with_capacity(dir.len() + entry.len() + 1);
    if !dir.is_empty() {
        combined.extend_from_slice(dir);
        combined.push(b'/');
    }
    combined.extend_from_slice(entry);
    combined.truncate(1023);
    Some(strip_extension(Some(&combined)))
}

#[must_use]
pub fn build_entry_path(rel_path: Option<&[u8]>, suffix: Option<&[u8]>) -> Vec<u8> {
    let dir = path_dirname(rel_path);
    let suffix = suffix.unwrap_or_default();
    let mut output = Vec::with_capacity(dir.len() + suffix.len() + 1);
    if !dir.is_empty() {
        output.extend_from_slice(&dir);
        output.push(b'/');
    }
    output.extend_from_slice(suffix);
    output.truncate(1023);
    output
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn checks_bounded_prefix_without_reading_past_range() {
        assert!(at_prefix(Some(b"name="), Some(b"name="), 5));
        assert!(!at_prefix(Some(b"name"), Some(b"name="), 6));
        assert!(!at_prefix(Some(b"other"), Some(b"name="), 5));
    }

    #[test]
    fn finds_manifest_value_offset_after_indentation() {
        let source = b"other\n  module github.com/acme/project\nnext";
        assert_eq!(
            find_line_value_offset(Some(source), Some(b"module "), source.len()),
            Some(15)
        );
        assert_eq!(
            find_line_value_offset(Some(source), Some(b"missing"), source.len()),
            None
        );
    }

    #[test]
    fn extracts_directory_and_file_stem() {
        assert_eq!(
            path_dirname(Some(b"packages/foo/package.json")),
            b"packages/foo"
        );
        assert_eq!(path_dirname(Some(b"package.json")), b"");
        assert_eq!(strip_extension(Some(b"src/index.ts")), b"src/index");
        assert_eq!(strip_extension(Some(b"Makefile")), b"Makefile");
        assert_eq!(
            join_and_strip(Some(b"packages/foo"), Some(b"./src/index.ts")),
            Some(b"packages/foo/src/index".to_vec())
        );
        assert_eq!(
            build_entry_path(Some(b"packages/foo/package.toml"), Some(b"src/lib")),
            b"packages/foo/src/lib"
        );
    }

    #[test]
    fn preserves_bounded_bytes_and_null_path_contracts() {
        let source = b"x\n \tmod\0";
        assert_eq!(
            find_line_value_offset(Some(source), Some(b"mod"), source.len()),
            Some(7)
        );
        assert!(!at_prefix(None, Some(b"mod"), 3));
        assert_eq!(path_dirname(None), b"");
        assert_eq!(strip_extension(None), b"");
        assert_eq!(join_and_strip(None, None), None);
        assert_eq!(build_entry_path(None, None), b"");

        let dir = vec![b'd'; 1022];
        let joined = join_and_strip(Some(&dir), Some(b"entry.ts")).expect("entry is present");
        assert_eq!(joined.len(), 1023);
        assert_eq!(joined.last(), Some(&b'/'));
    }
}
