//! `src/pipeline/fqn.c` 的 deterministic path-to-project-name contract。

use crate::foundation::platform;

const ROOT: &[u8] = b"root";

#[must_use]
pub fn project_name_from_path_bytes(path: Option<&[u8]>) -> Vec<u8> {
    let Some(path) = path else {
        return ROOT.to_vec();
    };
    if path.is_empty() {
        return ROOT.to_vec();
    }

    let mut normalized = path.to_vec();
    platform::normalize_path_sep_bytes(&mut normalized);

    let mut mapped = Vec::with_capacity(normalized.len().saturating_mul(2));
    for byte in normalized {
        if byte.is_ascii_alphanumeric() || matches!(byte, b'.' | b'_' | b'-') {
            mapped.push(byte);
        } else if byte >= 0x80 {
            mapped.push(hex_digit(byte >> 4));
            mapped.push(hex_digit(byte & 0x0f));
        } else {
            mapped.push(b'-');
        }
    }

    let mut collapsed = Vec::with_capacity(mapped.len());
    let mut previous = 0;
    for byte in mapped {
        if (byte == b'-' && previous == b'-') || (byte == b'.' && previous == b'.') {
            continue;
        }
        collapsed.push(byte);
        previous = byte;
    }

    let start = collapsed
        .iter()
        .position(|byte| !matches!(*byte, b'-' | b'.'))
        .unwrap_or(collapsed.len());
    let mut end = collapsed.len();
    while end > start && collapsed[end - 1] == b'-' {
        end -= 1;
    }

    if start == end {
        ROOT.to_vec()
    } else {
        collapsed[start..end].to_vec()
    }
}

fn hex_digit(nibble: u8) -> u8 {
    match nibble {
        0..=9 => b'0' + nibble,
        10..=15 => b'a' + (nibble - 10),
        _ => unreachable!("nibble is masked before hex conversion"),
    }
}

#[must_use]
pub fn fqn_compute(
    project: Option<&[u8]>,
    rel_path: Option<&[u8]>,
    name: Option<&[u8]>,
) -> Vec<u8> {
    let Some(project) = project else {
        return Vec::new();
    };

    let mut path = normalize_separators(rel_path.unwrap_or_default());
    strip_file_extension(&mut path);
    let mut segments = split_non_empty(&path, 254);
    let has_name = name.is_some_and(|value| !value.is_empty());
    if has_name
        && segments.last().is_some_and(|segment| {
            segment.as_slice() == b"__init__" || segment.as_slice() == b"index"
        })
    {
        segments.pop();
    }

    let mut joined =
        Vec::with_capacity(project.len() + path.len() + name.map_or(0, |v| v.len()) + 1);
    joined.extend_from_slice(project);
    for segment in segments {
        joined.push(b'.');
        joined.extend_from_slice(&segment);
    }
    if has_name {
        joined.push(b'.');
        joined.extend_from_slice(name.expect("name is present when has_name is true"));
    }
    joined
}

#[must_use]
pub fn fqn_module(project: Option<&[u8]>, rel_path: Option<&[u8]>) -> Vec<u8> {
    fqn_compute(project, rel_path, None)
}

#[must_use]
pub fn fqn_module_dir(
    project: Option<&[u8]>,
    rel_path: Option<&[u8]>,
    module_is_dir: bool,
) -> Vec<u8> {
    if !module_is_dir {
        return fqn_module(project, rel_path);
    }
    let path = rel_path.unwrap_or_default();
    let last_forward = path.iter().rposition(|byte| *byte == b'/');
    let last_backward = path.iter().rposition(|byte| *byte == b'\\');
    let last_separator = match (last_forward, last_backward) {
        (Some(forward), Some(backward)) => Some(forward.max(backward)),
        (Some(forward), None) => Some(forward),
        (None, Some(backward)) => Some(backward),
        (None, None) => None,
    };
    let directory = last_separator.map_or(&[][..], |index| &path[..index]);
    fqn_folder(project, Some(directory))
}

