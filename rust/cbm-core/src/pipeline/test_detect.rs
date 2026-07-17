//! Pipeline test file/function classifiers.
//!
//! These helpers mirror `src/pipeline/pass_tests.c` byte-for-byte enough for
//! opt-in dispatch. They do not create TESTS/TESTS_FILE edges or touch graph
//! state.

fn ends_with(bytes: &[u8], suffix: &[u8]) -> bool {
    bytes.len() >= suffix.len() && &bytes[bytes.len() - suffix.len()..] == suffix
}

fn starts_with(bytes: &[u8], prefix: &[u8]) -> bool {
    bytes.len() >= prefix.len() && &bytes[..prefix.len()] == prefix
}

fn contains(bytes: &[u8], needle: &[u8]) -> bool {
    !needle.is_empty() && bytes.windows(needle.len()).any(|window| window == needle)
}

#[must_use]
pub fn node_is_test(properties_json: Option<&[u8]>) -> bool {
    properties_json.is_some_and(|properties| contains(properties, b"\"is_test\":true"))
}

fn basename(path: &[u8]) -> &[u8] {
    match path.iter().rposition(|&byte| byte == b'/') {
        Some(pos) => &path[pos + 1..],
        None => path,
    }
}

fn find_subslice(haystack: &[u8], needle: &[u8]) -> Option<usize> {
    if needle.is_empty() {
        return Some(0);
    }
    haystack
        .windows(needle.len())
        .position(|window| window == needle)
}

fn build_path(path: &[u8], dir_len: usize, name: &[u8], ext: &[u8]) -> Vec<u8> {
    let mut out = Vec::with_capacity(dir_len + name.len() + ext.len() + usize::from(dir_len > 0));
    if dir_len > 0 {
        out.extend_from_slice(&path[..dir_len]);
        out.push(b'/');
    }
    out.extend_from_slice(name);
    out.extend_from_slice(ext);
    out
}

#[must_use]
pub fn is_test_path(path: Option<&[u8]>) -> bool {
    let Some(path) = path else {
        return false;
    };
    let base = basename(path);

    if starts_with(base, b"test_") {
        return true;
    }

    if ends_with(path, b"_test.go")
        || ends_with(path, b"_test.py")
        || ends_with(path, b"_test.rs")
        || ends_with(path, b"_test.cpp")
        || ends_with(path, b"_test.lua")
    {
        return true;
    }

    if contains(path, b".test.ts")
        || contains(path, b".spec.ts")
        || contains(path, b".test.js")
        || contains(path, b".spec.js")
        || contains(path, b".test.tsx")
        || contains(path, b".spec.tsx")
    {
        return true;
    }

    if ends_with(path, b"Test.java")
        || ends_with(path, b"Test.kt")
        || ends_with(path, b"Test.cs")
        || ends_with(path, b"Test.php")
        || ends_with(path, b"Spec.scala")
    {
        return true;
    }

    if contains(path, b"__tests__/")
        || contains(path, b"/tests/")
        || contains(path, b"/test/")
        || contains(path, b"/spec/")
    {
        return true;
    }

    if starts_with(path, b"tests/")
        || starts_with(path, b"test/")
        || starts_with(path, b"spec/")
        || starts_with(path, b"__tests__/")
    {
        return true;
    }

    ends_with(path, b"_spec.rb")
}

fn has_prefix_and_upper_or_end(name: &[u8], prefix: &[u8]) -> bool {
    starts_with(name, prefix)
        && (name.len() == prefix.len() || name[prefix.len()].is_ascii_uppercase())
}

#[must_use]
pub fn is_test_func_name(name: Option<&[u8]>) -> bool {
    let Some(name) = name else {
        return false;
    };

    if has_prefix_and_upper_or_end(name, b"Test")
        || has_prefix_and_upper_or_end(name, b"Benchmark")
        || has_prefix_and_upper_or_end(name, b"Example")
    {
        return true;
    }

    if starts_with(name, b"test_") {
        return true;
    }
    if starts_with(name, b"test") && name.len() > 4 && name[4].is_ascii_uppercase() {
        return true;
    }

    matches!(
        name,
        b"test"
            | b"it"
            | b"describe"
            | b"beforeAll"
            | b"afterAll"
            | b"beforeEach"
            | b"afterEach"
            | b"@testset"
            | b"@test"
    )
}

