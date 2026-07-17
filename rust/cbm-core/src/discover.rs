//! `src/discover/discover.c` 的目錄與檔名 filter helpers。

use std::os::raw::c_int;

const MODE_FULL: c_int = 0;

const ALWAYS_SKIP_DIRS: &[&[u8]] = &[
    b".git",
    b".hg",
    b".svn",
    b".worktrees",
    b".idea",
    b".vs",
    b".vscode",
    b".eclipse",
    b".claude",
    b".claude-worktrees",
    b"Antigravity",
    b".cache",
    b".eggs",
    b".env",
    b".mypy_cache",
    b".nox",
    b".pytest_cache",
    b".ruff_cache",
    b".tox",
    b".venv",
    b"__pycache__",
    b"env",
    b"htmlcov",
    b"site-packages",
    b"venv",
    b".npm",
    b".nyc_output",
    b".pnpm-store",
    b".yarn",
    b"bower_components",
    b"coverage",
    b"node_modules",
    b".next",
    b".nuxt",
    b".svelte-kit",
    b".angular",
    b".turbo",
    b".parcel-cache",
    b".docusaurus",
    b".expo",
    b"dist",
    b"obj",
    b"Pods",
    b"target",
    b"temp",
    b"tmp",
    b".terraform",
    b".serverless",
    b"bazel-bin",
    b"bazel-out",
    b"bazel-testlogs",
    b".cargo",
    b".stack-work",
    b".dart_tool",
    b"zig-cache",
    b"zig-out",
    b".metals",
    b".bloop",
    b".bsp",
    b".ccls-cache",
    b".clangd",
    b"elm-stuff",
    b"_opam",
    b".cpcache",
    b".shadow-cljs",
    b".vercel",
    b".netlify",
    b"deploy",
    b"deployed",
    b".codebase-memory",
    b".qdrant_code_embeddings",
    b".tmp",
    b"vendor",
    b"vendored",
];

const FAST_SKIP_DIRS: &[&[u8]] = &[
    b"generated",
    b"gen",
    b"auto-generated",
    b"fixtures",
    b"testdata",
    b"test_data",
    b"__tests__",
    b"__mocks__",
    b"__snapshots__",
    b"__fixtures__",
    b"__test__",
    b"docs",
    b"doc",
    b"documentation",
    b"examples",
    b"example",
    b"samples",
    b"sample",
    b"assets",
    b"static",
    b"public",
    b"media",
    b"third_party",
    b"thirdparty",
    b"3rdparty",
    b"external",
    b"migrations",
    b"seeds",
    b"e2e",
    b"integration",
    b"locale",
    b"locales",
    b"i18n",
    b"l10n",
    b"scripts",
    b"tools",
    b"hack",
    b"bin",
    b"build",
    b"out",
];

const ALWAYS_IGNORED_SUFFIXES: &[&[u8]] = &[
    b".tmp",
    b"~",
    b".pyc",
    b".pyo",
    b".o",
    b".a",
    b".so",
    b".dll",
    b".class",
    b".png",
    b".jpg",
    b".jpeg",
    b".gif",
    b".ico",
    b".bmp",
    b".tiff",
    b".webp",
    b".svg",
    b".wasm",
    b".node",
    b".exe",
    b".bin",
    b".dat",
    b".db",
    b".sqlite",
    b".sqlite3",
    b".woff",
    b".woff2",
    b".ttf",
    b".eot",
    b".otf",
];

const FAST_IGNORED_SUFFIXES: &[&[u8]] = &[
    b".zip",
    b".tar",
    b".gz",
    b".bz2",
    b".xz",
    b".rar",
    b".7z",
    b".jar",
    b".war",
    b".ear",
    b".mp3",
    b".mp4",
    b".avi",
    b".mov",
    b".wav",
    b".flac",
    b".ogg",
    b".mkv",
    b".webm",
    b".pdf",
    b".doc",
    b".docx",
    b".xls",
    b".xlsx",
    b".ppt",
    b".pptx",
    b".odt",
    b".ods",
    b".map",
    b".min.js",
    b".min.css",
    b".pem",
    b".crt",
    b".key",
    b".cer",
    b".p12",
    b".pb",
    b".avro",
    b".parquet",
    b".beam",
    b".elc",
    b".rlib",
    b".coverage",
    b".prof",
    b".out",
    b".patch",
    b".diff",
];

