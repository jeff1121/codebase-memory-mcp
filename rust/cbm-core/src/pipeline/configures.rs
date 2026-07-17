//! `src/pipeline/pass_configures.c` 的純 helper parity。

const KEY_BUF_CAP: usize = 512;
const MIN_ENV_NAME_LEN: usize = 2;

const CONFIG_EXTENSIONS: &[&[u8]] = &[
    b".toml",
    b".ini",
    b".yaml",
    b".yml",
    b".cfg",
    b".properties",
    b".json",
    b".xml",
    b".conf",
    b".env",
];

#[must_use]
pub fn is_env_var_name(value: Option<&[u8]>) -> bool {
    let Some(value) = value else {
        return false;
    };
    if value.len() < MIN_ENV_NAME_LEN {
        return false;
    }

    let mut has_upper = false;
    for &byte in value {
        if byte.is_ascii_uppercase() {
            has_upper = true;
        } else if byte == b'_' || byte.is_ascii_digit() {
            continue;
        } else {
            return false;
        }
    }
    has_upper
}

#[must_use]
pub fn normalize_config_key_bytes(key: Option<&[u8]>) -> (Vec<u8>, usize) {
    let Some(key) = key else {
        return (Vec::new(), 0);
    };

    let mut buf = key
        .iter()
        .copied()
        .take(KEY_BUF_CAP - 1)
        .collect::<Vec<_>>();
    for byte in &mut buf {
        if matches!(*byte, b'_' | b'-' | b'.') {
            *byte = b' ';
        }
    }

    let mut normalized = Vec::with_capacity(buf.len().saturating_mul(2));
    let mut token_count = 0usize;
    for part in buf.split(|byte| *byte == b' ') {
        if part.is_empty() {
            continue;
        }
        emit_camel_words(part, &mut normalized, &mut token_count);
    }

    (normalized, token_count)
}

#[must_use]
pub fn has_config_extension(path: Option<&[u8]>) -> bool {
    let Some(path) = path else {
        return false;
    };

    let basename = match path.iter().rposition(|byte| *byte == b'/') {
        Some(idx) => &path[idx + 1..],
        None => path,
    };
    if basename == b".env" {
        return true;
    }

    let Some(dot_idx) = path.iter().rposition(|byte| *byte == b'.') else {
        return false;
    };
    let ext = &path[dot_idx..];
    CONFIG_EXTENSIONS.contains(&ext)
}

fn emit_camel_words(part: &[u8], out: &mut Vec<u8>, token_count: &mut usize) {
    let mut start = 0usize;
    for idx in 1..part.len() {
        if part[idx].is_ascii_uppercase() && part[idx - 1].is_ascii_lowercase() {
            if !out.is_empty() {
                out.push(b'_');
            }
            out.extend(
                part[start..idx]
                    .iter()
                    .map(|byte| byte.to_ascii_lowercase()),
            );
            *token_count += 1;
            start = idx;
        }
    }

    if !out.is_empty() {
        out.push(b'_');
    }
    out.extend(part[start..].iter().map(|byte| byte.to_ascii_lowercase()));
    *token_count += 1;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn env_var_detection_matches_c_contract() {
        assert!(is_env_var_name(Some(b"DATABASE_URL")));
        assert!(is_env_var_name(Some(b"API_KEY")));
        assert!(is_env_var_name(Some(b"PORT")));
        assert!(is_env_var_name(Some(b"DB_2")));
        assert!(!is_env_var_name(Some(b"A")));
        assert!(!is_env_var_name(Some(b"port")));
        assert!(!is_env_var_name(Some(b"apiKey")));
        assert!(!is_env_var_name(Some(b"__")));
        assert!(!is_env_var_name(Some(b"")));
        assert!(!is_env_var_name(None));
    }

    #[test]
    fn normalize_config_key_matches_c_contract() {
        let (normalized, tokens) = normalize_config_key_bytes(Some(b"max_connections"));
        assert_eq!(normalized, b"max_connections");
        assert_eq!(tokens, 2);

        let (normalized, tokens) = normalize_config_key_bytes(Some(b"maxConnections"));
        assert_eq!(normalized, b"max_connections");
        assert_eq!(tokens, 2);

        let (normalized, tokens) = normalize_config_key_bytes(Some(b"DATABASE_HOST"));
        assert_eq!(normalized, b"database_host");
        assert_eq!(tokens, 2);

        let (normalized, tokens) = normalize_config_key_bytes(Some(b"database.host"));
        assert_eq!(normalized, b"database_host");
        assert_eq!(tokens, 2);

        let (normalized, tokens) = normalize_config_key_bytes(Some(b"port"));
        assert_eq!(normalized, b"port");
        assert_eq!(tokens, 1);

        let (normalized, tokens) = normalize_config_key_bytes(Some(b"maxRetryCount"));
        assert_eq!(normalized, b"max_retry_count");
        assert_eq!(tokens, 3);

        let long_key = vec![b'a'; 600];
        let (normalized, tokens) = normalize_config_key_bytes(Some(&long_key));
        assert_eq!(normalized.len(), KEY_BUF_CAP - 1);
        assert_eq!(tokens, 1);
        assert!(normalize_config_key_bytes(None).0.is_empty());
    }

    #[test]
    fn config_extension_detection_matches_c_contract() {
        assert!(has_config_extension(Some(b"config.toml")));
        assert!(has_config_extension(Some(b"settings.yaml")));
        assert!(has_config_extension(Some(b"config.yml")));
        assert!(has_config_extension(Some(b".env")));
        assert!(has_config_extension(Some(b"config.ini")));
        assert!(has_config_extension(Some(b"data.json")));
        assert!(has_config_extension(Some(b"pom.xml")));
        assert!(!has_config_extension(Some(b"main.go")));
        assert!(!has_config_extension(Some(b"app.py")));
        assert!(!has_config_extension(Some(b"data.csv")));
        assert!(!has_config_extension(None));
    }
}
