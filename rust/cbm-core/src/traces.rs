//! OTLP traces 純 helper 的 Rust parity 實作。
//!
//! 這個模組只處理借用的位元組切片與值型資料。C 指標轉換、呼叫端 buffer 寫入與
//! 靜態 URL buffer 都集中在 `ffi.rs` 的 ABI 邊界；實際 ingest handler 與
//! JSON-RPC 行為仍由 C 端控制。

pub const URL_BUF_CAP: usize = 1024;

const I64_MAX_MAGNITUDE: u128 = i64::MAX as u128;
const I64_MIN_MAGNITUDE: u128 = I64_MAX_MAGNITUDE + 1;

#[derive(Clone, Copy)]
pub struct TraceAttribute<'a> {
    pub key: &'a [u8],
    pub string_value: Option<&'a [u8]>,
}

#[derive(Clone, Copy)]
pub enum HttpPath<'a> {
    Attribute(&'a [u8]),
    Url(&'a [u8]),
}

impl<'a> HttpPath<'a> {
    fn bytes(self) -> &'a [u8] {
        match self {
            Self::Attribute(bytes) | Self::Url(bytes) => bytes,
        }
    }
}

#[derive(Clone, Copy)]
pub struct HttpSpanInfo<'a> {
    pub method: Option<&'a [u8]>,
    pub path: Option<HttpPath<'a>>,
    pub status_code: Option<&'a [u8]>,
}

fn is_ascii_space(byte: u8) -> bool {
    matches!(byte, b' ' | b'\t' | b'\n' | b'\r' | 0x0b | 0x0c)
}

fn parse_i64_prefix(bytes: &[u8]) -> Option<i64> {
    let mut idx = 0;
    while idx < bytes.len() && is_ascii_space(bytes[idx]) {
        idx += 1;
    }
    if idx == bytes.len() {
        return None;
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
        return None;
    }

    let mut magnitude = 0_u128;
    let limit = if negative {
        I64_MIN_MAGNITUDE
    } else {
        I64_MAX_MAGNITUDE
    };
    while idx < bytes.len() && bytes[idx].is_ascii_digit() {
        magnitude = match magnitude
            .checked_mul(10)
            .and_then(|value| value.checked_add(u128::from(bytes[idx] - b'0')))
        {
            Some(value) => value,
            None => return Some(if negative { i64::MIN } else { i64::MAX }),
        };
        if magnitude > limit {
            return Some(if negative { i64::MIN } else { i64::MAX });
        }
        idx += 1;
    }

    if negative {
        if magnitude == I64_MIN_MAGNITUDE {
            Some(i64::MIN)
        } else {
            Some(-(magnitude as i64))
        }
    } else {
        Some(magnitude as i64)
    }
}

pub fn extract_service_name<'a>(
    attributes: impl IntoIterator<Item = TraceAttribute<'a>>,
) -> Option<&'a [u8]> {
    for attr in attributes {
        if attr.key == b"service.name" {
            return attr.string_value;
        }
    }
    None
}

pub fn path_from_url(url: &[u8]) -> Option<&[u8]> {
    let mut slashes = 0;
    let mut start = None;
    for (idx, byte) in url.iter().enumerate() {
        if *byte == b'/' {
            slashes += 1;
            if slashes == 3 {
                start = Some(idx);
                break;
            }
        }
    }

    let start = start?;
    let path = &url[start..];
    let end = path
        .iter()
        .position(|byte| *byte == b'?')
        .unwrap_or(path.len());
    Some(&path[..end])
}

pub fn write_path(path: &[u8], output: &mut [u8]) {
    if output.is_empty() {
        return;
    }

    let copied = path.len().min(output.len() - 1);
    output[..copied].copy_from_slice(&path[..copied]);
    output[copied] = 0;
}

pub fn write_path_from_url(url: Option<&[u8]>, output: &mut [u8]) {
    write_path(url.and_then(path_from_url).unwrap_or(&[]), output);
}

