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
    parse_min_ratio_bytes(value.map(str::as_bytes)).0
}

/// 以 byte-level 行為模擬 C `strtod` 的 ratio parser。
///
/// 回傳 `(ratio, valid)`；`valid` 只表示數值已成功解析且落在 `0..=1`，
/// 讓 C wrapper 保留既有 warning 與 default fallback。輸入不要求是 UTF-8，
/// parser 只消費前綴數字，並支援 C `strtod` 接受的十六進位浮點前綴。
pub fn parse_min_ratio_bytes(value: Option<&[u8]>) -> (f64, bool) {
    let Some(value) = value else {
        return (DEFAULT_RATIO, false);
    };

    let start = value
        .iter()
        .position(|byte| !matches!(byte, b' ' | b'\t' | b'\n' | b'\r' | 0x0b | 0x0c))
        .unwrap_or(value.len());
    let value = &value[start..];
    let (negative, value) = match value.first() {
        Some(b'+') => (false, &value[1..]),
        Some(b'-') => (true, &value[1..]),
        _ => (false, value),
    };

    let parsed = parse_hex_prefix(value)
        .or_else(|| parse_decimal_prefix(value))
        .map(|(number, _)| if negative { -number } else { number });
    let Some(parsed) = parsed else {
        return (DEFAULT_RATIO, false);
    };
    if (0.0..=1.0).contains(&parsed) {
        (parsed, true)
    } else {
        (DEFAULT_RATIO, false)
    }
}

fn parse_decimal_prefix(value: &[u8]) -> Option<(f64, usize)> {
    let mut idx = 0;

    let digits_start = idx;
    while matches!(value.get(idx), Some(b'0'..=b'9')) {
        idx += 1;
    }
    let digits_before_dot = idx > digits_start;

    if matches!(value.get(idx), Some(b'.')) {
        idx += 1;
        let frac_start = idx;
        while matches!(value.get(idx), Some(b'0'..=b'9')) {
            idx += 1;
        }
        if !digits_before_dot && idx == frac_start {
            return None;
        }
    } else if !digits_before_dot {
        return None;
    }

    if matches!(value.get(idx), Some(b'e' | b'E')) {
        let exponent_marker = idx;
        idx += 1;
        if matches!(value.get(idx), Some(b'+' | b'-')) {
            idx += 1;
        }
        let exponent_start = idx;
        while matches!(value.get(idx), Some(b'0'..=b'9')) {
            idx += 1;
        }
        if idx == exponent_start {
            idx = exponent_marker;
        }
    }

    let prefix = std::str::from_utf8(&value[..idx]).ok()?;
    Some((prefix.parse::<f64>().ok()?, idx))
}

fn hex_digit(value: u8) -> Option<u8> {
    match value {
        b'0'..=b'9' => Some(value - b'0'),
        b'a'..=b'f' => Some(value - b'a' + 10),
        b'A'..=b'F' => Some(value - b'A' + 10),
        _ => None,
    }
}

fn parse_hex_prefix(value: &[u8]) -> Option<(f64, usize)> {
    if value.len() < 2 || value[0] != b'0' || !matches!(value[1], b'x' | b'X') {
        return None;
    }

    let mut idx = 2;
    let mut mantissa = 0.0;
    let mut digits = 0usize;
    while let Some(digit) = value.get(idx).and_then(|byte| hex_digit(*byte)) {
        mantissa = mantissa * 16.0 + f64::from(digit);
        digits += 1;
        idx += 1;
    }

    let mut fractional_digits = 0usize;
    if value.get(idx) == Some(&b'.') {
        idx += 1;
        while let Some(digit) = value.get(idx).and_then(|byte| hex_digit(*byte)) {
            mantissa = mantissa * 16.0 + f64::from(digit);
            fractional_digits = fractional_digits.saturating_add(1);
            digits += 1;
            idx += 1;
        }
    }
    if digits == 0 {
        return None;
    }

    let mut exponent = 0.0;
    if matches!(value.get(idx), Some(b'p' | b'P')) {
        let exponent_marker = idx;
        idx += 1;
        let negative_exponent = match value.get(idx) {
            Some(b'+') => {
                idx += 1;
                false
            }
            Some(b'-') => {
                idx += 1;
                true
            }
            _ => false,
        };
        let exponent_start = idx;
        while matches!(value.get(idx), Some(b'0'..=b'9')) {
            idx += 1;
        }
        if idx == exponent_start {
            idx = exponent_marker;
        } else {
            for digit in &value[exponent_start..idx] {
                exponent = (exponent * 10.0 + f64::from(*digit - b'0')).min(f64::MAX);
            }
            if negative_exponent {
                exponent = -exponent;
            }
        }
    }

    let scale = exponent - fractional_digits as f64 * 4.0;
    Some((mantissa * 2.0_f64.powf(scale), idx))
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
        assert_eq!(parse_min_ratio(Some("0x1p-1")), 0.5);
        assert_eq!(parse_min_ratio(Some("inf")), DEFAULT_RATIO);
        assert_eq!(parse_min_ratio(Some("nan")), DEFAULT_RATIO);

        for invalid in ["", "abc", "-0.1", "1.1", "2x", "x0.5"] {
            assert_eq!(parse_min_ratio(Some(invalid)), DEFAULT_RATIO, "{invalid:?}");
        }
    }
}