const FAST_PATTERNS: &[&[u8]] = &[
    b".d.ts",
    b".bundle.",
    b".chunk.",
    b".generated.",
    b".pb.go",
    b"_pb2.py",
    b".pb2.py",
    b"_grpc.pb.go",
    b"_string.go",
    b"mock_",
    b"_mock.",
    b"_test_helpers.",
    b".stories.",
    b".spec.",
    b".test.",
];

#[inline]
const fn is_restricted_mode(mode: c_int) -> bool {
    mode != MODE_FULL
}

#[inline]
fn in_list(list: &[&[u8]], value: &[u8]) -> bool {
    list.contains(&value)
}

#[inline]
fn has_suffix(path: &[u8], suffix: &[u8]) -> bool {
    path.len() >= suffix.len() && &path[path.len() - suffix.len()..] == suffix
}

#[inline]
fn contains_any_fast_pattern(path: &[u8]) -> bool {
    FAST_PATTERNS
        .iter()
        .copied()
        .any(|pattern| contains(path, pattern))
}

#[inline]
fn contains(haystack: &[u8], needle: &[u8]) -> bool {
    if needle.is_empty() || haystack.len() < needle.len() {
        return false;
    }
    for idx in 0..=haystack.len() - needle.len() {
        if &haystack[idx..idx + needle.len()] == needle {
            return true;
        }
    }
    false
}

#[must_use]
pub fn local_rel_path_offset(rel_path: Option<&[u8]>, local_prefix: Option<&[u8]>) -> usize {
    let (Some(rel_path), Some(local_prefix)) = (rel_path, local_prefix) else {
        return 0;
    };
    if local_prefix.is_empty() || !rel_path.starts_with(local_prefix) {
        return 0;
    }

    let separator_offset = local_prefix.len();
    if rel_path.get(separator_offset) == Some(&b'/') {
        separator_offset + 1
    } else {
        0
    }
}

#[cfg(test)]
mod local_rel_path_offset_tests {
    use super::local_rel_path_offset;

    #[test]
    fn accepts_prefix_followed_by_separator() {
        assert_eq!(
            local_rel_path_offset(Some(b".codebase-memory/data"), Some(b".codebase-memory")),
            17
        );
    }

    #[test]
    fn rejects_exact_prefix_and_non_boundary_prefix() {
        assert_eq!(
            local_rel_path_offset(Some(b".codebase-memory"), Some(b".codebase-memory")),
            0
        );
        assert_eq!(
            local_rel_path_offset(
                Some(b".codebase-memory-old/data"),
                Some(b".codebase-memory")
            ),
            0
        );
    }

    #[test]
    fn preserves_byte_case_and_separator_rules() {
        assert_eq!(
            local_rel_path_offset(Some(b".CODEBASE-MEMORY/data"), Some(b".codebase-memory")),
            0
        );
        assert_eq!(
            local_rel_path_offset(Some(b".codebase-memory\\data"), Some(b".codebase-memory")),
            0
        );
    }

    #[test]
    fn rejects_missing_or_empty_inputs() {
        assert_eq!(local_rel_path_offset(None, Some(b".codebase-memory")), 0);
        assert_eq!(
            local_rel_path_offset(Some(b".codebase-memory/data"), None),
            0
        );
        assert_eq!(
            local_rel_path_offset(Some(b".codebase-memory/data"), Some(b"")),
            0
        );
    }

    #[test]
    fn accepts_separator_without_remaining_path() {
        assert_eq!(
            local_rel_path_offset(Some(b".codebase-memory/"), Some(b".codebase-memory")),
            17
        );
    }
}