pub fn extract_http_info<'a>(
    attributes: impl IntoIterator<Item = TraceAttribute<'a>>,
) -> HttpSpanInfo<'a> {
    let mut info = HttpSpanInfo {
        method: None,
        path: None,
        status_code: None,
    };

    for attr in attributes {
        let Some(value) = attr.string_value else {
            continue;
        };
        if attr.key == b"http.method" || attr.key == b"http.request.method" {
            info.method = Some(value);
        } else if attr.key == b"http.route" || attr.key == b"http.target" || attr.key == b"url.path"
        {
            info.path = Some(HttpPath::Attribute(value));
        } else if attr.key == b"http.status_code" {
            info.status_code = Some(value);
        } else if attr.key == b"url.full" {
            if let Some(path) = path_from_url(value).filter(|path| !path.is_empty()) {
                info.path = Some(HttpPath::Url(path));
            }
        }
    }

    info
}

pub fn http_info_has_path(info: HttpSpanInfo<'_>) -> bool {
    info.path.is_some_and(|path| !path.bytes().is_empty())
}

pub fn parse_duration(start_nano: Option<&[u8]>, end_nano: Option<&[u8]>) -> i64 {
    let (Some(start_nano), Some(end_nano)) = (start_nano, end_nano) else {
        return 0;
    };
    let start = parse_i64_prefix(start_nano).unwrap_or(0);
    let end = parse_i64_prefix(end_nano).unwrap_or(0);
    if end > start {
        end - start
    } else {
        0
    }
}

pub fn calculate_p99(values: &mut [i64]) -> i64 {
    if values.is_empty() {
        return 0;
    }
    values.sort_unstable();
    let mut idx = ((values.len() as f64) * 0.99) as usize;
    if idx >= values.len() {
        idx = values.len().saturating_sub(1);
    }
    values[idx]
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn extracts_service_name_like_c_helper() {
        let attrs = [
            TraceAttribute {
                key: b"other",
                string_value: Some(b"x"),
            },
            TraceAttribute {
                key: b"service.name",
                string_value: Some(b"order-service"),
            },
        ];

        assert_eq!(
            extract_service_name(attrs.iter().copied()),
            Some(&b"order-service"[..])
        );
    }

    #[test]
    fn preserves_first_matching_service_name_with_missing_value() {
        let attrs = [
            TraceAttribute {
                key: b"service.name",
                string_value: None,
            },
            TraceAttribute {
                key: b"service.name",
                string_value: Some(b"later-value"),
            },
        ];

        assert_eq!(extract_service_name(attrs.iter().copied()), None);
    }

    #[test]
    fn extracts_http_info_and_path_like_c_helper() {
        let attrs = [
            TraceAttribute {
                key: b"http.method",
                string_value: Some(b"GET"),
            },
            TraceAttribute {
                key: b"url.full",
                string_value: Some(b"https://example.com/api/orders?q=1"),
            },
            TraceAttribute {
                key: b"http.status_code",
                string_value: Some(b"200"),
            },
        ];

        let info = extract_http_info(attrs.iter().copied());
        assert_eq!(info.method, Some(&b"GET"[..]));
        assert_eq!(info.status_code, Some(&b"200"[..]));
        assert!(http_info_has_path(info));
        assert!(matches!(info.path, Some(HttpPath::Url(b"/api/orders"))));
    }

    #[test]
    fn writes_path_with_nul_termination_and_truncation() {
        let mut output = [0_u8; 4];
        write_path_from_url(Some(b"https://example.com/api/orders"), &mut output);
        assert_eq!(output, *b"/ap\0");

        write_path_from_url(None, &mut output);
        assert_eq!(output[0], 0);
    }

    #[test]
    fn extracts_path_and_duration_like_c_helper() {
        assert_eq!(
            path_from_url(b"http://localhost:8080/health?check=true"),
            Some(&b"/health"[..])
        );
        assert_eq!(parse_duration(Some(b"1000"), Some(b"500")), 0);
    }

    #[test]
    fn calculates_p99_like_c_helper() {
        let mut values = [10_i64, 20, 30, 40, 50];
        assert_eq!(calculate_p99(&mut values), 50);
    }
}
