//! 對齊 `src/ui/httpd.c` 的無 socket parser helper。
//!
//! listener、socket、request allocation 與 HTTP response 仍由 C 負責。

#[must_use]
pub fn header_name_is(line: Option<&[u8]>, name_len: usize, name: Option<&[u8]>) -> bool {
    let (Some(line), Some(name)) = (line, name) else {
        return false;
    };
    if name.len() != name_len || line.len() < name_len {
        return false;
    }
    line[..name_len]
        .iter()
        .zip(name.iter())
        .all(|(left, right)| left.to_ascii_lowercase() == *right)
}

pub fn copy_header_value(value: &[u8], output: &mut [u8]) -> bool {
    let mut start = 0;
    let mut end = value.len();
    while start < end && matches!(value[start], b' ' | b'\t') {
        start += 1;
    }
    while end > start && matches!(value[end - 1], b' ' | b'\t') {
        end -= 1;
    }
    let length = end - start;
    if length >= output.len() {
        if let Some(first) = output.first_mut() {
            *first = 0;
        }
        return false;
    }
    output[..length].copy_from_slice(&value[start..end]);
    output[length] = 0;
    true
}

#[cfg(test)]
mod tests {
    use super::{copy_header_value, header_name_is};

    #[test]
    fn header_name_comparison_is_ascii_case_insensitive() {
        assert!(header_name_is(Some(b"Origin"), 6, Some(b"origin")));
        assert!(header_name_is(Some(b"ORIGIN"), 6, Some(b"origin")));
        assert!(!header_name_is(Some(b"OriginX"), 7, Some(b"origin")));
        assert!(!header_name_is(Some(b"Origin"), 5, Some(b"origin")));
        assert!(!header_name_is(None, 6, Some(b"origin")));
    }

    #[test]
    fn header_value_copy_trims_and_rejects_oversize() {
        let mut output = [0u8; 8];
        assert!(copy_header_value(b" \tvalue\t ", &mut output));
        assert_eq!(&output[..6], b"value\0");
        let mut small = [b'X'; 4];
        assert!(!copy_header_value(b"value", &mut small));
        assert_eq!(small[0], 0);
    }
}
