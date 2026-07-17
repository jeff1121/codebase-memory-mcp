//! `pass_lsp_cross.c` 的 LSP capability 與 TypeScript mode classifiers。

pub const LANG_GO: i32 = 0;
pub const LANG_PYTHON: i32 = 1;
pub const LANG_JAVASCRIPT: i32 = 2;
pub const LANG_TYPESCRIPT: i32 = 3;
pub const LANG_TSX: i32 = 4;
pub const LANG_RUST: i32 = 5;
pub const LANG_JAVA: i32 = 6;
pub const LANG_CPP: i32 = 7;
pub const LANG_CSHARP: i32 = 8;
pub const LANG_PHP: i32 = 9;
pub const LANG_KOTLIN: i32 = 12;
pub const LANG_C: i32 = 14;
pub const LANG_CUDA: i32 = 43;

pub const MODE_JS: i32 = 1;
pub const MODE_JSX: i32 = 2;
pub const MODE_DTS: i32 = 4;

#[must_use]
pub fn ts_modes(language: i32, rel_path: Option<&[u8]>) -> i32 {
    let mut modes = 0;
    if language == LANG_JAVASCRIPT {
        modes |= MODE_JS;
    }
    if language == LANG_TSX {
        modes |= MODE_JSX;
    }
    if let Some(path) = rel_path {
        if language == LANG_JAVASCRIPT && path.ends_with(b".jsx") {
            modes |= MODE_JSX;
        }
        if language == LANG_TYPESCRIPT && path.ends_with(b".d.ts") {
            modes |= MODE_DTS;
        }
    }
    modes
}

#[must_use]
pub fn has_cross_lsp(language: i32) -> bool {
    matches!(
        language,
        LANG_GO
            | LANG_C
            | LANG_CPP
            | LANG_CUDA
            | LANG_PYTHON
            | LANG_JAVASCRIPT
            | LANG_TYPESCRIPT
            | LANG_TSX
            | LANG_PHP
            | LANG_CSHARP
            | LANG_JAVA
            | LANG_KOTLIN
            | LANG_RUST
    )
}

#[must_use]
pub fn map_label_allowed(label: Option<&[u8]>) -> bool {
    let Some(label) = label else {
        return false;
    };
    matches!(
        label,
        b"Class"
            | b"Struct"
            | b"Interface"
            | b"Enum"
            | b"Type"
            | b"Trait"
            | b"Protocol"
            | b"Function"
            | b"Method"
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn computes_typescript_modes_for_language_path_matrix() {
        assert_eq!(ts_modes(LANG_JAVASCRIPT, None), MODE_JS);
        assert_eq!(ts_modes(LANG_JAVASCRIPT, Some(b"src/app.js")), MODE_JS);
        assert_eq!(
            ts_modes(LANG_JAVASCRIPT, Some(b"src/app.jsx")),
            MODE_JS | MODE_JSX
        );
        assert_eq!(ts_modes(LANG_JAVASCRIPT, Some(b"src/app.JSX")), MODE_JS);
        assert_eq!(ts_modes(LANG_TYPESCRIPT, None), 0);
        assert_eq!(ts_modes(LANG_TYPESCRIPT, Some(b"src/types.d.ts")), MODE_DTS);
        assert_eq!(ts_modes(LANG_TYPESCRIPT, Some(b"src/types.ts")), 0);
        assert_eq!(ts_modes(LANG_TSX, None), MODE_JSX);
        assert_eq!(ts_modes(LANG_TSX, Some(b"src/app.tsx")), MODE_JSX);
        assert_eq!(ts_modes(LANG_TSX, Some(b"src/types.d.ts")), MODE_JSX);
    }

    #[test]
    fn matches_all_lsp_supported_languages_and_rejects_unknown_values() {
        for (language, expected) in [
            (LANG_GO, 0),
            (LANG_PYTHON, 1),
            (LANG_JAVASCRIPT, 2),
            (LANG_TYPESCRIPT, 3),
            (LANG_TSX, 4),
            (LANG_RUST, 5),
            (LANG_JAVA, 6),
            (LANG_CPP, 7),
            (LANG_CSHARP, 8),
            (LANG_PHP, 9),
            (LANG_KOTLIN, 12),
            (LANG_C, 14),
            (LANG_CUDA, 43),
        ] {
            assert_eq!(language, expected);
            assert!(has_cross_lsp(language));
        }

        for language in [-1, 10, 11, 13, 15, 31, i32::MAX] {
            assert!(!has_cross_lsp(language));
        }
    }

    #[test]
    fn filters_lsp_definition_labels_by_exact_raw_bytes() {
        for label in [
            b"Class".as_slice(),
            b"Struct".as_slice(),
            b"Interface".as_slice(),
            b"Enum".as_slice(),
            b"Type".as_slice(),
            b"Trait".as_slice(),
            b"Protocol".as_slice(),
            b"Function".as_slice(),
            b"Method".as_slice(),
        ] {
            assert!(map_label_allowed(Some(label)));
        }

        assert!(!map_label_allowed(None));
        for label in [
            b"".as_slice(),
            b"Variable".as_slice(),
            b"struct".as_slice(),
            b"STRUCT".as_slice(),
            b"\xff".as_slice(),
            b"Struct\0Variable".as_slice(),
        ] {
            assert!(!map_label_allowed(Some(label)));
        }
    }
}
