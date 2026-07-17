//! `pass_complexity.c` 的 flat JSON property readers。

const PATTERN_BUF_SIZE: usize = 64;

#[must_use]
pub fn json_get_int(json: Option<&[u8]>, key: Option<&[u8]>, default: i32) -> i32 {
    let Some(value) = find_property_value(json, key) else {
        return default;
    };
    parse_decimal_prefix(value)
}

#[must_use]
pub fn json_get_bool(json: Option<&[u8]>, key: Option<&[u8]>) -> bool {
    find_property_value(json, key).is_some_and(|value| value.first() == Some(&b't'))
}

fn find_property_value<'a>(json: Option<&'a [u8]>, key: Option<&[u8]>) -> Option<&'a [u8]> {
    let json = json?;
    let key = key?;
    let mut pattern = Vec::with_capacity(key.len() + 4);
    pattern.extend_from_slice(b"\"");
    pattern.extend_from_slice(key);
    pattern.extend_from_slice(b"\":");
    pattern.truncate(PATTERN_BUF_SIZE - 1);
    let offset = json
        .windows(pattern.len())
        .position(|window| window == pattern)?;
    let mut value = &json[offset + pattern.len()..];
    while value
        .first()
        .is_some_and(|byte| matches!(*byte, b' ' | b'\t'))
    {
        value = &value[1..];
    }
    Some(value)
}

fn parse_decimal_prefix(value: &[u8]) -> i32 {
    let mut index = 0;
    let negative = value.first() == Some(&b'-');
    if negative || value.first() == Some(&b'+') {
        index += 1;
    }
    let start = index;
    let mut parsed = 0_i64;
    while let Some(&byte @ b'0'..=b'9') = value.get(index) {
        parsed = parsed
            .saturating_mul(10)
            .saturating_add(i64::from(byte - b'0'));
        index += 1;
    }
    if index == start {
        return 0;
    }
    let signed = if negative { -parsed } else { parsed };
    signed as i32
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn reads_integer_with_c_whitespace_contract() {
        let json = br#"{"loop_depth": 12, "other": 4}"#;
        assert_eq!(json_get_int(Some(json), Some(b"loop_depth"), 0), 12);
        assert_eq!(json_get_int(Some(json), Some(b"missing"), 7), 7);
        assert_eq!(json_get_int(Some(json), Some(b"other"), 0), 4);
    }

    #[test]
    fn reads_boolean_by_first_character() {
        assert!(json_get_bool(
            Some(br#"{"self_recursive":true}"#),
            Some(b"self_recursive")
        ));
        assert!(!json_get_bool(
            Some(br#"{"self_recursive":false}"#),
            Some(b"self_recursive")
        ));
        assert!(json_get_bool(
            Some(br#"{"self_recursive":trash}"#),
            Some(b"self_recursive")
        ));
    }
}
