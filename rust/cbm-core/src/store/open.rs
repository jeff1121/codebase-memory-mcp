//! Store open-path string helpers.
//!
//! 這裡只建構 SQLite immutable URI fallback 字串；不開 SQLite、不執行 pragma。

const PREFIX: &[u8] = b"file://";
const SUFFIX: &[u8] = b"?immutable=1";
const HEX: &[u8; 16] = b"0123456789ABCDEF";

fn is_uri_safe_path_byte(c: u8) -> bool {
    c.is_ascii_alphanumeric() || matches!(c, b'/' | b'.' | b'-' | b'_' | b'~' | b':')
}

#[must_use]
pub fn immutable_uri(path: Option<&[u8]>) -> Option<Vec<u8>> {
    let path = path?;
    let mut out = Vec::with_capacity(PREFIX.len() + 1 + path.len() * 3 + SUFFIX.len());
    out.extend_from_slice(PREFIX);

    if path.first().copied() != Some(b'/') {
        out.push(b'/');
    }

    for &raw in path {
        let c = if raw == b'\\' { b'/' } else { raw };
        if is_uri_safe_path_byte(c) {
            out.push(c);
        } else {
            out.push(b'%');
            out.push(HEX[((c >> 4) & 0x0f) as usize]);
            out.push(HEX[(c & 0x0f) as usize]);
        }
    }

    out.extend_from_slice(SUFFIX);
    Some(out)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn builds_posix_immutable_uri_like_c_helper() {
        assert_eq!(immutable_uri(Some(b"/")).unwrap(), b"file:///?immutable=1");
        assert_eq!(
            immutable_uri(Some(b"tmp/a.db")).unwrap(),
            b"file:///tmp/a.db?immutable=1"
        );
        assert_eq!(
            immutable_uri(Some(b"/tmp/codebase memory/graph.db")).unwrap(),
            b"file:///tmp/codebase%20memory/graph.db?immutable=1"
        );
        assert_eq!(
            immutable_uri(Some(b"/tmp/a?b#c&d=e.db")).unwrap(),
            b"file:///tmp/a%3Fb%23c%26d%3De.db?immutable=1"
        );
        assert_eq!(
            immutable_uri(Some(b"/tmp/a%20b.db")).unwrap(),
            b"file:///tmp/a%2520b.db?immutable=1"
        );
    }

    #[test]
    fn normalizes_windows_drive_paths_and_keeps_safe_chars() {
        assert_eq!(
            immutable_uri(Some(b"C:\\Users\\dev\\graph.db")).unwrap(),
            b"file:///C:/Users/dev/graph.db?immutable=1"
        );
        assert_eq!(
            immutable_uri(Some(b"C:/Users/dev/graph.db")).unwrap(),
            b"file:///C:/Users/dev/graph.db?immutable=1"
        );
        assert_eq!(
            immutable_uri(Some(b"\\tmp\\a.db")).unwrap(),
            b"file:////tmp/a.db?immutable=1"
        );
        assert_eq!(
            immutable_uri(Some(b"\\\\server\\share\\db.sqlite")).unwrap(),
            b"file://///server/share/db.sqlite?immutable=1"
        );
        assert_eq!(
            immutable_uri(Some(b"//server/share/db.sqlite")).unwrap(),
            b"file:////server/share/db.sqlite?immutable=1"
        );
        assert_eq!(
            immutable_uri(Some(b"/tmp/AZaz09/.-_~:.db")).unwrap(),
            b"file:///tmp/AZaz09/.-_~:.db?immutable=1"
        );
    }

    #[test]
    fn encodes_raw_bytes_and_handles_null_or_empty_path() {
        assert_eq!(immutable_uri(None), None);
        assert_eq!(immutable_uri(Some(b"")).unwrap(), b"file:///?immutable=1");
        assert_eq!(
            immutable_uri(Some(b"/tmp/\xe5\xbc\x80.db")).unwrap(),
            b"file:///tmp/%E5%BC%80.db?immutable=1"
        );
    }
}
