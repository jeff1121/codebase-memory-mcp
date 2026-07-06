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
}
