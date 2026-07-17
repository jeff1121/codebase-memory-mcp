//! `src/pipeline/pass_infrascan.c` 的純 helper parity。
//!
//! 這裡只固定 `cbm_is_dockerfile()`、`cbm_is_compose_file()`、
//! `cbm_is_env_file()`、`cbm_is_cloudbuild_file()`、`cbm_is_kustomize_file()` 與
//! `cbm_is_shell_script()` 的 byte-level contract；實際 infra scan、
//! k8s manifest 判斷與其他 pipeline side effects 仍由 C 負責。

fn eq_ignore_ascii_case(left: &[u8], right: &[u8]) -> bool {
    left.len() == right.len()
        && left
            .iter()
            .zip(right.iter())
            .all(|(lhs, rhs)| lhs.eq_ignore_ascii_case(rhs))
}

const MAX_LOWER_BYTES: usize = 255;

fn lower_ascii_truncated(bytes: &[u8]) -> Vec<u8> {
    bytes
        .iter()
        .take(MAX_LOWER_BYTES)
        .map(|byte| byte.to_ascii_lowercase())
        .collect()
}

fn starts_with_ignore_ascii_case(bytes: &[u8], prefix: &[u8]) -> bool {
    bytes.len() >= prefix.len() && eq_ignore_ascii_case(&bytes[..prefix.len()], prefix)
}

fn ends_with_ignore_ascii_case(bytes: &[u8], suffix: &[u8]) -> bool {
    bytes.len() >= suffix.len()
        && eq_ignore_ascii_case(&bytes[bytes.len() - suffix.len()..], suffix)
}

#[must_use]
pub fn is_dockerfile(name: Option<&[u8]>) -> bool {
    let Some(name) = name else {
        return false;
    };
    let lower = lower_ascii_truncated(name);
    if eq_ignore_ascii_case(&lower, b"dockerfile") {
        return true;
    }
    if starts_with_ignore_ascii_case(&lower, b"dockerfile.") {
        return true;
    }
    lower.len() > b".dockerfile".len() && ends_with_ignore_ascii_case(&lower, b".dockerfile")
}

#[must_use]
pub fn is_compose_file(name: Option<&[u8]>) -> bool {
    let Some(name) = name else {
        return false;
    };
    let lower = lower_ascii_truncated(name);
    let prefix_match = starts_with_ignore_ascii_case(&lower, b"docker-compose")
        || eq_ignore_ascii_case(&lower, b"compose.yml")
        || eq_ignore_ascii_case(&lower, b"compose.yaml");
    if !prefix_match {
        return false;
    }

    let Some(dot_idx) = lower.iter().rposition(|byte| *byte == b'.') else {
        return false;
    };
    let ext = &lower[dot_idx..];
    eq_ignore_ascii_case(ext, b".yml") || eq_ignore_ascii_case(ext, b".yaml")
}

#[must_use]
pub fn is_env_file(name: Option<&[u8]>) -> bool {
    let Some(name) = name else {
        return false;
    };
    let lower = lower_ascii_truncated(name);
    if eq_ignore_ascii_case(&lower, b".env") {
        return true;
    }
    if starts_with_ignore_ascii_case(&lower, b".env.") {
        return true;
    }
    ends_with_ignore_ascii_case(&lower, b".env")
}

#[must_use]
pub fn is_cloudbuild_file(name: Option<&[u8]>) -> bool {
    let Some(name) = name else {
        return false;
    };
    let lower = lower_ascii_truncated(name);
    if !starts_with_ignore_ascii_case(&lower, b"cloudbuild") {
        return false;
    }

    let Some(dot_idx) = lower.iter().rposition(|byte| *byte == b'.') else {
        return false;
    };
    let ext = &lower[dot_idx..];
    ends_with_ignore_ascii_case(ext, b".yml") || ends_with_ignore_ascii_case(ext, b".yaml")
}

#[must_use]
pub fn is_kustomize_file(name: Option<&[u8]>) -> bool {
    let Some(name) = name else {
        return false;
    };
    let lower = lower_ascii_truncated(name);
    eq_ignore_ascii_case(&lower, b"kustomization.yaml")
        || eq_ignore_ascii_case(&lower, b"kustomization.yml")
}

#[must_use]
pub fn is_shell_script(_name: Option<&[u8]>, ext: Option<&[u8]>) -> bool {
    let Some(ext) = ext else {
        return false;
    };
    ext == b".sh" || ext == b".bash" || ext == b".zsh"
}

#[must_use]
pub fn is_helm_chart_file(name: Option<&[u8]>) -> bool {
    matches!(name, Some(b"Chart.yaml") | Some(b"Chart.yml"))
}

#[must_use]
pub fn is_gomod_file(name: Option<&[u8]>) -> bool {
    name == Some(b"go.mod")
}

#[must_use]
pub fn is_requirements_file(name: Option<&[u8]>) -> bool {
    name == Some(b"requirements.txt")
}

#[must_use]
pub fn clean_json_brackets(input: Option<&[u8]>, output_size: usize) -> Option<Vec<u8>> {
    let input = input?;
    if output_size == 0 {
        return None;
    }

    let max_len = output_size - 1;
    if input.len() >= 2 && input[0] == b'[' && input[input.len() - 1] == b']' {
        let mut output = Vec::with_capacity(max_len.min(input.len()));
        let mut in_space = false;
        for byte in &input[1..input.len() - 1] {
            if output.len() >= max_len {
                break;
            }
            if *byte == b'"' {
                continue;
            }
            let byte = if *byte == b',' { b' ' } else { *byte };
            if byte == b' ' || byte == b'\t' {
                if !in_space && !output.is_empty() {
                    output.push(b' ');
                    in_space = true;
                }
            } else {
                output.push(byte);
                in_space = false;
            }
        }
        while output.last() == Some(&b' ') {
            output.pop();
        }
        Some(output)
    } else {
        Some(input[..input.len().min(max_len)].to_vec())
    }
}

