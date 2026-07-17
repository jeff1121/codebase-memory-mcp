use std::os::raw::c_int;

#[must_use]
pub fn path_is_absolute(path: Option<&[u8]>) -> bool {
    let Some(path) = path else {
        return false;
    };
    if path.first() == Some(&b'/') {
        return true;
    }

    #[cfg(windows)]
    {
        return path.len() >= 2 && path[0].is_ascii_alphabetic() && path[1] == b':';
    }

    #[cfg(not(windows))]
    {
        false
    }
}

#[must_use]
pub fn json_escaped_len(source: Option<&[u8]>) -> c_int {
    let Some(source) = source else {
        return 0;
    };
    let mut length = 0;
    for &byte in source {
        let Some(next) = json_escaped_len_add(length, byte) else {
            return -1;
        };
        length = next;
    }
    length
}

const JSON_ESCAPED_LEN_MAX: c_int = c_int::MAX - 1;

fn json_escaped_len_add(length: c_int, byte: u8) -> Option<c_int> {
    let width = if matches!(byte, b'"' | b'\\' | b'\n' | b'\r' | b'\t') {
        2
    } else if byte < 0x20 {
        6
    } else {
        1
    };
    length
        .checked_add(width)
        .filter(|next| *next <= JSON_ESCAPED_LEN_MAX)
}

pub fn trim_newlines(value: &mut [u8]) {
    let mut length = value.len();
    while length > 0 && (value[length - 1] == b'\n' || value[length - 1] == b'\r') {
        length -= 1;
    }
    value[length..].fill(0);
}

#[cfg(test)]
mod tests {
    use super::{json_escaped_len, json_escaped_len_add, path_is_absolute, trim_newlines};

    #[test]
    fn classifies_absolute_paths() {
        assert!(path_is_absolute(Some(b"/")));
        assert!(path_is_absolute(Some(b"/tmp/project")));
        assert!(path_is_absolute(Some(b"//server")));
        assert!(!path_is_absolute(Some(b"")));
        assert!(!path_is_absolute(Some(b"relative/project")));
        assert!(!path_is_absolute(Some(b"\\root")));
        assert!(!path_is_absolute(None));
        #[cfg(not(windows))]
        {
            assert!(!path_is_absolute(Some(b"C:")));
            assert!(!path_is_absolute(Some(b"C:project")));
            assert!(!path_is_absolute(Some(b"C:\\project")));
        }
    }

    #[test]
    fn counts_json_escape_lengths() {
        assert_eq!(json_escaped_len(Some(b"plain")), 5);
        assert_eq!(json_escaped_len(Some(b"quote\\slash")), 12);
        assert_eq!(json_escaped_len(Some(b"\"")), 2);
        assert_eq!(json_escaped_len(Some(b"\\")), 2);
        assert_eq!(json_escaped_len(Some(b"line\n")), 6);
        assert_eq!(json_escaped_len(Some(b"\r\t")), 4);
        assert_eq!(json_escaped_len(Some(b"A\x01B\n")), 10);
        assert_eq!(json_escaped_len(Some(b"")), 0);
        assert_eq!(json_escaped_len(None), 0);
    }

    #[test]
    fn json_escape_length_rejects_int_buffer_overflow() {
        let max = std::os::raw::c_int::MAX - 1;
        assert_eq!(json_escaped_len_add(max - 1, b'a'), Some(max));
        assert_eq!(json_escaped_len_add(max, b'a'), None);
        assert_eq!(json_escaped_len_add(max - 6, b'\x01'), Some(max));
        assert_eq!(json_escaped_len_add(max - 5, b'\x01'), None);
    }

    #[test]
    fn trims_terminal_newlines() {
        let mut value = b"output\r\n".to_vec();
        trim_newlines(&mut value);
        assert_eq!(value, b"output\0\0");

        let mut unchanged = b"output".to_vec();
        trim_newlines(&mut unchanged);
        assert_eq!(unchanged, b"output");
    }

    #[cfg(windows)]
    #[test]
    fn classifies_windows_drive_paths() {
        assert!(path_is_absolute(Some(b"C:")));
        assert!(path_is_absolute(Some(b"C:project")));
        assert!(path_is_absolute(Some(b"z:/project")));
        assert!(path_is_absolute(Some(b"z:\\project")));
        assert!(!path_is_absolute(Some(b"1:/project")));
        assert!(!path_is_absolute(Some(b"\xc4:project")));
        assert!(!path_is_absolute(Some(b"\\\\server\\share")));
    }
}