#[must_use]
pub fn fqn_folder(project: Option<&[u8]>, rel_dir: Option<&[u8]>) -> Vec<u8> {
    let Some(project) = project else {
        return Vec::new();
    };
    let normalized = normalize_separators(rel_dir.unwrap_or_default());
    let segments = split_non_empty(&normalized, 254);
    let mut joined = Vec::with_capacity(project.len() + normalized.len() + 1);
    joined.extend_from_slice(project);
    for segment in segments {
        joined.push(b'.');
        joined.extend_from_slice(&segment);
    }
    joined
}

#[must_use]
pub fn resolve_relative_import(
    source_rel: Option<&[u8]>,
    module_path: Option<&[u8]>,
) -> Option<Vec<u8>> {
    let module_path = module_path?;
    let kind = classify_relative_import(module_path)?;
    let mut path = seed_source_dir(source_rel.unwrap_or_default());
    match kind {
        RelativeImportKind::Python => resolve_python_relative(&mut path, module_path),
        RelativeImportKind::Javascript => resolve_javascript_relative(&mut path, module_path),
    }
}

fn normalize_separators(path: &[u8]) -> Vec<u8> {
    path.iter()
        .map(|byte| if *byte == b'\\' { b'/' } else { *byte })
        .collect()
}

fn strip_file_extension(path: &mut Vec<u8>) {
    let start = path
        .iter()
        .rposition(|byte| *byte == b'/')
        .map_or(0, |index| index + 1);
    if let Some(relative_dot) = path[start..].iter().rposition(|byte| *byte == b'.') {
        path.truncate(start + relative_dot);
    }
}

fn split_non_empty(path: &[u8], max_segments: usize) -> Vec<Vec<u8>> {
    path.split(|byte| *byte == b'/')
        .filter(|segment| !segment.is_empty())
        .take(max_segments)
        .map(ToOwned::to_owned)
        .collect()
}

#[derive(Clone, Copy)]
enum RelativeImportKind {
    Python,
    Javascript,
}

fn classify_relative_import(module_path: &[u8]) -> Option<RelativeImportKind> {
    if module_path.first() != Some(&b'.') {
        return None;
    }
    let has_slash = module_path.contains(&b'/');
    let javascript_like = module_path.get(1) == Some(&b'/')
        || (module_path.get(1) == Some(&b'.') && module_path.get(2) == Some(&b'/'));
    if has_slash || javascript_like {
        Some(RelativeImportKind::Javascript)
    } else {
        Some(RelativeImportKind::Python)
    }
}

fn seed_source_dir(source_rel: &[u8]) -> Vec<u8> {
    let mut path = normalize_separators(source_rel);
    path.truncate(1023);
    if let Some(separator) = path.iter().rposition(|byte| *byte == b'/') {
        path.truncate(separator);
    } else {
        path.clear();
    }
    path
}

fn append_path_segment(path: &mut Vec<u8>, segment: &[u8]) -> bool {
    let separator = usize::from(!path.is_empty());
    if path.len() + separator + segment.len() + 1 > 1024 {
        return false;
    }
    if !path.is_empty() {
        path.push(b'/');
    }
    path.extend_from_slice(segment);
    true
}

fn pop_path_segment(path: &mut Vec<u8>) {
    if let Some(separator) = path.iter().rposition(|byte| *byte == b'/') {
        path.truncate(separator);
    } else {
        path.clear();
    }
}

fn resolve_python_relative(path: &mut Vec<u8>, module_path: &[u8]) -> Option<Vec<u8>> {
    let dot_count = module_path.iter().take_while(|byte| **byte == b'.').count();
    for _ in 1..dot_count {
        pop_path_segment(path);
    }
    let rest = &module_path[dot_count..];
    for segment in rest
        .split(|byte| *byte == b'.')
        .filter(|segment| !segment.is_empty())
    {
        if !append_path_segment(path, segment) {
            return None;
        }
    }
    Some(path.clone())
}

