//! `src/pipeline/pass_githistory.c` 的可追蹤檔案 classifier parity。
//!
//! 只處理路徑前綴、lock/generated basename 與非 source 副檔名；git log、coupling
//! 計算、檔案 I/O、graph mutation 與 pipeline side effects 仍由 C 負責。

const SKIP_PREFIXES: &[&[u8]] = &[
    b".git/",
    b"node_modules/",
    b"vendor/",
    b"__pycache__/",
    b".cache/",
];

const SKIP_BASENAMES: &[&[u8]] = &[
    b"package-lock.json",
    b"yarn.lock",
    b"pnpm-lock.yaml",
    b"Cargo.lock",
    b"poetry.lock",
    b"composer.lock",
    b"Gemfile.lock",
    b"Pipfile.lock",
];

const SKIP_SUFFIXES: &[&[u8]] = &[
    b".lock",
    b".sum",
    b".min.js",
    b".min.css",
    b".map",
    b".wasm",
    b".png",
    b".jpg",
    b".gif",
    b".ico",
    b".svg",
];

#[must_use]
pub fn is_trackable_file(path: Option<&[u8]>) -> bool {
    let Some(path) = path else {
        return false;
    };
    if SKIP_PREFIXES.iter().any(|prefix| path.starts_with(prefix)) {
        return false;
    }

    let basename = path
        .iter()
        .rposition(|byte| *byte == b'/')
        .map_or(path, |idx| &path[idx + 1..]);
    if SKIP_BASENAMES.contains(&basename) {
        return false;
    }
    !SKIP_SUFFIXES.iter().any(|suffix| path.ends_with(suffix))
}

#[cfg(test)]
mod tests {
    use super::is_trackable_file;

    #[test]
    fn accepts_source_and_document_paths() {
        assert!(is_trackable_file(Some(b"main.go")));
        assert!(is_trackable_file(Some(b"src/app.py")));
        assert!(is_trackable_file(Some(b"README.md")));
    }

    #[test]
    fn rejects_ignored_directory_prefixes() {
        for path in [
            b".git/config".as_slice(),
            b"node_modules/pkg/index.js".as_slice(),
            b"vendor/lib/dep.go".as_slice(),
            b"__pycache__/mod.pyc".as_slice(),
            b".cache/generated".as_slice(),
        ] {
            assert!(!is_trackable_file(Some(path)));
        }
    }

    #[test]
    fn rejects_lock_basenames_and_generated_suffixes() {
        for path in [
            b"package-lock.json".as_slice(),
            b"go.sum".as_slice(),
            b"Cargo.lock".as_slice(),
            b"image.png".as_slice(),
            b"src/style.min.css".as_slice(),
            b"bundle.js.map".as_slice(),
        ] {
            assert!(!is_trackable_file(Some(path)));
        }
    }

    #[test]
    fn keeps_unknown_and_null_contract() {
        assert!(is_trackable_file(Some(b"unknown.data")));
        assert!(!is_trackable_file(None));
    }
}