#[cfg(test)]
mod tests {
    use super::{
        clean_json_brackets, is_cloudbuild_file, is_compose_file, is_dockerfile, is_env_file,
        is_gomod_file, is_helm_chart_file, is_kustomize_file, is_requirements_file,
        is_shell_script,
    };

    #[test]
    fn dockerfile_matches_c_contract() {
        assert!(is_dockerfile(Some(b"Dockerfile")));
        assert!(is_dockerfile(Some(b"dockerfile")));
        assert!(is_dockerfile(Some(b"Dockerfile.prod")));
        assert!(is_dockerfile(Some(b"app.dockerfile")));
        assert!(!is_dockerfile(Some(b".dockerfile")));
        assert!(!is_dockerfile(Some(b"docker-compose.yml")));
        assert!(!is_dockerfile(Some(b"main.go")));
        assert!(!is_dockerfile(None));
    }

    #[test]
    fn compose_matches_c_contract() {
        assert!(is_compose_file(Some(b"docker-compose.yml")));
        assert!(is_compose_file(Some(b"docker-compose.yaml")));
        assert!(is_compose_file(Some(b"docker-compose.prod.yml")));
        assert!(is_compose_file(Some(b"compose.yml")));
        assert!(is_compose_file(Some(b"compose.yaml")));
        assert!(!is_compose_file(Some(b"mycompose.yml")));
        assert!(!is_compose_file(Some(b"docker-compose.txt")));
        assert!(!is_compose_file(Some(b"Dockerfile")));
        assert!(!is_compose_file(None));
    }

    #[test]
    fn env_matches_c_contract() {
        assert!(is_env_file(Some(b".env")));
        assert!(is_env_file(Some(b".env.production")));
        assert!(is_env_file(Some(b"foo.env")));
        assert!(is_env_file(Some(b"FOO.ENV")));
        assert!(!is_env_file(Some(b"env")));
        assert!(!is_env_file(Some(b"foo.env.bak")));
        assert!(!is_env_file(None));
    }

    #[test]
    fn cloudbuild_matches_c_contract() {
        assert!(is_cloudbuild_file(Some(b"cloudbuild.yaml")));
        assert!(is_cloudbuild_file(Some(b"cloudbuild.yml")));
        assert!(is_cloudbuild_file(Some(b"cloudbuild-prod.yaml")));
        assert!(is_cloudbuild_file(Some(b"Cloudbuild.yml")));
        assert!(!is_cloudbuild_file(Some(b"build.yaml")));
        assert!(!is_cloudbuild_file(Some(b"cloudbuild")));
        assert!(!is_cloudbuild_file(Some(b"cloudbuild.txt")));
        assert!(!is_cloudbuild_file(Some(b"mycloudbuild.yaml")));
        assert!(!is_cloudbuild_file(None));
    }

    #[test]
    fn kustomize_matches_c_contract() {
        assert!(is_kustomize_file(Some(b"kustomization.yaml")));
        assert!(is_kustomize_file(Some(b"kustomization.yml")));
        assert!(is_kustomize_file(Some(b"KUSTOMIZATION.YAML")));
        assert!(!is_kustomize_file(Some(b"deployment.yaml")));
        assert!(!is_kustomize_file(Some(b"kustomize.yaml")));
        assert!(!is_kustomize_file(Some(b"kustomization.yml.bak")));
        assert!(!is_kustomize_file(None));
    }

    #[test]
    fn dockerfile_truncates_like_c_contract() {
        let mut name = vec![b'a'; 248];
        name.extend_from_slice(b".dockerfile");
        assert!(!is_dockerfile(Some(&name)));
    }

    #[test]
    fn shell_script_matches_c_contract() {
        assert!(is_shell_script(Some(b"run.sh"), Some(b".sh")));
        assert!(is_shell_script(Some(b"deploy.bash"), Some(b".bash")));
        assert!(is_shell_script(Some(b"init.zsh"), Some(b".zsh")));
        assert!(is_shell_script(None, Some(b".sh")));
        assert!(!is_shell_script(Some(b"main.py"), Some(b".py")));
        assert!(!is_shell_script(Some(b"Dockerfile"), Some(b"")));
        assert!(!is_shell_script(Some(b"run.sh"), None));
    }

    #[test]
    fn dependency_manifest_names_match_c_contract() {
        assert!(is_helm_chart_file(Some(b"Chart.yaml")));
        assert!(is_helm_chart_file(Some(b"Chart.yml")));
        assert!(!is_helm_chart_file(Some(b"chart.yaml")));
        assert!(is_gomod_file(Some(b"go.mod")));
        assert!(!is_gomod_file(Some(b"go.sum")));
        assert!(is_requirements_file(Some(b"requirements.txt")));
        assert!(!is_requirements_file(Some(b"requirements-dev.txt")));
        assert!(!is_requirements_file(None));
    }

    #[test]
    fn clean_json_brackets_matches_c_contract() {
        assert_eq!(
            clean_json_brackets(Some(b"[\"alpha\", \"beta\",\t\"gamma\"]"), 64),
            Some(b"alpha beta gamma".to_vec())
        );
        assert_eq!(
            clean_json_brackets(Some(b"plain value"), 7),
            Some(b"plain ".to_vec())
        );
        assert_eq!(
            clean_json_brackets(Some(b"[\"alpha\", \"beta\"]"), 7),
            Some(b"alpha".to_vec())
        );
        assert_eq!(clean_json_brackets(None, 8), None);
        assert_eq!(clean_json_brackets(Some(b"value"), 0), None);
    }
}
