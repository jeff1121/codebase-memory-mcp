//! `src/pipeline/artifact.c` 的 artifact 路徑組裝 helper。
//!
//! 這裡只固定 `<repo>/.codebase-memory/<name>` 的 byte-level contract；
//! artifact 壓縮、解壓、I/O、metadata 與 pipeline side effects 仍由 C 負責。

const ARTIFACT_DIR: &[u8] = b".codebase-memory";

#[must_use]
pub fn artifact_path(repo_path: Option<&[u8]>, name: Option<&[u8]>) -> Option<Vec<u8>> {
    let repo_path = repo_path?;
    let name = name?;

    let mut out = Vec::with_capacity(repo_path.len() + ARTIFACT_DIR.len() + name.len() + 2);
    out.extend_from_slice(repo_path);
    out.push(b'/');
    out.extend_from_slice(ARTIFACT_DIR);
    out.push(b'/');
    out.extend_from_slice(name);
    Some(out)
}

#[cfg(test)]
mod tests {
    use super::artifact_path;

    #[test]
    fn artifact_path_builds_expected_paths() {
        assert_eq!(
            artifact_path(Some(b"/tmp/repo"), Some(b"graph.db.zst")),
            Some(b"/tmp/repo/.codebase-memory/graph.db.zst".to_vec())
        );
        assert_eq!(
            artifact_path(Some(b"/tmp/repo"), Some(b"artifact.json")),
            Some(b"/tmp/repo/.codebase-memory/artifact.json".to_vec())
        );
    }

    #[test]
    fn artifact_path_keeps_raw_bytes() {
        assert_eq!(
            artifact_path(Some(b"/tmp/\xff"), Some(b"art\x80ifact.json")),
            Some(b"/tmp/\xff/.codebase-memory/art\x80ifact.json".to_vec())
        );
    }

    #[test]
    fn artifact_path_handles_empty_segments() {
        assert_eq!(
            artifact_path(Some(b""), Some(b"")),
            Some(b"/.codebase-memory/".to_vec())
        );
    }

    #[test]
    fn artifact_path_returns_none_for_missing_inputs() {
        assert_eq!(artifact_path(None, Some(b"graph.db.zst")), None);
        assert_eq!(artifact_path(Some(b"/tmp/repo"), None), None);
    }
}