fn resolve_javascript_relative(path: &mut Vec<u8>, module_path: &[u8]) -> Option<Vec<u8>> {
    let mut cursor = 0;
    while cursor < module_path.len() {
        while cursor < module_path.len() && module_path[cursor] == b'/' {
            cursor += 1;
        }
        if cursor == module_path.len() {
            break;
        }
        let start = cursor;
        while cursor < module_path.len() && module_path[cursor] != b'/' {
            cursor += 1;
        }
        let mut segment = &module_path[start..cursor];
        if segment == b"." {
            continue;
        }
        if segment == b".." {
            pop_path_segment(path);
            continue;
        }
        if cursor == module_path.len() {
            segment = strip_segment_extension(segment);
        }
        if !segment.is_empty() && !append_path_segment(path, segment) {
            return None;
        }
    }
    Some(path.clone())
}

fn strip_segment_extension(segment: &[u8]) -> &[u8] {
    match segment.iter().rposition(|byte| *byte == b'.') {
        Some(dot) if dot > 0 => &segment[..dot],
        _ => segment,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn project_name(path: Option<&[u8]>) -> Vec<u8> {
        project_name_from_path_bytes(path)
    }

    #[test]
    fn handles_empty_separator_and_drive_contracts() {
        assert_eq!(project_name(None), b"root");
        assert_eq!(project_name(Some(b"")), b"root");
        assert_eq!(project_name(Some(b"///")), b"root");
        assert_eq!(
            project_name(Some(br"C:\Users\dev\project")),
            b"C-Users-dev-project"
        );
        assert_eq!(
            project_name(Some(b"c:/WEBDEV/Cardio-Cloud")),
            b"C-WEBDEV-Cardio-Cloud"
        );
    }

    #[test]
    fn maps_unsafe_ascii_and_collapses_validator_rejections() {
        assert_eq!(
            project_name(Some(b"/Users/dev/my-project")),
            b"Users-dev-my-project"
        );
        assert_eq!(
            project_name(Some(b"/home/u/my project")),
            b"home-u-my-project"
        );
        assert_eq!(project_name(Some(b"/x/a..b/repo")), b"x-a.b-repo");
        assert_eq!(project_name(Some(b":")), b"root");
        assert_eq!(project_name(Some(b"a::b")), b"a-b");
    }

    #[test]
    fn encodes_non_ascii_bytes_as_lowercase_hex() {
        assert_eq!(
            project_name(Some(b"/Users/dev/caf\xc3\xa9app")),
            b"Users-dev-cafc3a9app"
        );
        assert_eq!(project_name(Some(b"/tmp/\xe5\xbc\x80")), b"tmp-e5bc80");
    }

    #[test]
    fn resolves_python_relative_imports_against_source_directory() {
        assert_eq!(
            resolve_relative_import(Some(b"src/pkg/module.py"), Some(b".helpers")),
            Some(b"src/pkg/helpers".to_vec())
        );
        assert_eq!(
            resolve_relative_import(Some(b"src/pkg/module.py"), Some(b"..shared")),
            Some(b"src/shared".to_vec())
        );
        assert_eq!(
            resolve_relative_import(Some(b"src/pkg/module.py"), Some(b"...root")),
            Some(b"root".to_vec())
        );
    }

    #[test]
    fn resolves_javascript_relative_imports_and_extensions() {
        assert_eq!(
            resolve_relative_import(Some(b"src/pkg/module.ts"), Some(b"./helpers.ts")),
            Some(b"src/pkg/helpers".to_vec())
        );
        assert_eq!(
            resolve_relative_import(Some(b"src/pkg/module.ts"), Some(b"../shared/index.js")),
            Some(b"src/shared/index".to_vec())
        );
        assert_eq!(
            resolve_relative_import(Some(b"main.js"), Some(b"./entry")),
            Some(b"entry".to_vec())
        );
        assert_eq!(
            resolve_relative_import(Some(b"main.js"), Some(b"lodash")),
            None
        );
    }

    #[test]
    fn relative_import_respects_c_path_buffer_limit() {
        let mut source = vec![b'a'; 1020];
        source.extend_from_slice(b"/module.py");
        assert_eq!(
            resolve_relative_import(Some(&source), Some(b"./helper")),
            None
        );
    }
}
