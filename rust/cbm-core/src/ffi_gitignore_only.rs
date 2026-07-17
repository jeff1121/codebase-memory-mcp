#![allow(unsafe_code)]

use core::ffi::{c_char, c_int, c_void};
use core::ptr;
use std::ffi::CStr;

struct GitIgnorePattern {
    pattern: Vec<u8>,
    negated: bool,
    dir_only: bool,
    rooted: bool,
}

pub struct CbmGitignore {
    patterns: Vec<GitIgnorePattern>,
}

unsafe extern "C" {
    fn fopen(path: *const c_char, mode: *const c_char) -> *mut c_void;
    fn fseek(stream: *mut c_void, offset: i64, origin: c_int) -> c_int;
    fn ftell(stream: *mut c_void) -> i64;
    fn fread(buffer: *mut c_void, size: usize, count: usize, stream: *mut c_void) -> usize;
    fn fclose(stream: *mut c_void) -> c_int;
    fn free(pointer: *mut c_void);
}

#[no_mangle]
pub static mut cbm_gitignore_merge_dup_hook_for_test: Option<
    unsafe extern "C" fn(*const c_char) -> *mut c_char,
> = None;

unsafe fn c_bytes<'a>(value: *const c_char) -> &'a [u8] {
    if value.is_null() {
        &[]
    } else {
        CStr::from_ptr(value).to_bytes()
    }
}

fn add_pattern(patterns: &mut Vec<GitIgnorePattern>, line: &[u8]) -> bool {
    let mut len = line.len();
    while len > 0 && matches!(line[len - 1], b' ' | b'\t' | b'\r') {
        len -= 1;
    }
    if len == 0 {
        return true;
    }

    let mut start = 0;
    let mut negated = false;
    if line[start] == b'!' {
        negated = true;
        start += 1;
        len -= 1;
    }
    if len == 0 {
        return true;
    }

    let mut dir_only = false;
    if line[start + len - 1] == b'/' {
        dir_only = true;
        len -= 1;
    }
    if len == 0 {
        return true;
    }

    let mut rooted = false;
    if line[start] == b'/' {
        rooted = true;
        start += 1;
        len -= 1;
    }
    if len == 0 {
        return true;
    }

    if !rooted {
        rooted = line[start..start + len].contains(&b'/');
    }

    let mut pattern = Vec::new();
    if pattern.try_reserve(len).is_err() {
        return false;
    }
    pattern.extend_from_slice(&line[start..start + len]);

    if patterns.try_reserve(1).is_err() {
        return false;
    }
    patterns.push(GitIgnorePattern {
        pattern,
        negated,
        dir_only,
        rooted,
    });
    true
}

fn parse_bytes(content: &[u8]) -> Option<Box<CbmGitignore>> {
    let mut patterns = Vec::new();
    let mut line_start = 0;
    while line_start < content.len() {
        let line_end = content[line_start..]
            .iter()
            .position(|byte| *byte == b'\n')
            .map(|offset| line_start + offset);
        let end = line_end.unwrap_or(content.len());
        let line = &content[line_start..end];
        if line.first() != Some(&b'#') && !add_pattern(&mut patterns, line) {
            return None;
        }
        let Some(end) = line_end else {
            break;
        };
        line_start = end + 1;
    }
    Some(Box::new(CbmGitignore { patterns }))
}

fn glob_match_charclass(pattern: &[u8], character: u8) -> (bool, usize) {
    let mut index = 0;
    let mut negate = false;
    if pattern.get(index) == Some(&b'!') || pattern.get(index) == Some(&b'^') {
        negate = true;
        index += 1;
    }
    let mut matched = false;
    let mut previous = 0;
    while index < pattern.len() && pattern[index] != b']' {
        if pattern[index] == b'-'
            && previous != 0
            && index + 1 < pattern.len()
            && pattern[index + 1] != b']'
        {
            index += 1;
            if character >= previous && character <= pattern[index] {
                matched = true;
            }
            previous = pattern[index];
            index += 1;
        } else {
            if character == pattern[index] {
                matched = true;
            }
            previous = pattern[index];
            index += 1;
        }
    }
    if pattern.get(index) == Some(&b']') {
        index += 1;
    }
    (if negate { !matched } else { matched }, index)
}

fn glob_match_doublestar_slash(pattern: &[u8], value: &[u8]) -> bool {
    if glob_match(pattern, value) {
        return true;
    }
    for (index, byte) in value.iter().enumerate() {
        if *byte == b'/' && glob_match(pattern, &value[index + 1..]) {
            return true;
        }
    }
    false
}

