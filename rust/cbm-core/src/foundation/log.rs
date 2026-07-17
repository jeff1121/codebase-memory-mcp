//! 對齊 `src/foundation/log.c` 的 deterministic helper contract。
//!
//! 這裡只建模可純函式驗證的 env parsing、格式化與 HTTP level 決策；
//! stderr/sink lifecycle 仍留在 C 產品路徑。

pub const LEVEL_DEBUG: i32 = 0;
pub const LEVEL_INFO: i32 = 1;
pub const LEVEL_WARN: i32 = 2;
pub const LEVEL_ERROR: i32 = 3;
pub const LEVEL_NONE: i32 = 4;

pub const FORMAT_TEXT: i32 = 0;
pub const FORMAT_JSON: i32 = 1;

#[must_use]
pub fn level_name(level: i32) -> &'static [u8] {
    match level {
        LEVEL_DEBUG => b"debug",
        LEVEL_INFO => b"info",
        LEVEL_WARN => b"warn",
        LEVEL_ERROR => b"error",
        _ => b"unknown",
    }
}

#[must_use]
pub fn parse_level_value(raw: Option<&[u8]>, current: i32) -> i32 {
    let Some(raw) = raw else {
        return current;
    };
    if raw.is_empty() {
        return current;
    }

    if raw.len() < 8 {
        let lower = ascii_lower(raw);
        match lower.as_slice() {
            b"debug" => return LEVEL_DEBUG,
            b"info" => return LEVEL_INFO,
            b"warn" => return LEVEL_WARN,
            b"error" => return LEVEL_ERROR,
            b"none" => return LEVEL_NONE,
            _ => {}
        }
    }

    parse_c_long(raw).map_or(current, |n| {
        if (LEVEL_DEBUG..=LEVEL_NONE).contains(&n) {
            n
        } else {
            current
        }
    })
}

#[must_use]
pub fn parse_format_value(raw: Option<&[u8]>, current: i32) -> i32 {
    let Some(raw) = raw else {
        return current;
    };
    if raw.is_empty() || raw.len() >= 8 {
        return current;
    }
    match ascii_lower(raw).as_slice() {
        b"json" => FORMAT_JSON,
        b"text" => FORMAT_TEXT,
        _ => current,
    }
}

#[must_use]
pub fn sanitize_text_atom(value: Option<&[u8]>) -> Vec<u8> {
    let Some(value) = value else {
        return Vec::new();
    };
    value
        .iter()
        .map(|&ch| if ch <= b' ' || ch == 0x7f { b'_' } else { ch })
        .collect()
}

#[must_use]
pub fn json_string(value: Option<&[u8]>) -> Vec<u8> {
    let mut out = Vec::new();
    out.push(b'"');
    if let Some(value) = value {
        for &ch in value {
            match ch {
                b'"' => out.extend_from_slice(br#"\""#),
                b'\\' => out.extend_from_slice(br#"\\"#),
                b'\x08' => out.extend_from_slice(br#"\b"#),
                b'\x0c' => out.extend_from_slice(br#"\f"#),
                b'\n' => out.extend_from_slice(br#"\n"#),
                b'\r' => out.extend_from_slice(br#"\r"#),
                b'\t' => out.extend_from_slice(br#"\t"#),
                0x00..=0x1f => {
                    const HEX: &[u8; 16] = b"0123456789abcdef";
                    out.extend_from_slice(b"\\u00");
                    out.push(HEX[(ch >> 4) as usize]);
                    out.push(HEX[(ch & 0x0f) as usize]);
                }
                _ => out.push(ch),
            }
        }
    }
    out.push(b'"');
    out
}

#[must_use]
pub fn http_path_without_query(path: Option<&[u8]>) -> Vec<u8> {
    let Some(path) = path else {
        return Vec::new();
    };
    let end = path
        .iter()
        .position(|&ch| ch == b'?' || ch == b'#')
        .unwrap_or(path.len());
    path[..end].to_vec()
}

#[must_use]
pub fn http_status_level(status: i32) -> i32 {
    if status >= 500 {
        LEVEL_ERROR
    } else if status >= 400 {
        LEVEL_WARN
    } else {
        LEVEL_INFO
    }
}

fn ascii_lower(value: &[u8]) -> Vec<u8> {
    value.iter().map(u8::to_ascii_lowercase).collect()
}

fn parse_c_long(value: &[u8]) -> Option<i32> {
    let s = std::str::from_utf8(value).ok()?;
    let trimmed_start = s.trim_start_matches(|ch: char| ch.is_ascii_whitespace());
    if trimmed_start.is_empty() {
        return None;
    }
    let parsed = trimmed_start.parse::<i64>().ok()?;
    if parsed < i64::from(i32::MIN) || parsed > i64::from(i32::MAX) {
        return None;
    }
    Some(parsed as i32)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_level_and_format_like_c_env() {
        assert_eq!(parse_level_value(Some(b"WARN"), LEVEL_INFO), LEVEL_WARN);
        assert_eq!(parse_level_value(Some(b"3"), LEVEL_INFO), LEVEL_ERROR);
        assert_eq!(parse_level_value(Some(b" +3"), LEVEL_INFO), LEVEL_ERROR);
        assert_eq!(parse_level_value(Some(b"3 "), LEVEL_WARN), LEVEL_WARN);
        assert_eq!(parse_level_value(Some(b"-1"), LEVEL_WARN), LEVEL_WARN);
        assert_eq!(
            parse_level_value(Some(b"999999999999999999999"), LEVEL_WARN),
            LEVEL_WARN
        );
        assert_eq!(parse_level_value(Some(b"5"), LEVEL_INFO), LEVEL_INFO);
        assert_eq!(parse_level_value(Some(b""), LEVEL_WARN), LEVEL_WARN);
        assert_eq!(parse_format_value(Some(b"json"), FORMAT_TEXT), FORMAT_JSON);
        assert_eq!(parse_format_value(Some(b"text"), FORMAT_JSON), FORMAT_TEXT);
        assert_eq!(parse_format_value(Some(b" json"), FORMAT_JSON), FORMAT_JSON);
        assert_eq!(parse_format_value(None, FORMAT_JSON), FORMAT_JSON);
    }

    #[test]
    fn formats_text_json_and_http_helpers() {
        assert_eq!(
            sanitize_text_atom(Some(b"line\r\nbreak\tvalue")),
            b"line__break_value"
        );
        assert_eq!(
            json_string(Some(b"line\n\"quote\"\\tail")),
            br#""line\n\"quote\"\\tail""#
        );
        assert_eq!(
            http_path_without_query(Some(b"/api/layout?token=secret#frag")),
            b"/api/layout"
        );
        assert_eq!(http_status_level(200), LEVEL_INFO);
        assert_eq!(http_status_level(404), LEVEL_WARN);
        assert_eq!(http_status_level(500), LEVEL_ERROR);
    }
}
