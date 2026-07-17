const MIN_TOKEN_LEN: usize = 4;
const MAX_TOKEN_LEN: usize = 96;

#[must_use]
pub fn extract_token(pattern: Option<&[u8]>, out_size: usize) -> Option<Vec<u8>> {
    let pattern = pattern?;
    let mut best_start = 0;
    let mut best_len = 0;
    let mut index = 0;

    while index < pattern.len() {
        let byte = pattern[index];
        if byte.is_ascii_alphabetic() || byte == b'_' {
            let start = index;
            index += 1;
            while index < pattern.len()
                && (pattern[index].is_ascii_alphanumeric() || pattern[index] == b'_')
            {
                index += 1;
            }
            let length = index - start;
            if length > best_len {
                best_start = start;
                best_len = length;
            }
        } else {
            index += 1;
        }
    }

    if best_len < MIN_TOKEN_LEN || out_size == 0 {
        return None;
    }
    best_len = best_len.min(MAX_TOKEN_LEN).min(out_size - 1);
    Some(pattern[best_start..best_start + best_len].to_vec())
}

#[cfg(test)]
mod tests {
    use super::extract_token;

    #[test]
    fn extracts_longest_identifier_token() {
        assert_eq!(
            extract_token(Some(b"**/OrderHandler.py"), 97),
            Some(b"OrderHandler".to_vec())
        );
        assert_eq!(
            extract_token(Some(b"x short longer_token"), 97),
            Some(b"longer_token".to_vec())
        );
        assert_eq!(extract_token(Some(b"foo/bar"), 97), None);
        assert_eq!(
            extract_token(Some(b"prefix_abcdefghijklmnopqrstuvwxyz"), 8),
            Some(b"prefix_".to_vec())
        );
        assert_eq!(extract_token(None, 97), None);
        assert_eq!(extract_token(Some(b"valid_token"), 0), None);
    }
}
