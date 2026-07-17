//! `pass_configlink.c` 的 manifest/dependency 純判斷 helper。

const MANIFESTS: [&[u8]; 8] = [
    b"Cargo.toml",
    b"package.json",
    b"go.mod",
    b"requirements.txt",
    b"Gemfile",
    b"build.gradle",
    b"pom.xml",
    b"composer.json",
];
const DEPENDENCY_SECTIONS: [&[u8]; 5] = [
    b"dependencies",
    b"devdependencies",
    b"peerdependencies",
    b"dev-dependencies",
    b"build-dependencies",
];

#[must_use]
pub fn is_manifest_file(basename: Option<&[u8]>) -> bool {
    basename.is_some_and(|value| MANIFESTS.contains(&value))
}

#[must_use]
pub fn is_dep_section(value: Option<&[u8]>) -> bool {
    value.is_some_and(|value| {
        DEPENDENCY_SECTIONS
            .iter()
            .any(|section| contains_ascii_case_insensitive(value, section))
    })
}

#[must_use]
pub fn is_cargo_dep_section(value: Option<&[u8]>) -> bool {
    let Some(value) = value else {
        return false;
    };
    value[..value.len().min(511)]
        .split(|byte| *byte == b'.')
        .any(|part| {
            let part = &part[..part.len().min(127)];
            DEPENDENCY_SECTIONS
                .iter()
                .any(|section| ascii_lower(part).as_slice() == *section)
        })
}

#[must_use]
pub fn match_dep_to_import(
    target_name: Option<&[u8]>,
    target_qn: Option<&[u8]>,
    dep_lower: Option<&[u8]>,
) -> f64 {
    let (Some(target_name), Some(dep_lower)) = (target_name, dep_lower) else {
        return 0.0;
    };
    if ascii_lower(&target_name[..target_name.len().min(255)]).as_slice() == dep_lower {
        return 0.95;
    }
    if target_qn
        .is_some_and(|qn| contains_ascii_case_insensitive(&qn[..qn.len().min(511)], dep_lower))
    {
        return 0.80;
    }
    0.0
}

#[must_use]
pub fn lowercase_into(value: Option<&[u8]>, output_size: usize) -> Vec<u8> {
    let Some(value) = value else {
        return Vec::new();
    };
    if output_size == 0 {
        return Vec::new();
    }
    value[..value.len().min(output_size - 1)]
        .iter()
        .map(u8::to_ascii_lowercase)
        .collect()
}

fn ascii_lower(value: &[u8]) -> Vec<u8> {
    value.iter().map(u8::to_ascii_lowercase).collect()
}

fn contains_ascii_case_insensitive(haystack: &[u8], needle: &[u8]) -> bool {
    needle.is_empty()
        || haystack
            .windows(needle.len())
            .any(|window| ascii_lower(window).as_slice() == needle)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn classifies_manifests_and_dependency_sections() {
        assert!(is_manifest_file(Some(b"Cargo.toml")));
        assert!(!is_manifest_file(Some(b"main.rs")));
        assert!(is_dep_section(Some(b"devDependencies")));
        assert!(!is_dep_section(Some(b"scripts")));
        assert!(is_cargo_dep_section(Some(b"crate.dependencies")));
    }

    #[test]
    fn scores_dependency_import_matches() {
        assert_eq!(
            match_dep_to_import(Some(b"serde"), None, Some(b"serde")),
            0.95
        );
        assert_eq!(
            match_dep_to_import(
                Some(b"SerdeCore"),
                Some(b"crate.serde.core"),
                Some(b"serde")
            ),
            0.80
        );
        assert_eq!(
            match_dep_to_import(Some(b"tokio"), Some(b"crate.io"), Some(b"serde")),
            0.0
        );
        assert_eq!(lowercase_into(Some(b"SerdeCore"), 5), b"serd");
    }
}
