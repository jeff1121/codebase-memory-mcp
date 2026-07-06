//! SQLite pragma contract helpers for Store migration tests.

const MMAP_DEFAULT: i64 = 67_108_864;
const I64_MAX_MAGNITUDE: u128 = i64::MAX as u128;
const I64_MIN_MAGNITUDE: u128 = I64_MAX_MAGNITUDE + 1;

fn is_ascii_space(byte: u8) -> bool {
    matches!(byte, b' ' | b'\t' | b'\n' | b'\r' | 0x0b | 0x0c)
}

/// Resolve the value used for `PRAGMA mmap_size`.
///
/// This mirrors the C `strtoll` contract used by `cbm_store_resolve_mmap_size`:
/// missing, empty, malformed, partially numeric, or overflowing values fall
/// back to the 64 MiB default; valid negative values clamp to zero.
pub fn resolve_mmap_size_value(value: Option<&[u8]>) -> i64 {
    let Some(bytes) = value else {
        return MMAP_DEFAULT;
    };

    let mut idx = 0;
    while idx < bytes.len() && is_ascii_space(bytes[idx]) {
        idx += 1;
    }
    if idx == bytes.len() {
        return MMAP_DEFAULT;
    }

    let negative = match bytes[idx] {
        b'-' => {
            idx += 1;
            true
        }
        b'+' => {
            idx += 1;
            false
        }
        _ => false,
    };
    if idx == bytes.len() || !bytes[idx].is_ascii_digit() {
        return MMAP_DEFAULT;
    }

    let mut magnitude = 0_u128;
    while idx < bytes.len() && bytes[idx].is_ascii_digit() {
        magnitude = magnitude * 10 + u128::from(bytes[idx] - b'0');
        let limit = if negative {
            I64_MIN_MAGNITUDE
        } else {
            I64_MAX_MAGNITUDE
        };
        if magnitude > limit {
            return MMAP_DEFAULT;
        }
        idx += 1;
    }

    if idx != bytes.len() {
        return MMAP_DEFAULT;
    }
    if negative {
        return 0;
    }
    magnitude as i64
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn resolves_mmap_size_like_c_store_contract() {
        assert_eq!(resolve_mmap_size_value(None), MMAP_DEFAULT);
        assert_eq!(resolve_mmap_size_value(Some(b"")), MMAP_DEFAULT);
        assert_eq!(resolve_mmap_size_value(Some(b"not-a-number")), MMAP_DEFAULT);
        assert_eq!(resolve_mmap_size_value(Some(b"123abc")), MMAP_DEFAULT);
        assert_eq!(resolve_mmap_size_value(Some(b"4096 ")), MMAP_DEFAULT);
        assert_eq!(
            resolve_mmap_size_value(Some(b"9223372036854775808")),
            MMAP_DEFAULT
        );
        assert_eq!(resolve_mmap_size_value(Some(b"0")), 0);
        assert_eq!(resolve_mmap_size_value(Some(b"-1")), 0);
        assert_eq!(resolve_mmap_size_value(Some(b" \t+4096")), 4096);
        assert_eq!(
            resolve_mmap_size_value(Some(b"9223372036854775807")),
            i64::MAX
        );
        assert_eq!(resolve_mmap_size_value(Some(b"-9223372036854775808")), 0);
    }
}