fn glob_match_doublestar_any(pattern: &[u8], value: &[u8]) -> bool {
    for index in 0..=value.len() {
        if glob_match(pattern, &value[index..]) {
            return true;
        }
    }
    false
}

fn glob_match_star(pattern: &[u8], value: &[u8]) -> bool {
    for index in 0..=value.len() {
        if glob_match(pattern, &value[index..]) {
            return true;
        }
        if index == value.len() || value[index] == b'/' {
            return false;
        }
    }
    false
}

fn glob_match_doublestar(pattern: &[u8], value: &[u8]) -> bool {
    match pattern.get(2) {
        Some(b'/') => glob_match_doublestar_slash(&pattern[3..], value),
        None => true,
        Some(_) => glob_match_doublestar_any(&pattern[2..], value),
    }
}

fn glob_match(pattern: &[u8], value: &[u8]) -> bool {
    let mut pattern_index = 0;
    let mut value_index = 0;
    while pattern_index < pattern.len() && value_index < value.len() {
        if pattern[pattern_index] == b'*' && pattern.get(pattern_index + 1) == Some(&b'*') {
            return glob_match_doublestar(&pattern[pattern_index..], &value[value_index..]);
        }
        if pattern[pattern_index] == b'*' {
            return glob_match_star(&pattern[pattern_index + 1..], &value[value_index..]);
        }
        if pattern[pattern_index] == b'?' {
            if value[value_index] == b'/' {
                return false;
            }
            pattern_index += 1;
            value_index += 1;
            continue;
        }
        if pattern[pattern_index] == b'[' {
            let (matched, consumed) =
                glob_match_charclass(&pattern[pattern_index + 1..], value[value_index]);
            if !matched {
                return false;
            }
            pattern_index += consumed + 1;
            value_index += 1;
            continue;
        }
        if pattern[pattern_index] != value[value_index] {
            return false;
        }
        pattern_index += 1;
        value_index += 1;
    }
    while pattern.get(pattern_index) == Some(&b'*') {
        pattern_index += 1;
    }
    pattern_index == pattern.len() && value_index == value.len()
}

fn match_unrooted(pattern: &[u8], rel_path: &[u8]) -> bool {
    let basename_start = rel_path
        .iter()
        .rposition(|byte| *byte == b'/')
        .map(|index| index + 1)
        .unwrap_or(0);
    if glob_match(pattern, &rel_path[basename_start..]) {
        return true;
    }
    if basename_start == 0 {
        return false;
    }
    let mut start = 0;
    loop {
        if glob_match(pattern, &rel_path[start..]) {
            return true;
        }
        let Some(offset) = rel_path[start..].iter().position(|byte| *byte == b'/') else {
            break;
        };
        start += offset + 1;
    }
    false
}

fn match_result(matcher: &CbmGitignore, rel_path: &[u8], is_dir: bool) -> c_int {
    let mut result = 0;
    for pattern in &matcher.patterns {
        if pattern.dir_only && !is_dir {
            continue;
        }
        let matched = if pattern.rooted {
            glob_match(&pattern.pattern, rel_path)
        } else {
            match_unrooted(&pattern.pattern, rel_path)
        };
        if matched {
            result = if pattern.negated { -1 } else { 1 };
        }
    }
    result
}

#[no_mangle]
/// 從 NUL 結尾字串解析 gitignore pattern。
///
/// # Safety
/// `content` 必須是可讀取且以 NUL 結尾的 C 字串，或為 null。
pub unsafe extern "C" fn cbm_gitignore_parse(content: *const c_char) -> *mut CbmGitignore {
    if content.is_null() {
        return ptr::null_mut();
    }
    parse_bytes(c_bytes(content))
        .map(Box::into_raw)
        .unwrap_or(ptr::null_mut())
}

#[no_mangle]
/// 從檔案載入 gitignore pattern。
///
/// # Safety
/// `path` 必須是可讀取且以 NUL 結尾的 C 字串，或為 null。
pub unsafe extern "C" fn cbm_gitignore_load(path: *const c_char) -> *mut CbmGitignore {
    if path.is_null() {
        return ptr::null_mut();
    }
    let stream = fopen(path, c"r".as_ptr());
    if stream.is_null() {
        return ptr::null_mut();
    }
    let _ = fseek(stream, 0, 2);
    let size = ftell(stream);
    let _ = fseek(stream, 0, 0);
    if size <= 0 {
        let _ = fclose(stream);
        return parse_bytes(&[])
            .map(Box::into_raw)
            .unwrap_or(ptr::null_mut());
    }
    let mut content = Vec::new();
    if content.try_reserve(size as usize).is_err() {
        let _ = fclose(stream);
        return ptr::null_mut();
    }
    content.resize(size as usize, 0);
    let length = fread(
        content.as_mut_ptr().cast::<c_void>(),
        1,
        content.len(),
        stream,
    );
    let _ = fclose(stream);
    content.truncate(length);
    parse_bytes(&content)
        .map(Box::into_raw)
        .unwrap_or(ptr::null_mut())
}