#[must_use]
pub fn should_skip_dir(value: Option<&[u8]>, mode: c_int) -> bool {
    let Some(value) = value else {
        return false;
    };
    if in_list(ALWAYS_SKIP_DIRS, value) {
        return true;
    }
    is_restricted_mode(mode) && in_list(FAST_SKIP_DIRS, value)
}

#[must_use]
pub fn has_ignored_suffix(value: Option<&[u8]>, mode: c_int) -> bool {
    let Some(value) = value else {
        return false;
    };
    if ALWAYS_IGNORED_SUFFIXES
        .iter()
        .copied()
        .any(|suffix| has_suffix(value, suffix))
    {
        return true;
    }
    is_restricted_mode(mode)
        && FAST_IGNORED_SUFFIXES
            .iter()
            .copied()
            .any(|suffix| has_suffix(value, suffix))
}

#[must_use]
pub fn should_skip_filename(value: Option<&[u8]>, mode: c_int) -> bool {
    let Some(value) = value else {
        return false;
    };
    is_restricted_mode(mode) && in_list(FAST_SKIP_FILENAMES, value)
}

const FAST_SKIP_FILENAMES: &[&[u8]] = &[
    b"LICENSE",
    b"LICENSE.txt",
    b"LICENSE.md",
    b"LICENSE-MIT",
    b"LICENSE-APACHE",
    b"LICENCE",
    b"LICENCE.txt",
    b"LICENCE.md",
    b"CHANGELOG",
    b"CHANGELOG.md",
    b"CHANGES.md",
    b"HISTORY",
    b"HISTORY.md",
    b"AUTHORS",
    b"AUTHORS.md",
    b"CONTRIBUTORS",
    b"CONTRIBUTORS.md",
    b"CODEOWNERS",
    b"go.sum",
    b"yarn.lock",
    b"pnpm-lock.yaml",
    b"Pipfile.lock",
    b"poetry.lock",
    b"Gemfile.lock",
    b"Cargo.lock",
    b"mix.lock",
    b"flake.lock",
    b"pubspec.lock",
    b"composer.lock",
    b"package-lock.json",
    b"configure",
    b"Makefile.in",
    b"config.guess",
    b"config.sub",
];

#[must_use]
pub fn matches_fast_pattern(value: Option<&[u8]>, mode: c_int) -> bool {
    is_restricted_mode(mode) && value.is_some_and(contains_any_fast_pattern)
}

#[must_use]
pub fn str_in_list(value: &[u8], list: &[&[u8]]) -> bool {
    list.contains(&value)
}

#[must_use]
pub fn ends_with(value: Option<&[u8]>, suffix: Option<&[u8]>) -> bool {
    let (Some(value), Some(suffix)) = (value, suffix) else {
        return false;
    };
    value.ends_with(suffix)
}

#[must_use]
pub fn str_contains(value: Option<&[u8]>, substring: Option<&[u8]>) -> bool {
    let (Some(value), Some(substring)) = (value, substring) else {
        return false;
    };
    substring.is_empty()
        || value
            .windows(substring.len())
            .any(|window| window == substring)
}

#[must_use]
pub fn language_marker(value: Option<&[u8]>, kind: c_int) -> bool {
    let Some(value) = value else {
        return false;
    };
    match kind {
        0 => [
            b"@interface".as_slice(),
            b"@implementation".as_slice(),
            b"@protocol".as_slice(),
            b"@property".as_slice(),
            b"#import".as_slice(),
            b"@selector".as_slice(),
            b"@encode".as_slice(),
            b"@synthesize".as_slice(),
            b"@dynamic".as_slice(),
        ]
        .iter()
        .any(|marker| value.windows(marker.len()).any(|window| window == *marker)),
        1 => [
            b"end function;".as_slice(),
            b"end procedure;".as_slice(),
            b"end intrinsic;".as_slice(),
            b"end if;".as_slice(),
            b"end for;".as_slice(),
            b"end while;".as_slice(),
        ]
        .iter()
        .any(|marker| value.windows(marker.len()).any(|window| window == *marker)),
        2 => [b"intrinsic ".as_slice(), b"procedure ".as_slice()]
            .iter()
            .any(|marker| {
                let Some(start) = value
                    .windows(marker.len())
                    .position(|window| window == *marker)
                else {
                    return false;
                };
                let mut index = start + marker.len();
                let name_start = index;
                while value
                    .get(index)
                    .is_some_and(|byte| byte.is_ascii_alphabetic())
                {
                    index += 1;
                }
                index > name_start && value.get(index) == Some(&b'(')
            }),
        3 => value.split(|byte| *byte == b'\n').any(|line| {
            let mut line = line;
            while matches!(line.first().copied(), Some(b' ' | b'\t')) {
                line = &line[1..];
            }
            line.starts_with(b"function ")
                || line.starts_with(b"function\t")
                || line.starts_with(b"classdef ")
                || line.starts_with(b"classdef\t")
                || line.starts_with(b"%%")
                || (line.first() == Some(&b'%') && line.get(1) != Some(&b'{'))
        }),
        _ => false,
    }
}