#[must_use]
pub fn test_to_prod_path(path: Option<&[u8]>) -> Option<Vec<u8>> {
    let path = path?;
    let base = basename(path);
    let dir_len = path.iter().rposition(|&byte| byte == b'/').unwrap_or(0);

    if ends_with(base, b"_test.go") {
        let stem_len = base.len() - b"_test.go".len();
        return Some(build_path(path, dir_len, &base[..stem_len], b".go"));
    }

    if starts_with(base, b"test_") && ends_with(base, b".py") {
        let stem = &base[b"test_".len()..base.len() - b".py".len()];
        return Some(build_path(path, dir_len, stem, b".py"));
    }

    for pat in [b".test.".as_slice(), b".spec.".as_slice()] {
        if let Some(pos) = find_subslice(base, pat) {
            return Some(build_path(path, dir_len, &base[..pos], &base[pos + 5..]));
        }
    }

    None
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn detects_test_paths_like_c_classifier() {
        for path in [
            b"foo_test.go".as_slice(),
            b"test_handler.py",
            b"handler.test.js",
            b"handler.spec.ts",
            b"Component.test.tsx",
            b"OrderTest.java",
            b"handler_test.rs",
            b"handler_test.cpp",
            b"OrderTest.cs",
            b"OrderTest.php",
            b"OrderSpec.scala",
            b"OrderTest.kt",
            b"handler_test.lua",
            b"__tests__/Component.js",
            b"tests/test_main.py",
            b"spec/handler_spec.rb",
            b"src/spec/handler.rb",
            b"src/__tests__/Component.js",
        ] {
            assert!(is_test_path(Some(path)), "{:?}", path);
        }

        for path in [
            b"foo.go".as_slice(),
            b"handler.py",
            b"handler.js",
            b"handler.ts",
            b"Component.tsx",
            b"Order.java",
            b"handler.rs",
            b"handler.cpp",
            b"Order.cs",
            b"Order.php",
            b"Order.scala",
            b"Order.kt",
            b"handler.lua",
            b"contest.go",
            b"latest.py",
        ] {
            assert!(!is_test_path(Some(path)), "{:?}", path);
        }
        assert!(!is_test_path(None));
    }

    #[test]
    fn detects_test_node_property_like_c_classifier() {
        assert!(node_is_test(Some(b"{\"is_test\":true}")));
        assert!(node_is_test(Some(b"{\"name\":\"x\",\"is_test\":true}")));
        assert!(!node_is_test(Some(b"{\"is_test\":false}")));
        assert!(!node_is_test(Some(b"{\"is_test\": true}")));
        assert!(!node_is_test(None));
    }

    #[test]
    fn mirrors_existing_path_edge_cases() {
        assert!(is_test_path(Some(b"handler.test.tsx")));
        assert!(is_test_path(Some(b"handler.spec.tsx")));
        assert!(is_test_path(Some(b"handler.test.ts.snap")));
        assert!(!is_test_path(Some(b"C:\\repo\\test_handler.py")));
        assert!(!is_test_path(Some(b"C:\\repo\\tests\\handler.py")));
        assert!(!is_test_path(Some(b"__tests__")));
        assert!(!is_test_path(Some(b"tests")));
        assert!(!is_test_path(Some(b"Test.java.bak")));
    }

    #[test]
    fn detects_test_function_names_like_c_classifier() {
        for name in [
            b"Test".as_slice(),
            b"TestCreate",
            b"TestHTTPHandler",
            b"Benchmark",
            b"BenchmarkParser",
            b"Example",
            b"ExampleReader",
            b"test_create",
            b"test",
            b"testCreate",
            b"describe",
            b"it",
            b"beforeAll",
            b"afterAll",
            b"beforeEach",
            b"afterEach",
            b"@testset",
            b"@test",
        ] {
            assert!(is_test_func_name(Some(name)), "{:?}", name);
        }

        for name in [
            b"create".as_slice(),
            b"handleRequest",
            b"process",
            b"Testable",
            b"Benchmarkable",
            b"Exampleable",
            b"testable",
            b"describeEach",
        ] {
            assert!(!is_test_func_name(Some(name)), "{:?}", name);
        }
        assert!(!is_test_func_name(None));
    }

    #[test]
    fn maps_test_paths_to_production_paths_like_c_contract() {
        assert_eq!(
            test_to_prod_path(Some(b"foo_test.go")).as_deref(),
            Some(b"foo.go".as_slice())
        );
        assert_eq!(
            test_to_prod_path(Some(b"nested/test_handler.py")).as_deref(),
            Some(b"nested/handler.py".as_slice())
        );
        assert_eq!(
            test_to_prod_path(Some(b"handler.test.ts")).as_deref(),
            Some(b"handler.ts".as_slice())
        );
        assert_eq!(
            test_to_prod_path(Some(b"handler.spec.tsx")).as_deref(),
            Some(b"handler.tsx".as_slice())
        );
        assert_eq!(
            test_to_prod_path(Some(b"handler.test.ts.bak")).as_deref(),
            Some(b"handler.ts.bak".as_slice())
        );
        assert_eq!(test_to_prod_path(Some(b"handler.go")), None);
        assert_eq!(test_to_prod_path(None), None);
    }
}
