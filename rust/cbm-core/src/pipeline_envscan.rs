//! `pass_envscan.c` 的檔名與檔案類型分類 helper。

const DOCKERFILE: &[u8] = b"dockerfile";
const SECRET_PATTERNS: [&[u8]; 8] = [
    b"service_account",
    b"credentials",
    b"key.json",
    b"key.pem",
    b"id_rsa",
    b"id_ed25519",
    b".pem",
    b".key",
];
const IGNORED_DIRS: [&[u8]; 15] = [
    b".git",
    b"node_modules",
    b".svn",
    b".hg",
    b"__pycache__",
    b"vendor",
    b".terraform",
    b".cache",
    b".idea",
    b".vscode",
    b"dist",
    b"build",
    b".next",
    b".nuxt",
    b"target",
];

pub const FT_UNKNOWN: i32 = 0;
pub const FT_DOCKERFILE: i32 = 1;
pub const FT_YAML: i32 = 2;
pub const FT_TERRAFORM: i32 = 3;
pub const FT_SHELL: i32 = 4;
pub const FT_ENVFILE: i32 = 5;
pub const FT_TOML: i32 = 6;
pub const FT_PROPERTIES: i32 = 7;

#[must_use]
pub fn is_dockerfile_name(name: Option<&[u8]>) -> bool {
    let Some(name) = name else {
        return false;
    };
    if name.len() >= 256 {
        return false;
    }
    let lower = ascii_lower(name);
    lower == DOCKERFILE
        || lower.starts_with(b"dockerfile.")
        || (lower.len() > 11 && lower.ends_with(b".dockerfile"))
}

#[must_use]
pub fn is_env_file_name(name: Option<&[u8]>) -> bool {
    let Some(name) = name else {
        return false;
    };
    if name.len() >= 256 {
        return false;
    }
    let lower = ascii_lower(name);
    lower == b".env" || lower.starts_with(b".env.") || (lower.len() > 4 && lower.ends_with(b".env"))
}

#[must_use]
pub fn is_secret_file(name: Option<&[u8]>) -> bool {
    let Some(name) = name else {
        return false;
    };
    if name.len() >= 256 {
        return false;
    }
    let lower = ascii_lower(name);
    SECRET_PATTERNS
        .iter()
        .any(|pattern| lower.windows(pattern.len()).any(|w| w == *pattern))
}

#[must_use]
pub fn is_ignored_dir(name: Option<&[u8]>) -> bool {
    name.is_some_and(|name| IGNORED_DIRS.contains(&name))
}

#[must_use]
pub fn detect_file_type(name: Option<&[u8]>) -> i32 {
    let Some(name) = name else {
        return FT_UNKNOWN;
    };
    if is_dockerfile_name(Some(name)) {
        return FT_DOCKERFILE;
    }
    if is_env_file_name(Some(name)) {
        return FT_ENVFILE;
    }
    let Some(dot) = name.iter().rposition(|byte| *byte == b'.') else {
        return FT_UNKNOWN;
    };
    match &name[dot..] {
        b".yaml" | b".yml" => FT_YAML,
        b".tf" | b".hcl" => FT_TERRAFORM,
        b".sh" | b".bash" | b".zsh" => FT_SHELL,
        b".toml" => FT_TOML,
        b".properties" | b".cfg" | b".ini" => FT_PROPERTIES,
        _ => FT_UNKNOWN,
    }
}

fn ascii_lower(value: &[u8]) -> Vec<u8> {
    value.iter().map(u8::to_ascii_lowercase).collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn classifies_docker_env_secret_and_ignored_names() {
        assert!(is_dockerfile_name(Some(b"Dockerfile.prod")));
        assert!(is_dockerfile_name(Some(b"app.DOCKERFILE")));
        assert!(is_env_file_name(Some(b".ENV.local")));
        assert!(is_env_file_name(Some(b"settings.env")));
        assert!(is_secret_file(Some(b"service_account.json")));
        assert!(is_ignored_dir(Some(b"node_modules")));
    }

    #[test]
    fn detects_case_sensitive_source_extensions() {
        assert_eq!(detect_file_type(Some(b"Dockerfile")), FT_DOCKERFILE);
        assert_eq!(detect_file_type(Some(b"app.yml")), FT_YAML);
        assert_eq!(detect_file_type(Some(b"main.tf")), FT_TERRAFORM);
        assert_eq!(detect_file_type(Some(b"run.sh")), FT_SHELL);
        assert_eq!(detect_file_type(Some(b"config.toml")), FT_TOML);
        assert_eq!(detect_file_type(Some(b"local.ini")), FT_PROPERTIES);
        assert_eq!(detect_file_type(Some(b"main.YML")), FT_UNKNOWN);
    }
}
