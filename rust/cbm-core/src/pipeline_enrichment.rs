//! `pass_enrichment.c` 的 decorator tokenization helpers。

const DECORATOR_BUF_SIZE: usize = 256;
const MAX_CAMEL_PARTS: usize = 16;

const STOPWORDS: [&[u8]; 22] = [
    b"get",
    b"set",
    b"new",
    b"class",
    b"method",
    b"function",
    b"value",
    b"type",
    b"param",
    b"return",
    b"public",
    b"private",
    b"for",
    b"if",
    b"the",
    b"and",
    b"or",
    b"not",
    b"with",
    b"from",
    b"app",
    b"router",
];

#[must_use]
pub fn split_camel_case(value: Option<&[u8]>, max_output: usize) -> Vec<Vec<u8>> {
    let Some(value) = value else {
        return Vec::new();
    };
    if value.is_empty() || max_output == 0 {
        return Vec::new();
    }

    let mut output = Vec::new();
    let mut start = 0;
    for index in 1..value.len() {
        if value[index].is_ascii_uppercase() && value[index - 1].is_ascii_lowercase() {
            if output.len() < max_output {
                output.push(value[start..index].to_vec());
            }
            start = index;
        }
    }
    if output.len() < max_output {
        output.push(value[start..].to_vec());
    }
    output
}

#[must_use]
pub fn tokenize_decorator(value: Option<&[u8]>, max_output: usize) -> Vec<Vec<u8>> {
    let Some(value) = value else {
        return Vec::new();
    };
    if max_output == 0 {
        return Vec::new();
    }

    let mut normalized = value[..value.len().min(DECORATOR_BUF_SIZE - 1)].to_vec();
    if normalized.first() == Some(&b'@') {
        normalized.remove(0);
    }
    if normalized.starts_with(b"#[") {
        normalized.drain(..2);
        if normalized.last() == Some(&b']') {
            normalized.pop();
        }
    }
    if let Some(paren) = normalized.iter().position(|byte| *byte == b'(') {
        normalized.truncate(paren);
    }
    for byte in &mut normalized {
        if matches!(*byte, b'.' | b'_' | b'-' | b':' | b'/') {
            *byte = b' ';
        }
    }

    let mut output = Vec::new();
    for part in normalized.split(|byte| *byte == b' ') {
        if part.is_empty() || output.len() >= max_output {
            continue;
        }
        for mut camel_part in split_camel_case(Some(part), MAX_CAMEL_PARTS) {
            camel_part.make_ascii_lowercase();
            if camel_part.len() >= 2 && !STOPWORDS.contains(&camel_part.as_slice()) {
                output.push(camel_part);
                if output.len() >= max_output {
                    break;
                }
            }
        }
    }
    output
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn splits_lower_to_upper_boundaries() {
        assert_eq!(
            split_camel_case(Some(b"loadHTTPServer"), 8),
            vec![b"load".to_vec(), b"HTTPServer".to_vec()]
        );
    }

    #[test]
    fn tokenizes_decorator_syntax_and_filters_stopwords() {
        assert_eq!(
            tokenize_decorator(Some(b"@router.getUser_config(arg)"), 8),
            vec![b"user".to_vec(), b"config".to_vec()]
        );
        assert_eq!(
            tokenize_decorator(Some(b"#[derive(Debug)]"), 8),
            vec![b"derive".to_vec()]
        );
    }

    #[test]
    fn tokenizes_ascii_only_and_truncates_decorator_input() {
        assert_eq!(
            tokenize_decorator(Some(b"@M\xC9thod"), 2),
            vec![b"m\xC9thod".to_vec()]
        );

        let mut long_decorator = vec![b'@'];
        long_decorator.extend(std::iter::repeat(b'a').take(DECORATOR_BUF_SIZE - 1));
        assert_eq!(
            tokenize_decorator(Some(&long_decorator), 1),
            vec![vec![b'a'; DECORATOR_BUF_SIZE - 2]]
        );
    }
}