#[must_use]
pub fn ascii_ieq(left: Option<&[u8]>, right: Option<&[u8]>) -> bool {
    let (Some(left), Some(right)) = (left, right) else {
        return false;
    };
    left.eq_ignore_ascii_case(right)
}

#[must_use]
pub fn has_trailing_sep(value: Option<&[u8]>) -> bool {
    value.is_some_and(|value| matches!(value.last(), Some(b'/' | b'\\')))
}

fn canonicalize_drive(path: &mut [u8]) {
    if path.len() >= 2
        && path[0].is_ascii_lowercase()
        && path[1] == b':'
        && (path.len() == 2 || path[2] == b'/')
    {
        path[0] = path[0].to_ascii_uppercase();
    }
}

#[must_use]
pub fn path_join(base: Option<&[u8]>, relative: Option<&[u8]>, output_size: usize) -> Vec<u8> {
    let base_empty = base.map_or(true, |value| value.is_empty());
    let relative_empty = relative.map_or(true, |value| value.is_empty());
    let mut joined = Vec::new();
    match (base_empty, relative_empty) {
        (true, _) => joined.extend_from_slice(relative.unwrap_or_default()),
        (_, true) => joined.extend_from_slice(base.unwrap_or_default()),
        (false, false) => {
            joined.extend_from_slice(base.unwrap_or_default());
            if !has_trailing_sep(base) {
                joined.push(b'/');
            }
            joined.extend_from_slice(relative.unwrap_or_default());
        }
    }

    let copy_len = joined.len().min(output_size.saturating_sub(1));
    joined.truncate(copy_len);
    for byte in &mut joined {
        if *byte == b'\\' {
            *byte = b'/';
        }
    }
    canonicalize_drive(&mut joined);
    joined
}

#[cfg(test)]
mod string_helper_tests {
    use super::{ascii_ieq, ends_with, has_trailing_sep, path_join, str_contains, str_in_list};

