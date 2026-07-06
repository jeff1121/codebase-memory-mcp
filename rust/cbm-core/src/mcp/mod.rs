//! MCP JSON-RPC codec 的 test-only parity helper。
//!
//! 這裡只固定 request envelope 的 byte-level summary contract；production MCP
//! dispatch、tool handlers、Content-Length transport 與公開 JSON 輸出仍由 C 負責。

#[derive(Debug, Clone, PartialEq, Eq)]
enum IdSummary {
    None,
    Int(i64),
    String(Vec<u8>),
    Other,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum ValueKind {
    Object,
    Array,
    String,
    Int,
    Number,
    Bool,
    Null,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct RequestSummary {
    jsonrpc: Vec<u8>,
    method: Vec<u8>,
    id: IdSummary,
    params: Option<ParamSummary>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct ParamSummary {
    kind: ValueKind,
    raw: Vec<u8>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct JsonRpcRequest {
    pub jsonrpc: Vec<u8>,
    pub method: Vec<u8>,
    pub id: ParsedId,
    pub params_raw: Option<Vec<u8>>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ParsedId {
    None,
    Int(i64),
    String(Vec<u8>),
    Other,
}

#[must_use]
pub fn jsonrpc_parse(input: &[u8]) -> Option<JsonRpcRequest> {
    let mut parser = Parser::new(input);
    let summary = parser.parse_root_request()?;
    parser.skip_ws();
    if !parser.is_eof() {
        return None;
    }
    Some(JsonRpcRequest {
        jsonrpc: summary.jsonrpc,
        method: summary.method,
        id: match summary.id {
            IdSummary::None => ParsedId::None,
            IdSummary::Int(value) => ParsedId::Int(value),
            IdSummary::String(value) => ParsedId::String(value),
            IdSummary::Other => ParsedId::Other,
        },
        params_raw: summary.params.map(|params| params.raw),
    })
}

#[must_use]
pub fn jsonrpc_request_summary(input: &[u8]) -> Option<Vec<u8>> {
    let mut parser = Parser::new(input);
    let summary = parser.parse_root_request()?;
    parser.skip_ws();
    if !parser.is_eof() {
        return None;
    }
    Some(render_summary(&summary))
}

fn render_summary(summary: &RequestSummary) -> Vec<u8> {
    let mut out = Vec::new();
    out.extend_from_slice(b"jsonrpc=");
    push_escaped(&mut out, &summary.jsonrpc);
    out.extend_from_slice(b";method=");
    push_escaped(&mut out, &summary.method);
    out.extend_from_slice(b";id=");
    match &summary.id {
        IdSummary::None => out.extend_from_slice(b"none"),
        IdSummary::Int(value) => {
            out.extend_from_slice(b"int:");
            out.extend_from_slice(value.to_string().as_bytes());
        }
        IdSummary::String(value) => {
            out.extend_from_slice(b"string:");
            push_escaped(&mut out, value);
        }
        IdSummary::Other => out.extend_from_slice(b"other"),
    }
    out.extend_from_slice(b";params=");
    out.extend_from_slice(match summary.params.as_ref().map(|params| params.kind) {
        None => b"none".as_slice(),
        Some(ValueKind::Object) => b"object",
        Some(ValueKind::Array) => b"array",
        Some(ValueKind::String) => b"string",
        Some(ValueKind::Int) => b"int",
        Some(ValueKind::Number) => b"number",
        Some(ValueKind::Bool) => b"bool",
        Some(ValueKind::Null) => b"null",
    });
    out
}

fn push_escaped(out: &mut Vec<u8>, value: &[u8]) {
    for &b in value {
        match b {
            b'\\' => out.extend_from_slice(b"\\\\"),
            b';' => out.extend_from_slice(b"\\;"),
            b'=' => out.extend_from_slice(b"\\="),
            b'\n' => out.extend_from_slice(b"\\n"),
            b'\r' => out.extend_from_slice(b"\\r"),
            b'\t' => out.extend_from_slice(b"\\t"),
            0x20..=0x7e => out.push(b),
            _ => {
                out.extend_from_slice(b"\\x");
                out.push(hex_digit(b >> 4));
                out.push(hex_digit(b & 0x0f));
            }
        }
    }
}

fn hex_digit(n: u8) -> u8 {
    match n {
        0..=9 => b'0' + n,
        _ => b'a' + (n - 10),
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
enum JsonValue {
    String(Vec<u8>),
    Int(i64),
    Other(ValueKind),
}

impl JsonValue {
    fn kind(&self) -> ValueKind {
        match self {
            Self::String(_) => ValueKind::String,
            Self::Int(_) => ValueKind::Int,
            Self::Other(kind) => *kind,
        }
    }
}

struct Parser<'a> {
    input: &'a [u8],
    pos: usize,
}

impl<'a> Parser<'a> {
    fn new(input: &'a [u8]) -> Self {
        Self { input, pos: 0 }
    }

    fn is_eof(&self) -> bool {
        self.pos >= self.input.len()
    }

    fn skip_ws(&mut self) {
        while let Some(b' ' | b'\n' | b'\r' | b'\t') = self.peek() {
            self.pos += 1;
        }
    }

    fn peek(&self) -> Option<u8> {
        self.input.get(self.pos).copied()
    }

    fn consume(&mut self, expected: u8) -> bool {
        if self.peek() == Some(expected) {
            self.pos += 1;
            true
        } else {
            false
        }
    }

    fn parse_root_request(&mut self) -> Option<RequestSummary> {
        self.skip_ws();
        if !self.consume(b'{') {
            return None;
        }

        let mut jsonrpc = b"2.0".to_vec();
        let mut method = None;
        let mut id = IdSummary::None;
        let mut params = None;
        let mut seen_jsonrpc = false;
        let mut seen_method = false;
        let mut seen_id = false;
        let mut seen_params = false;

        self.skip_ws();
        if self.consume(b'}') {
            return None;
        }

        loop {
            self.skip_ws();
            let key = self.parse_string()?;
            self.skip_ws();
            if !self.consume(b':') {
                return None;
            }
            self.skip_ws();
            let value_start = self.pos;
            let value = self.parse_value()?;
            let value_end = self.pos;
            match key.as_slice() {
                b"jsonrpc" if !seen_jsonrpc => {
                    seen_jsonrpc = true;
                    if let JsonValue::String(value) = value {
                        jsonrpc = value;
                    }
                }
                b"method" if !seen_method => {
                    seen_method = true;
                    if let JsonValue::String(value) = value {
                        method = Some(value);
                    }
                }
                b"id" if !seen_id => {
                    seen_id = true;
                    id = match value {
                        JsonValue::Int(value) => IdSummary::Int(value),
                        JsonValue::String(value) => IdSummary::String(value),
                        JsonValue::Other(_) => IdSummary::Other,
                    };
                }
                b"params" if !seen_params => {
                    seen_params = true;
                    params = Some(ParamSummary {
                        kind: value.kind(),
                        raw: self.input[value_start..value_end].to_vec(),
                    });
                }
                _ => {}
            }

            self.skip_ws();
            if self.consume(b'}') {
                break;
            }
            if !self.consume(b',') {
                return None;
            }
        }

        Some(RequestSummary {
            jsonrpc,
            method: method?,
            id,
            params,
        })
    }

    fn parse_value(&mut self) -> Option<JsonValue> {
        self.skip_ws();
        match self.peek()? {
            b'"' => self.parse_string().map(JsonValue::String),
            b'{' => {
                self.parse_object_skip()?;
                Some(JsonValue::Other(ValueKind::Object))
            }
            b'[' => {
                self.parse_array_skip()?;
                Some(JsonValue::Other(ValueKind::Array))
            }
            b't' => {
                self.expect_bytes(b"true")?;
                Some(JsonValue::Other(ValueKind::Bool))
            }
            b'f' => {
                self.expect_bytes(b"false")?;
                Some(JsonValue::Other(ValueKind::Bool))
            }
            b'n' => {
                self.expect_bytes(b"null")?;
                Some(JsonValue::Other(ValueKind::Null))
            }
            b'-' | b'0'..=b'9' => self.parse_number(),
            _ => None,
        }
    }

    fn parse_object_skip(&mut self) -> Option<()> {
        if !self.consume(b'{') {
            return None;
        }
        self.skip_ws();
        if self.consume(b'}') {
            return Some(());
        }
        loop {
            self.skip_ws();
            self.parse_string()?;
            self.skip_ws();
            if !self.consume(b':') {
                return None;
            }
            self.parse_value()?;
            self.skip_ws();
            if self.consume(b'}') {
                return Some(());
            }
            if !self.consume(b',') {
                return None;
            }
        }
    }

    fn parse_array_skip(&mut self) -> Option<()> {
        if !self.consume(b'[') {
            return None;
        }
        self.skip_ws();
        if self.consume(b']') {
            return Some(());
        }
        loop {
            self.parse_value()?;
            self.skip_ws();
            if self.consume(b']') {
                return Some(());
            }
            if !self.consume(b',') {
                return None;
            }
        }
    }

    fn parse_number(&mut self) -> Option<JsonValue> {
        let start = self.pos;
        if self.consume(b'-') && !matches!(self.peek(), Some(b'0'..=b'9')) {
            return None;
        }
        match self.peek()? {
            b'0' => self.pos += 1,
            b'1'..=b'9' => {
                while matches!(self.peek(), Some(b'0'..=b'9')) {
                    self.pos += 1;
                }
            }
            _ => return None,
        }

        let mut integer_only = true;
        if self.consume(b'.') {
            integer_only = false;
            if !matches!(self.peek(), Some(b'0'..=b'9')) {
                return None;
            }
            while matches!(self.peek(), Some(b'0'..=b'9')) {
                self.pos += 1;
            }
        }
        if matches!(self.peek(), Some(b'e' | b'E')) {
            integer_only = false;
            self.pos += 1;
            if matches!(self.peek(), Some(b'+' | b'-')) {
                self.pos += 1;
            }
            if !matches!(self.peek(), Some(b'0'..=b'9')) {
                return None;
            }
            while matches!(self.peek(), Some(b'0'..=b'9')) {
                self.pos += 1;
            }
        }

        if integer_only {
            let text = std::str::from_utf8(&self.input[start..self.pos]).ok()?;
            if let Ok(value) = text.parse::<i64>() {
                return Some(JsonValue::Int(value));
            }
        }
        Some(JsonValue::Other(ValueKind::Number))
    }

    fn parse_string(&mut self) -> Option<Vec<u8>> {
        if !self.consume(b'"') {
            return None;
        }
        let mut out = Vec::new();
        while let Some(b) = self.peek() {
            self.pos += 1;
            match b {
                b'"' => {
                    std::str::from_utf8(&out).ok()?;
                    return Some(out);
                }
                b'\\' => self.parse_escape_into(&mut out)?,
                0x00..=0x1f => return None,
                _ => out.push(b),
            }
        }
        None
    }

    fn parse_escape_into(&mut self, out: &mut Vec<u8>) -> Option<()> {
        let escaped = self.peek()?;
        self.pos += 1;
        match escaped {
            b'"' | b'\\' | b'/' => out.push(escaped),
            b'b' => out.push(0x08),
            b'f' => out.push(0x0c),
            b'n' => out.push(b'\n'),
            b'r' => out.push(b'\r'),
            b't' => out.push(b'\t'),
            b'u' => {
                let scalar = self.parse_unicode_scalar()?;
                let mut buf = [0; 4];
                out.extend_from_slice(scalar.encode_utf8(&mut buf).as_bytes());
            }
            _ => return None,
        };
        Some(())
    }

    fn parse_unicode_escape_value(&mut self) -> Option<u32> {
        let mut value = 0u32;
        for _ in 0..4 {
            value = (value << 4) | u32::from(hex_value(self.peek()?)?);
            self.pos += 1;
        }
        Some(value)
    }

    fn parse_unicode_scalar(&mut self) -> Option<char> {
        let value = self.parse_unicode_escape_value()?;
        let scalar = if (0xd800..=0xdbff).contains(&value) {
            if !self.consume(b'\\') || !self.consume(b'u') {
                return None;
            }
            let low = self.parse_unicode_escape_value()?;
            if !(0xdc00..=0xdfff).contains(&low) {
                return None;
            }
            0x10000 + (((value - 0xd800) << 10) | (low - 0xdc00))
        } else if (0xdc00..=0xdfff).contains(&value) {
            return None;
        } else {
            value
        };
        char::from_u32(scalar)
    }

    fn expect_bytes(&mut self, expected: &[u8]) -> Option<()> {
        if self.input.get(self.pos..self.pos + expected.len()) == Some(expected) {
            self.pos += expected.len();
            Some(())
        } else {
            None
        }
    }
}

fn hex_value(b: u8) -> Option<u8> {
    match b {
        b'0'..=b'9' => Some(b - b'0'),
        b'a'..=b'f' => Some(b - b'a' + 10),
        b'A'..=b'F' => Some(b - b'A' + 10),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::jsonrpc_request_summary;

    fn summary(input: &[u8]) -> String {
        String::from_utf8(jsonrpc_request_summary(input).unwrap()).unwrap()
    }

    #[test]
    fn summarizes_request_notification_and_params() {
        assert_eq!(
            summary(
                br#"{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{}}}"#
            ),
            "jsonrpc=2.0;method=initialize;id=int:1;params=object"
        );
        assert_eq!(
            summary(br#"{"jsonrpc":"2.0","method":"notifications/initialized"}"#),
            "jsonrpc=2.0;method=notifications/initialized;id=none;params=none"
        );
        assert_eq!(
            summary(br#"{"jsonrpc":"2.0","id":42,"method":"tools/call","params":{"name":"search_graph","arguments":{"limit":5}}}"#),
            "jsonrpc=2.0;method=tools/call;id=int:42;params=object"
        );
    }

    #[test]
    fn mirrors_c_parser_defaults_and_id_kinds() {
        assert_eq!(
            summary(br#"{"id":1,"method":"initialize","params":{}}"#),
            "jsonrpc=2.0;method=initialize;id=int:1;params=object"
        );
        assert_eq!(
            summary(br#"  { "jsonrpc" : "2.0" , "id" : "99" , "method" : "tools/list" }  "#),
            "jsonrpc=2.0;method=tools/list;id=string:99;params=none"
        );
        assert_eq!(
            summary(br#"{"jsonrpc":2,"id":true,"method":"ping","params":null}"#),
            "jsonrpc=2.0;method=ping;id=other;params=null"
        );
    }

    #[test]
    fn rejects_invalid_or_missing_method() {
        assert!(jsonrpc_request_summary(b"not json").is_none());
        assert!(jsonrpc_request_summary(b"").is_none());
        assert!(jsonrpc_request_summary(b"[1,2,3]").is_none());
        assert!(jsonrpc_request_summary(br#"{"jsonrpc":"2.0","id":1}"#).is_none());
        assert!(jsonrpc_request_summary(br#"{"jsonrpc":"2.0","method":7}"#).is_none());
    }

    #[test]
    fn escapes_summary_delimiters() {
        assert_eq!(
            summary(br#"{"method":"weird\nmethod","id":"a;b=c\\d","params":[]}"#),
            "jsonrpc=2.0;method=weird\\nmethod;id=string:a\\;b\\=c\\\\d;params=array"
        );
    }

    #[test]
    fn parses_request_fields_for_c_wrapper() {
        let req = super::jsonrpc_parse(
            br#" { "id" : "abc" , "method" : "tools/call" , "params" : { "name" : "search_graph" } } "#,
        )
        .unwrap();
        assert_eq!(req.jsonrpc, b"2.0");
        assert_eq!(req.method, b"tools/call");
        assert_eq!(req.id, super::ParsedId::String(b"abc".to_vec()));
        assert_eq!(
            req.params_raw,
            Some(br#"{ "name" : "search_graph" }"#.to_vec())
        );

        let unicode = super::jsonrpc_parse(br#"{"id":"\u4e2d","method":"ping"}"#).unwrap();
        assert_eq!(
            unicode.id,
            super::ParsedId::String("中".as_bytes().to_vec())
        );
    }

    #[test]
    fn mirrors_first_key_wins_and_utf8_validation() {
        let duplicate = super::jsonrpc_parse(
            br#"{"method":"ping","method":"tools/list","id":1,"id":"late","params":{"a":1},"params":{"b":2},"jsonrpc":3,"jsonrpc":"late"}"#,
        )
        .unwrap();
        assert_eq!(duplicate.jsonrpc, b"2.0");
        assert_eq!(duplicate.method, b"ping");
        assert_eq!(duplicate.id, super::ParsedId::Int(1));
        assert_eq!(duplicate.params_raw, Some(br#"{"a":1}"#.to_vec()));

        assert!(super::jsonrpc_parse(b"{\"method\":\"bad\xff\"}").is_none());
    }
}