#[no_mangle]
/// 判斷相對路徑是否被 gitignore 規則忽略。
///
/// # Safety
/// `matcher` 與 `rel_path` 必須是有效指標；`rel_path` 必須指向 NUL 結尾字串，或為 null。
pub unsafe extern "C" fn cbm_gitignore_matches(
    matcher: *const CbmGitignore,
    rel_path: *const c_char,
    is_dir: bool,
) -> bool {
    if matcher.is_null() || rel_path.is_null() {
        return false;
    }
    match_result(&*matcher, c_bytes(rel_path), is_dir) > 0
}

#[no_mangle]
/// 回傳 gitignore 的三態結果：未命中為 0、忽略為 1、否定規則為 -1。
///
/// # Safety
/// `matcher` 與 `rel_path` 必須是有效指標；`rel_path` 必須指向 NUL 結尾字串。
pub unsafe extern "C" fn cbm_gitignore_match_result(
    matcher: *const CbmGitignore,
    rel_path: *const c_char,
    is_dir: bool,
) -> c_int {
    if matcher.is_null() || rel_path.is_null() {
        return 0;
    }
    match_result(&*matcher, c_bytes(rel_path), is_dir)
}

#[no_mangle]
/// 釋放 Rust 建立的 gitignore matcher。
///
/// # Safety
/// `matcher` 必須是本模組回傳且尚未釋放的指標，或為 null。
pub unsafe extern "C" fn cbm_gitignore_free(matcher: *mut CbmGitignore) {
    if !matcher.is_null() {
        drop(Box::from_raw(matcher));
    }
}

unsafe fn duplicate_for_merge(pattern: &[u8]) -> Option<Vec<u8>> {
    let hook = cbm_gitignore_merge_dup_hook_for_test;
    if let Some(hook) = hook {
        let mut input = Vec::new();
        input.try_reserve(pattern.len() + 1).ok()?;
        input.extend_from_slice(pattern);
        input.push(0);
        let copied = hook(input.as_ptr().cast::<c_char>());
        if copied.is_null() {
            return None;
        }
        let result = CStr::from_ptr(copied).to_bytes().to_vec();
        free(copied.cast::<c_void>());
        Some(result)
    } else {
        Some(pattern.to_vec())
    }
}

#[no_mangle]
/// 將來源 matcher 的 pattern 原子地複製到目的 matcher。
///
/// # Safety
/// `dst` 必須是本模組建立且可寫的 matcher；`src` 必須是有效 matcher 或 null。
pub unsafe extern "C" fn cbm_gitignore_merge(
    dst: *mut CbmGitignore,
    src: *const CbmGitignore,
) -> bool {
    if dst.is_null() {
        return false;
    }
    if src.is_null() || (*src).patterns.is_empty() {
        return true;
    }
    let source = &*src;
    let mut copies = Vec::new();
    if copies.try_reserve(source.patterns.len()).is_err() {
        return false;
    }
    for pattern in &source.patterns {
        let copied = match duplicate_for_merge(&pattern.pattern) {
            Some(value) => value,
            None => return false,
        };
        copies.push(GitIgnorePattern {
            pattern: copied,
            negated: pattern.negated,
            dir_only: pattern.dir_only,
            rooted: pattern.rooted,
        });
    }
    let destination = &mut *dst;
    if destination.patterns.try_reserve(copies.len()).is_err() {
        return false;
    }
    destination.patterns.extend(copies);
    true
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn matches_glob_negation_directory_and_classes() {
        let matcher = parse_bytes(b"*.log\n!important.log\nbuild/\nfoo/[a-c].rs\n").unwrap();
        assert_eq!(match_result(&matcher, b"error.log", false), 1);
        assert_eq!(match_result(&matcher, b"important.log", false), -1);
        assert_eq!(match_result(&matcher, b"build", true), 1);
        assert_eq!(match_result(&matcher, b"foo/b.rs", false), 1);
        assert_eq!(match_result(&matcher, b"foo/z.rs", false), 0);
    }

    #[test]
    fn matches_rooted_and_double_star_patterns() {
        let matcher = parse_bytes(b"/target/\n**/*.gen\n").unwrap();
        assert_eq!(match_result(&matcher, b"target", true), 1);
        assert_eq!(match_result(&matcher, b"src/target", true), 0);
        assert_eq!(match_result(&matcher, b"a/b/file.gen", false), 1);
    }
}