    #[test]
    fn string_helpers_match_discover_contract() {
        assert!(str_in_list(b"docs", &[b"src", b"docs"]));
        assert!(!str_in_list(b"tests", &[b"src", b"docs"]));
        assert!(ends_with(Some(b"module.pyc"), Some(b".pyc")));
        assert!(ends_with(Some(b"module.pyc"), Some(b"")));
        assert!(!ends_with(Some(b"module.pyc"), Some(b".c")));
        assert!(!ends_with(Some(b"module.pyc"), Some(b".sqlite")));
        assert!(!ends_with(None, Some(b".pyc")));
        assert!(str_contains(Some(b"service.pb.go"), Some(b".pb.")));
        assert!(str_contains(Some(b"service.pb.go"), Some(b"")));
        assert!(!str_contains(Some(b"service.pb.go"), Some(b"mock")));
        assert!(!str_contains(None, Some(b".pb.")));
        assert!(ascii_ieq(Some(b"ExcludesFile"), Some(b"excludesfile")));
        assert!(!ascii_ieq(Some(b"core"), Some(b"cache")));
        assert!(!ascii_ieq(Some(b"\xc0"), Some(b"\xe0")));
        assert!(!ascii_ieq(None, Some(b"core")));
        assert!(has_trailing_sep(Some(b"src/")));
        assert!(has_trailing_sep(Some(b"src\\")));
        assert!(!has_trailing_sep(Some(b"src")));
        assert!(!has_trailing_sep(Some(b"")));
        assert!(!has_trailing_sep(None));
        assert_eq!(path_join(Some(b"/tmp"), Some(b"file"), 64), b"/tmp/file");
        assert_eq!(path_join(Some(b"/tmp/"), Some(b"file"), 64), b"/tmp/file");
        assert_eq!(path_join(None, Some(b"file"), 64), b"file");
        assert_eq!(path_join(Some(b"/tmp"), None, 64), b"/tmp");
        assert_eq!(
            path_join(Some(b"C:\\tmp"), Some(b"file"), 64),
            b"C:/tmp/file"
        );
        assert_eq!(
            path_join(Some(b"c:/tmp"), Some(b"file"), 64),
            b"C:/tmp/file"
        );
        assert_eq!(
            path_join(Some(b"c:\\tmp"), Some(b"file"), 64),
            b"C:/tmp/file"
        );
        assert_eq!(path_join(Some(b"c:"), None, 64), b"C:");
        assert_eq!(
            path_join(Some(b"c:relative"), Some(b"file"), 64),
            b"c:relative/file"
        );
        assert_eq!(path_join(Some(b"c:x"), None, 3), b"C:");
        assert_eq!(path_join(Some(b"/tmp"), Some(b"filename"), 5), b"/tmp");
    }
}

#[cfg(test)]
mod language_marker_tests {
    use super::language_marker;

    #[test]
    fn disambiguation_markers_match_c_contract() {
        assert!(language_marker(Some(b"@interface Foo"), 0));
        assert!(!language_marker(Some(b"int main()"), 0));
        assert!(language_marker(Some(b"end function;"), 1));
        assert!(language_marker(Some(b"intrinsic Foo(x)"), 2));
        assert!(!language_marker(Some(b"intrinsic Foo"), 2));
        assert!(language_marker(Some(b"  classdef Example"), 3));
        assert!(language_marker(Some(b"% comment"), 3));
        assert!(!language_marker(Some(b"%{ block"), 3));
        assert!(!language_marker(None, 0));
    }

    #[test]
    fn 第二類標記僅接受_ascii_可呼叫名稱() {
        assert!(language_marker(Some(b"intrinsic Foo("), 2));
        assert!(!language_marker(Some(b"intrinsic \xE9("), 2));
        assert!(!language_marker(Some(b"intrinsic Foo "), 2));
        assert!(!language_marker(Some(b"intrinsic ("), 2));
    }
}

#[cfg(test)]
mod tests {
    use super::{has_ignored_suffix, matches_fast_pattern, should_skip_dir, should_skip_filename};

    #[test]
    fn should_skip_dir_matches_contract() {
        assert!(should_skip_dir(Some(b".git"), 0));
        assert!(!should_skip_dir(Some(b"src"), 0));
        assert!(should_skip_dir(Some(b"docs"), 2));
        assert!(should_skip_dir(Some(b"docs"), 1));
        assert!(!should_skip_dir(None, 2));
    }

    #[test]
    fn should_skip_filename_only_fast() {
        assert!(should_skip_filename(Some(b"LICENSE"), 2));
        assert!(should_skip_filename(Some(b"LICENSE"), 1));
        assert!(!should_skip_filename(Some(b"main.rs"), 2));
        assert!(!should_skip_filename(None, 2));
    }

    #[test]
    fn fast_patterns_and_suffixes_match() {
        assert!(matches_fast_pattern(Some(b"foo.d.ts"), 2));
        assert!(!matches_fast_pattern(Some(b"foo.rs"), 2));
        assert!(!matches_fast_pattern(Some(b"foo.d.ts"), 0));
        assert!(has_ignored_suffix(Some(b"file.pyc"), 0));
        assert!(has_ignored_suffix(Some(b"bundle.min.js"), 2));
        assert!(has_ignored_suffix(Some(b"bundle.min.js"), 1));
    }
}
