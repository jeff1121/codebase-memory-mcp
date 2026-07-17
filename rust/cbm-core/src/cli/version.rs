//! 對齊 `src/cli/cli.c` 的版本比較 helper。

fn parse_semver(value: &[u8]) -> [i64; 3] {
    let mut pos = 0;
    if matches!(value.first(), Some(b'v' | b'V')) {
        pos = 1;
    }

    let mut parts = [0_i64; 3];
    let mut count = 0;
    while pos < value.len() && count < parts.len() {
        if value[pos] == b'-' {
            break;
        }

        while pos < value.len() && value[pos].is_ascii_whitespace() {
            pos += 1;
        }
        let mut sign = 1_i64;
        if pos < value.len() {
            match value[pos] {
                b'+' => pos += 1,
                b'-' => {
                    sign = -1;
                    pos += 1;
                }
                _ => {}
            }
        }

        let digit_start = pos;
        let mut number = 0_i64;
        while pos < value.len() && value[pos].is_ascii_digit() {
            number = number
                .saturating_mul(10)
                .saturating_add(i64::from(value[pos] - b'0'));
            pos += 1;
        }
        if pos == digit_start {
            break;
        }

        parts[count] = if sign < 0 {
            number.saturating_neg()
        } else {
            number
        };
        count += 1;
        if pos < value.len() && value[pos] == b'.' {
            pos += 1;
        } else {
            break;
        }
    }
    parts
}

fn has_prerelease(value: &[u8]) -> bool {
    let value = if matches!(value.first(), Some(b'v' | b'V')) {
        &value[1..]
    } else {
        value
    };
    value.contains(&b'-')
}

#[must_use]
pub fn compare_versions(left: Option<&[u8]>, right: Option<&[u8]>) -> i32 {
    let left = left.unwrap_or_default();
    let right = right.unwrap_or_default();
    let left_parts = parse_semver(left);
    let right_parts = parse_semver(right);

    for (left_part, right_part) in left_parts.iter().zip(right_parts.iter()) {
        if left_part != right_part {
            let difference = left_part - right_part;
            return difference.clamp(i64::from(i32::MIN), i64::from(i32::MAX)) as i32;
        }
    }

    match (has_prerelease(left), has_prerelease(right)) {
        (true, false) => -1,
        (false, true) => 1,
        _ => 0,
    }
}

#[cfg(test)]
mod tests {
    use super::compare_versions;

    #[test]
    fn compare_matches_cli_contract() {
        assert!(compare_versions(Some(b"0.2.1"), Some(b"0.2.0")) > 0);
        assert_eq!(compare_versions(Some(b"0.2.0"), Some(b"0.2.0")), 0);
        assert!(compare_versions(Some(b"0.1.9"), Some(b"0.2.0")) < 0);
        assert!(compare_versions(Some(b"0.10.0"), Some(b"0.2.0")) > 0);
        assert!(compare_versions(Some(b"1.0.0"), Some(b"0.99.99")) > 0);
        assert_eq!(compare_versions(Some(b"v0.2.1"), Some(b"0.2.1")), 0);
        assert!(compare_versions(Some(b"0.2.1-dev"), Some(b"0.2.1")) < 0);
        assert!(compare_versions(Some(b"0.2.1"), Some(b"0.2.1-dev")) > 0);
        assert_eq!(compare_versions(Some(b"0.2.1-dev"), Some(b"0.2.1-dev")), 0);
    }

    #[test]
    fn compare_handles_missing_parts_and_null_ffi_inputs() {
        assert_eq!(compare_versions(Some(b"1"), Some(b"1.0.0")), 0);
        assert!(compare_versions(Some(b"1.0.0"), Some(b"1.0.0-rc1")) > 0);
        assert_eq!(compare_versions(None, None), 0);
    }
}
