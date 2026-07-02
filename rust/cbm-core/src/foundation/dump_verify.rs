//! 對齊 `src/foundation/dump_verify.c` 的 dump 後合理性 gate。

pub const MIN_FLOOR: i32 = 50;
pub const DEFAULT_RATIO: f64 = 0.5;

/// 當 persisted nodes 明顯低於 committed nodes 時回傳 true。
pub fn is_degraded(committed_nodes: i32, persisted_nodes: i32, ratio: f64, min_floor: i32) -> bool {
    if ratio <= 0.0 {
        return false;
    }
    if committed_nodes < 0 {
        return false;
    }
    if committed_nodes <= min_floor {
        return false;
    }
    if persisted_nodes < 0 {
        return true;
    }
    f64::from(persisted_nodes) < f64::from(committed_nodes) * ratio
}

/// 解析 `CBM_DUMP_VERIFY_MIN_RATIO` 形式的值。
///
/// invalid、empty 或 out-of-range 值會回到 C default。與 C `strtod` 一樣，
/// 即使尾端還有文字，只要前綴是合法數字就會接受。`0` 是合法值，代表停用 gate。
pub fn parse_min_ratio(value: Option<&str>) -> f64 {
    let Some(value) = value else {
        return DEFAULT_RATIO;
    };
    let Some(prefix) = numeric_prefix(value) else {
        return DEFAULT_RATIO;
    };
    let Ok(parsed) = prefix.parse::<f64>() else {
        return DEFAULT_RATIO;
    };
    if (0.0..=1.0).contains(&parsed) {
        parsed
    } else {
        DEFAULT_RATIO
    }
}

fn numeric_prefix(value: &str) -> Option<&str> {
    let trimmed = value.trim_start();
    let bytes = trimmed.as_bytes();
    let mut idx = 0;

    if matches!(bytes.get(idx), Some(b'+' | b'-')) {
        idx += 1;
    }

    let digits_start = idx;
    while matches!(bytes.get(idx), Some(b'0'..=b'9')) {
        idx += 1;
    }
    let digits_before_dot = idx > digits_start;

    if matches!(bytes.get(idx), Some(b'.')) {
        idx += 1;
        let frac_start = idx;
        while matches!(bytes.get(idx), Some(b'0'..=b'9')) {
            idx += 1;
        }
        if !digits_before_dot && idx == frac_start {
            return None;
        }
    } else if !digits_before_dot {
        return None;
    }

    if matches!(bytes.get(idx), Some(b'e' | b'E')) {
        let exponent_marker = idx;
        idx += 1;
        if matches!(bytes.get(idx), Some(b'+' | b'-')) {
            idx += 1;
        }
        let exponent_start = idx;
        while matches!(bytes.get(idx), Some(b'0'..=b'9')) {
            idx += 1;
        }
        if idx == exponent_start {
            idx = exponent_marker;
        }
    }

    Some(&trimmed[..idx])
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn pure_gate_matches_c_matrix() {
        assert!(!is_degraded(-1, 500, 0.5, MIN_FLOOR));
        assert!(!is_degraded(50, 10, 0.5, MIN_FLOOR));
        assert!(!is_degraded(12, 5, 0.5, MIN_FLOOR));
        assert!(is_degraded(1000, 400, 0.5, MIN_FLOOR));
        assert!(!is_degraded(1000, 500, 0.5, MIN_FLOOR));
        assert!(is_degraded(1000, 499, 0.5, MIN_FLOOR));
        assert!(is_degraded(1000, 0, 0.5, MIN_FLOOR));
        assert!(!is_degraded(500, 750, 0.5, MIN_FLOOR));
        assert!(is_degraded(1000, -1, 0.5, MIN_FLOOR));
        assert!(!is_degraded(1000, 10, 0.0, MIN_FLOOR));
        assert!(!is_degraded(1000, 600, 0.5, MIN_FLOOR));
        assert!(is_degraded(1000, 900, 0.95, MIN_FLOOR));
        assert!(!is_degraded(200, 200, 0.5, MIN_FLOOR));
    }

    #[test]
    fn boundary_values_match_c_policy() {
        assert!(!is_degraded(51, 26, 0.5, MIN_FLOOR));
        assert!(is_degraded(51, 25, 0.5, MIN_FLOOR));
        assert!(!is_degraded(100, 0, -0.1, MIN_FLOOR));
        assert!(!is_degraded(100, 0, f64::NEG_INFINITY, MIN_FLOOR));
        assert!(!is_degraded(100, 0, f64::NAN, MIN_FLOOR));
    }

    #[test]
    fn parse_min_ratio_matches_env_policy() {
        assert_eq!(parse_min_ratio(None), DEFAULT_RATIO);
        assert_eq!(parse_min_ratio(Some("0")), 0.0);
        assert_eq!(parse_min_ratio(Some("0.25")), 0.25);
        assert_eq!(parse_min_ratio(Some(".25")), 0.25);
        assert_eq!(parse_min_ratio(Some(" +0.25")), 0.25);
        assert_eq!(parse_min_ratio(Some("1")), 1.0);
        assert_eq!(parse_min_ratio(Some("0.5x")), 0.5);
        assert_eq!(parse_min_ratio(Some("0.5e")), 0.5);
        assert_eq!(parse_min_ratio(Some("5e-1")), 0.5);

        for invalid in ["", "abc", "-0.1", "1.1", "2x", "x0.5"] {
            assert_eq!(parse_min_ratio(Some(invalid)), DEFAULT_RATIO, "{invalid:?}");
        }
    }
}
