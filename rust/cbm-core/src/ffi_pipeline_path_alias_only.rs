#![allow(unsafe_code)]

//! `src/pipeline/path_alias.c` 的完整 Rust 實作。

use core::ffi::{c_char, c_int, c_void};
use std::ffi::CStr;
#[cfg(not(test))]
use std::ffi::CString;
use std::fs::{self, File};
use std::io::Read;
use std::mem::size_of;
use std::path::{Path, PathBuf};
use std::ptr;

const MAX_ENTRIES: usize = 256;
const MAX_FILES: usize = 256;
const MAX_FILE_BYTES: u64 = 64 * 1024;
const MAX_DEPTH: usize = 32;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CbmPathAlias {
    pub alias_prefix: *mut c_char,
    pub alias_suffix: *mut c_char,
    pub target_prefix: *mut c_char,
    pub target_suffix: *mut c_char,
    pub has_wildcard: bool,
}

#[repr(C)]
pub struct CbmPathAliasMap {
    pub entries: *mut CbmPathAlias,
    pub count: c_int,
    pub base_url: *mut c_char,
}

#[repr(C)]
pub struct CbmPathAliasScope {
    pub dir_prefix: *mut c_char,
    pub map: *mut CbmPathAliasMap,
}

#[repr(C)]
pub struct CbmPathAliasCollection {
    pub scopes: *mut CbmPathAliasScope,
    pub count: c_int,
}

#[cfg(not(test))]
unsafe extern "C" {
    fn cbm_rs_path_alias_warn(message: *const c_char, key: *const c_char, value: *const c_char);
}

unsafe extern "C" {
    fn malloc(size: usize) -> *mut c_void;
    fn free(ptr: *mut c_void);
}

fn c_alloc_bytes(size: usize) -> *mut u8 {
    if size == 0 {
        return ptr::null_mut();
    }
    // SAFETY: malloc is called with the requested byte count; the caller owns the result.
    let raw = unsafe { malloc(size) as *mut u8 };
    if !raw.is_null() {
        // SAFETY: malloc returned at least `size` writable bytes.
        unsafe { ptr::write_bytes(raw, 0, size) };
    }
    raw
}

fn c_alloc<T>() -> *mut T {
    c_alloc_bytes(size_of::<T>()).cast()
}

fn c_alloc_array<T>(count: usize) -> *mut T {
    count
        .checked_mul(size_of::<T>())
        .map_or(ptr::null_mut(), |size| c_alloc_bytes(size).cast())
}

unsafe fn c_free<T>(raw: *mut T) {
    if !raw.is_null() {
        // SAFETY: every pointer passed here came from C malloc-compatible allocation.
        unsafe { free(raw.cast()) };
    }
}

fn c_string(bytes: &[u8]) -> *mut c_char {
    let end = bytes
        .iter()
        .position(|byte| *byte == 0)
        .unwrap_or(bytes.len());
    let raw = c_alloc_bytes(end + 1);
    if raw.is_null() {
        return ptr::null_mut();
    }
    // SAFETY: raw has end + 1 bytes and the source is valid for end bytes.
    unsafe {
        ptr::copy_nonoverlapping(bytes.as_ptr(), raw, end);
        *raw.add(end) = 0;
    }
    raw.cast()
}

unsafe fn c_bytes<'a>(raw: *const c_char) -> Option<&'a [u8]> {
    if raw.is_null() {
        None
    } else {
        // SAFETY: C ABI callers provide NUL-terminated strings.
        Some(unsafe { CStr::from_ptr(raw) }.to_bytes())
    }
}

#[cfg(not(test))]
fn bytes_from_path(path: &Path) -> Vec<u8> {
    path.to_string_lossy().as_bytes().to_vec()
}

fn warn_cap(message: &str, key: &str, value: &Path) {
    #[cfg(not(test))]
    {
        let Ok(message) = CString::new(message) else {
            return;
        };
        let Ok(key) = CString::new(key) else {
            return;
        };
        let value = bytes_from_path(value);
        let Ok(value) = CString::new(value) else {
            return;
        };
        // SAFETY: all temporary C strings live through the non-variadic bridge call.
        unsafe { cbm_rs_path_alias_warn(message.as_ptr(), key.as_ptr(), value.as_ptr()) };
    }
    #[cfg(test)]
    {
        let _ = (message, key, value);
    }
}

fn strip_resolved_ext(mut path: Vec<u8>) -> Vec<u8> {
    if path.len() > 3 {
        let suffix = &path[path.len() - 3..];
        if suffix == b".ts" || suffix == b".js" {
            path.truncate(path.len() - 3);
            return path;
        }
    }
    if path.len() > 4 {
        let suffix = &path[path.len() - 4..];
        if suffix == b".tsx" || suffix == b".jsx" {
            path.truncate(path.len() - 4);
        }
    }
    path
}

fn resolve_target_relative(dir_prefix: &[u8], target: &[u8]) -> Vec<u8> {
    let target = target.strip_prefix(b"./").unwrap_or(target);
    if dir_prefix.is_empty() {
        return target.to_vec();
    }
    let mut result = Vec::with_capacity(dir_prefix.len() + 1 + target.len());
    result.extend_from_slice(dir_prefix);
    result.push(b'/');
    result.extend_from_slice(target);
    result
}

#[derive(Debug)]
enum JsonValue {
    Object(Vec<(Vec<u8>, JsonValue)>),
    Array(Vec<JsonValue>),
    String(Vec<u8>),
    Other,
}

struct JsonParser<'a> {
    input: &'a [u8],
    pos: usize,
}

impl<'a> JsonParser<'a> {
    fn new(input: &'a [u8]) -> Self {
        Self { input, pos: 0 }
    }

    fn parse(mut self) -> Option<JsonValue> {
        let value = self.value()?;
        self.space()?;
        (self.pos == self.input.len()).then_some(value)
    }

    fn space(&mut self) -> Option<()> {
        loop {
            while self
                .input
                .get(self.pos)
                .is_some_and(|byte| byte.is_ascii_whitespace())
            {
                self.pos += 1;
            }
            if self.input.get(self.pos..self.pos + 2) == Some(b"//") {
                self.pos += 2;
                while self.pos < self.input.len() && self.input[self.pos] != b'\n' {
                    self.pos += 1;
                }
                continue;
            }
            if self.input.get(self.pos..self.pos + 2) == Some(b"/*") {
                self.pos += 2;
                let remaining = &self.input[self.pos..];
                let end = remaining.windows(2).position(|window| window == b"*/")?;
                self.pos += end + 2;
                continue;
            }
            return Some(());
        }
    }

    fn value(&mut self) -> Option<JsonValue> {
        self.space()?;
        match self.input.get(self.pos).copied()? {
            b'{' => self.object(),
            b'[' => self.array(),
            b'"' => self.string().map(JsonValue::String),
            b't' | b'f' | b'n' | b'-' | b'0'..=b'9' => self.other().map(|()| JsonValue::Other),
            _ => None,
        }
    }

    fn object(&mut self) -> Option<JsonValue> {
        self.pos += 1;
        let mut fields = Vec::new();
        self.space()?;
        if self.input.get(self.pos) == Some(&b'}') {
            self.pos += 1;
            return Some(JsonValue::Object(fields));
        }
        loop {
            self.space()?;
            if self.input.get(self.pos) != Some(&b'"') {
                return None;
            }
            let key = self.string()?;
            self.space()?;
            if self.input.get(self.pos) != Some(&b':') {
                return None;
            }
            self.pos += 1;
            let value = self.value()?;
            fields.push((key, value));
            self.space()?;
            match self.input.get(self.pos).copied()? {
                b'}' => {
                    self.pos += 1;
                    return Some(JsonValue::Object(fields));
                }
                b',' => {
                    self.pos += 1;
                    self.space()?;
                    if self.input.get(self.pos) == Some(&b'}') {
                        self.pos += 1;
                        return Some(JsonValue::Object(fields));
                    }
                }
                _ => return None,
            }
        }
    }

    fn array(&mut self) -> Option<JsonValue> {
        self.pos += 1;
        let mut values = Vec::new();
        self.space()?;
        if self.input.get(self.pos) == Some(&b']') {
            self.pos += 1;
            return Some(JsonValue::Array(values));
        }
        loop {
            values.push(self.value()?);
            self.space()?;
            match self.input.get(self.pos).copied()? {
                b']' => {
                    self.pos += 1;
                    return Some(JsonValue::Array(values));
                }
                b',' => {
                    self.pos += 1;
                    self.space()?;
                    if self.input.get(self.pos) == Some(&b']') {
                        self.pos += 1;
                        return Some(JsonValue::Array(values));
                    }
                }
                _ => return None,
            }
        }
    }

    fn string(&mut self) -> Option<Vec<u8>> {
        if self.input.get(self.pos) != Some(&b'"') {
            return None;
        }
        self.pos += 1;
        let mut result = Vec::new();
        while let Some(byte) = self.input.get(self.pos).copied() {
            self.pos += 1;
            match byte {
                b'"' => return Some(result),
                b'\\' => self.escape(&mut result)?,
                0..=0x1f => return None,
                _ => result.push(byte),
            }
        }
        None
    }

    fn escape(&mut self, result: &mut Vec<u8>) -> Option<()> {
        match self.input.get(self.pos).copied()? {
            b'"' | b'\\' | b'/' => result.push(self.input[self.pos]),
            b'b' => result.push(0x08),
            b'f' => result.push(0x0c),
            b'n' => result.push(b'\n'),
            b'r' => result.push(b'\r'),
            b't' => result.push(b'\t'),
            b'u' => {
                let first = self.hex_quad(self.pos + 1)?;
                self.pos += 5;
                let code = if (0xd800..=0xdbff).contains(&first) {
                    if self.input.get(self.pos..self.pos + 2) != Some(b"\\u") {
                        return None;
                    }
                    let second = self.hex_quad(self.pos + 2)?;
                    if !(0xdc00..=0xdfff).contains(&second) {
                        return None;
                    }
                    self.pos += 6;
                    0x10000 + ((first - 0xd800) << 10) + (second - 0xdc00)
                } else {
                    if (0xdc00..=0xdfff).contains(&first) {
                        return None;
                    }
                    first
                };
                append_codepoint(result, code)?;
                return Some(());
            }
            _ => return None,
        }
        self.pos += 1;
        Some(())
    }

    fn hex_quad(&self, start: usize) -> Option<u32> {
        let digits = self.input.get(start..start + 4)?;
        let mut value: u32 = 0;
        for digit in digits {
            value = value.checked_mul(16)?.checked_add(hex_digit(*digit)?)?;
        }
        Some(value)
    }

    fn other(&mut self) -> Option<()> {
        let start = self.pos;
        if self.input[start..].starts_with(b"true") {
            self.pos += 4;
            return self.token_end().then_some(());
        }
        if self.input[start..].starts_with(b"false") {
            self.pos += 5;
            return self.token_end().then_some(());
        }
        if self.input[start..].starts_with(b"null") {
            self.pos += 4;
            return self.token_end().then_some(());
        }
        if self.input.get(self.pos) == Some(&b'-') {
            self.pos += 1;
        }
        match self.input.get(self.pos) {
            Some(b'0') => self.pos += 1,
            Some(b'1'..=b'9') => {
                self.pos += 1;
                while self
                    .input
                    .get(self.pos)
                    .is_some_and(|byte| byte.is_ascii_digit())
                {
                    self.pos += 1;
                }
            }
            _ => return None,
        }
        if self.input.get(self.pos) == Some(&b'.') {
            self.pos += 1;
            let start_fraction = self.pos;
            while self
                .input
                .get(self.pos)
                .is_some_and(|byte| byte.is_ascii_digit())
            {
                self.pos += 1;
            }
            if self.pos == start_fraction {
                return None;
            }
        }
        if self
            .input
            .get(self.pos)
            .is_some_and(|byte| *byte == b'e' || *byte == b'E')
        {
            self.pos += 1;
            if self
                .input
                .get(self.pos)
                .is_some_and(|byte| *byte == b'+' || *byte == b'-')
            {
                self.pos += 1;
            }
            let start_exponent = self.pos;
            while self
                .input
                .get(self.pos)
                .is_some_and(|byte| byte.is_ascii_digit())
            {
                self.pos += 1;
            }
            if self.pos == start_exponent {
                return None;
            }
        }
        self.token_end().then_some(())
    }

    fn token_end(&self) -> bool {
        match self.input.get(self.pos).copied() {
            None | Some(b',') | Some(b']') | Some(b'}') => true,
            Some(byte) if byte.is_ascii_whitespace() => true,
            Some(b'/') if self.input.get(self.pos..self.pos + 2) == Some(b"//") => true,
            Some(b'/') if self.input.get(self.pos..self.pos + 2) == Some(b"/*") => true,
            _ => false,
        }
    }
}

fn hex_digit(byte: u8) -> Option<u32> {
    match byte {
        b'0'..=b'9' => Some(u32::from(byte - b'0')),
        b'a'..=b'f' => Some(u32::from(byte - b'a' + 10)),
        b'A'..=b'F' => Some(u32::from(byte - b'A' + 10)),
        _ => None,
    }
}

fn append_codepoint(out: &mut Vec<u8>, code: u32) -> Option<()> {
    let character = char::from_u32(code)?;
    let mut encoded = [0u8; 4];
    let encoded = character.encode_utf8(&mut encoded);
    out.extend_from_slice(encoded.as_bytes());
    Some(())
}

fn object_field<'a>(value: &'a JsonValue, name: &[u8]) -> Option<&'a JsonValue> {
    let JsonValue::Object(fields) = value else {
        return None;
    };
    fields
        .iter()
        .find(|(key, _)| key.as_slice() == name)
        .map(|(_, value)| value)
}

fn string_value(value: &JsonValue) -> Option<&[u8]> {
    match value {
        JsonValue::String(value) => Some(value),
        _ => None,
    }
}

unsafe fn free_map(map: *mut CbmPathAliasMap) {
    if map.is_null() {
        return;
    }
    let count = unsafe { (*map).count.max(0) as usize };
    let entries = unsafe { (*map).entries };
    if !entries.is_null() {
        for index in 0..count {
            // SAFETY: the map owns `count` initialized entries.
            let entry = unsafe { &mut *entries.add(index) };
            unsafe {
                c_free(entry.alias_prefix);
                c_free(entry.alias_suffix);
                c_free(entry.target_prefix);
                c_free(entry.target_suffix);
            }
        }
        unsafe { c_free(entries) };
    }
    unsafe {
        c_free((*map).base_url);
        c_free(map);
    }
}

unsafe fn free_collection(coll: *mut CbmPathAliasCollection) {
    if coll.is_null() {
        return;
    }
    let count = unsafe { (*coll).count.max(0) as usize };
    let scopes = unsafe { (*coll).scopes };
    if !scopes.is_null() {
        for index in 0..count {
            // SAFETY: the collection owns `count` initialized scopes.
            let scope = unsafe { &mut *scopes.add(index) };
            unsafe {
                c_free(scope.dir_prefix);
                free_map(scope.map);
            }
        }
        unsafe { c_free(scopes) };
    }
    unsafe { c_free(coll) };
}

fn load_tsconfig_file(abs_path: &Path, dir_prefix: &[u8]) -> Option<*mut CbmPathAliasMap> {
    let mut file = File::open(abs_path).ok()?;
    let size = file.metadata().ok()?.len();
    if size == 0 || size > MAX_FILE_BYTES {
        return None;
    }
    let mut input = Vec::with_capacity(size as usize);
    file.read_to_end(&mut input).ok()?;
    if input.len() != size as usize {
        return None;
    }

    let root = JsonParser::new(&input).parse()?;
    let compiler_options = object_field(&root, b"compilerOptions")?;
    let paths_value = object_field(compiler_options, b"paths");
    let base_url = object_field(compiler_options, b"baseUrl").and_then(string_value);
    if paths_value.is_none() && base_url.is_none() {
        return None;
    }

    let map = c_alloc::<CbmPathAliasMap>();
    if map.is_null() {
        return None;
    }

    if let Some(base_url) = base_url {
        let resolved = if !base_url.is_empty() && base_url != b"." {
            resolve_target_relative(dir_prefix, base_url)
        } else if base_url == b"." && !dir_prefix.is_empty() {
            dir_prefix.to_vec()
        } else {
            Vec::new()
        };
        if !resolved.is_empty() {
            // SAFETY: map is a newly allocated zeroed C struct.
            unsafe { (*map).base_url = c_string(&resolved) };
            if unsafe { (*map).base_url.is_null() } {
                unsafe { free_map(map) };
                return None;
            }
        }
    }

    if let Some(JsonValue::Object(fields)) = paths_value {
        let capped = fields.len() > MAX_ENTRIES;
        let capacity = fields.len().min(MAX_ENTRIES);
        if capacity > 0 {
            // SAFETY: map is a newly allocated zeroed C struct.
            unsafe { (*map).entries = c_alloc_array::<CbmPathAlias>(capacity) };
            if unsafe { (*map).entries.is_null() } {
                unsafe { free_map(map) };
                return None;
            }
            for (key, value) in fields {
                let count = unsafe { (*map).count.max(0) as usize };
                if count >= capacity {
                    break;
                }
                let JsonValue::Array(targets) = value else {
                    continue;
                };
                let Some(target) = targets.first().and_then(string_value) else {
                    continue;
                };

                let (alias_prefix, alias_suffix, has_wildcard) =
                    if let Some(star) = key.iter().position(|byte| *byte == b'*') {
                        (&key[..star], &key[star + 1..], true)
                    } else {
                        (key.as_slice(), b"".as_slice(), false)
                    };
                let (target_prefix, target_suffix) =
                    if let Some(star) = target.iter().position(|byte| *byte == b'*') {
                        (
                            resolve_target_relative(dir_prefix, &target[..star]),
                            target[star + 1..].to_vec(),
                        )
                    } else {
                        (resolve_target_relative(dir_prefix, target), Vec::new())
                    };
                let entry = CbmPathAlias {
                    alias_prefix: c_string(alias_prefix),
                    alias_suffix: c_string(alias_suffix),
                    target_prefix: c_string(&target_prefix),
                    target_suffix: c_string(&target_suffix),
                    has_wildcard,
                };
                if entry.alias_prefix.is_null()
                    || entry.alias_suffix.is_null()
                    || entry.target_prefix.is_null()
                    || entry.target_suffix.is_null()
                {
                    unsafe {
                        c_free(entry.alias_prefix);
                        c_free(entry.alias_suffix);
                        c_free(entry.target_prefix);
                        c_free(entry.target_suffix);
                        free_map(map);
                    }
                    return None;
                }
                // SAFETY: count is below the allocated capacity.
                unsafe {
                    ptr::write((*map).entries.add(count), entry);
                    (*map).count += 1;
                }
            }
            if capped {
                warn_cap("path_alias.entries.cap_hit", "config", abs_path);
            }
            let count = unsafe { (*map).count.max(0) as usize };
            // SAFETY: entries points to `capacity` initialized zeroed entries and the first
            // `count` positions now contain valid alias entries.
            unsafe {
                std::slice::from_raw_parts_mut((*map).entries, count).sort_by(|left, right| {
                    let left_len = c_bytes(left.alias_prefix).map_or(0, <[u8]>::len);
                    let right_len = c_bytes(right.alias_prefix).map_or(0, <[u8]>::len);
                    right_len.cmp(&left_len)
                });
            }
        } else if capped {
            warn_cap("path_alias.entries.cap_hit", "config", abs_path);
        }
    }

    Some(map)
}

struct ConfigHit {
    abs_path: PathBuf,
    rel_dir: Vec<u8>,
}

fn skip_directory(name: &[u8]) -> bool {
    name.first() == Some(&b'.')
        || name == b"node_modules"
        || name == b"dist"
        || name == b"build"
        || name == b".next"
        || name == b"coverage"
        || name == b"target"
}

fn find_alias_files(dir: &Path, rel_dir: &[u8], depth: usize, hits: &mut Vec<ConfigHit>) {
    if hits.len() >= MAX_FILES || depth > MAX_DEPTH {
        return;
    }

    let tsconfig = dir.join("tsconfig.json");
    let jsconfig = dir.join("jsconfig.json");
    let config = if File::open(&tsconfig).is_ok() {
        Some(tsconfig)
    } else if File::open(&jsconfig).is_ok() {
        Some(jsconfig)
    } else {
        None
    };
    if let Some(abs_path) = config {
        hits.push(ConfigHit {
            abs_path,
            rel_dir: rel_dir.to_vec(),
        });
    }

    let Ok(entries) = fs::read_dir(dir) else {
        return;
    };
    for entry in entries.flatten() {
        if hits.len() >= MAX_FILES {
            return;
        }
        let Ok(file_type) = entry.file_type() else {
            continue;
        };
        if !file_type.is_dir() {
            continue;
        }
        let name = entry.file_name().to_string_lossy().as_bytes().to_vec();
        if skip_directory(&name) {
            continue;
        }
        let mut child_rel = rel_dir.to_vec();
        if !child_rel.is_empty() {
            child_rel.push(b'/');
        }
        child_rel.extend_from_slice(&name);
        find_alias_files(&entry.path(), &child_rel, depth + 1, hits);
    }
}

#[no_mangle]
pub unsafe extern "C" fn cbm_path_alias_collection_free(coll: *mut CbmPathAliasCollection) {
    unsafe { free_collection(coll) };
}

#[no_mangle]
pub unsafe extern "C" fn cbm_path_alias_resolve(
    map: *const CbmPathAliasMap,
    module_path: *const c_char,
) -> *mut c_char {
    let Some(module_path) = (unsafe { c_bytes(module_path) }) else {
        return ptr::null_mut();
    };
    if map.is_null() {
        return ptr::null_mut();
    }

    let count = unsafe { (*map).count.max(0) as usize };
    let entries = unsafe { (*map).entries };
    if !entries.is_null() {
        for index in 0..count {
            // SAFETY: C ABI map points at at least `count` entries.
            let entry = unsafe { &*entries.add(index) };
            let alias_prefix = unsafe { c_bytes(entry.alias_prefix) }.unwrap_or_default();
            let alias_suffix = unsafe { c_bytes(entry.alias_suffix) }.unwrap_or_default();
            let target_prefix = unsafe { c_bytes(entry.target_prefix) }.unwrap_or_default();
            let target_suffix = unsafe { c_bytes(entry.target_suffix) }.unwrap_or_default();

            if entry.has_wildcard {
                if module_path.len() < alias_prefix.len() + alias_suffix.len()
                    || !module_path.starts_with(alias_prefix)
                    || (!alias_suffix.is_empty() && !module_path.ends_with(alias_suffix))
                {
                    continue;
                }
                let wildcard_end = module_path.len() - alias_suffix.len();
                let wildcard = &module_path[alias_prefix.len()..wildcard_end];
                let mut result =
                    Vec::with_capacity(target_prefix.len() + wildcard.len() + target_suffix.len());
                result.extend_from_slice(target_prefix);
                result.extend_from_slice(wildcard);
                result.extend_from_slice(target_suffix);
                return c_string(&strip_resolved_ext(result));
            }
            if module_path == alias_prefix {
                return c_string(&strip_resolved_ext(target_prefix.to_vec()));
            }
        }
    }

    let base_url = unsafe { c_bytes((*map).base_url) }.unwrap_or_default();
    if !base_url.is_empty()
        && module_path.first() != Some(&b'.')
        && module_path.first() != Some(&b'@')
        && module_path.contains(&b'/')
    {
        let mut result = Vec::with_capacity(base_url.len() + 1 + module_path.len());
        result.extend_from_slice(base_url);
        result.push(b'/');
        result.extend_from_slice(module_path);
        return c_string(&strip_resolved_ext(result));
    }
    ptr::null_mut()
}

#[no_mangle]
pub unsafe extern "C" fn cbm_load_path_aliases(
    repo_path: *const c_char,
) -> *mut CbmPathAliasCollection {
    let Some(repo_path) = (unsafe { c_bytes(repo_path) }) else {
        return ptr::null_mut();
    };
    let Ok(repo_path) = std::str::from_utf8(repo_path) else {
        return ptr::null_mut();
    };
    let root = Path::new(repo_path);
    let mut hits = Vec::new();
    find_alias_files(root, b"", 0, &mut hits);
    if hits.len() >= MAX_FILES {
        warn_cap("path_alias.files.cap_hit", "repo", root);
    }
    if hits.is_empty() {
        return ptr::null_mut();
    }

    let coll = c_alloc::<CbmPathAliasCollection>();
    if coll.is_null() {
        return ptr::null_mut();
    }
    // SAFETY: coll is a newly allocated zeroed C struct.
    unsafe { (*coll).scopes = c_alloc_array::<CbmPathAliasScope>(hits.len()) };
    if unsafe { (*coll).scopes.is_null() } {
        unsafe { c_free(coll) };
        return ptr::null_mut();
    }

    for hit in &hits {
        let Some(map) = load_tsconfig_file(&hit.abs_path, &hit.rel_dir) else {
            continue;
        };
        let scope = CbmPathAliasScope {
            dir_prefix: c_string(&hit.rel_dir),
            map,
        };
        if scope.dir_prefix.is_null() {
            unsafe {
                free_map(map);
                free_collection(coll);
            }
            return ptr::null_mut();
        }
        let count = unsafe { (*coll).count.max(0) as usize };
        // SAFETY: count is below hits.len() and scopes was allocated for all hits.
        unsafe {
            ptr::write((*coll).scopes.add(count), scope);
            (*coll).count += 1;
        }
    }

    let count = unsafe { (*coll).count.max(0) as usize };
    if count == 0 {
        unsafe { free_collection(coll) };
        return ptr::null_mut();
    }
    // SAFETY: scopes points at `count` initialized scopes.
    unsafe {
        std::slice::from_raw_parts_mut((*coll).scopes, count).sort_by(|left, right| {
            let left_len = c_bytes(left.dir_prefix).map_or(0, <[u8]>::len);
            let right_len = c_bytes(right.dir_prefix).map_or(0, <[u8]>::len);
            right_len.cmp(&left_len)
        });
    }
    coll
}

#[no_mangle]
pub unsafe extern "C" fn cbm_path_alias_find_for_file(
    coll: *const CbmPathAliasCollection,
    rel_path: *const c_char,
) -> *const CbmPathAliasMap {
    let Some(rel_path) = (unsafe { c_bytes(rel_path) }) else {
        return ptr::null();
    };
    if coll.is_null() {
        return ptr::null();
    }
    let count = unsafe { (*coll).count.max(0) as usize };
    let scopes = unsafe { (*coll).scopes };
    if scopes.is_null() {
        return ptr::null();
    }
    for index in 0..count {
        // SAFETY: C ABI collection points at at least `count` scopes.
        let scope = unsafe { &*scopes.add(index) };
        let prefix = unsafe { c_bytes(scope.dir_prefix) }.unwrap_or_default();
        if prefix.is_empty() {
            return scope.map;
        }
        if rel_path.len() >= prefix.len()
            && rel_path.starts_with(prefix)
            && (rel_path.get(prefix.len()) == Some(&b'/') || rel_path.get(prefix.len()) == Some(&0))
        {
            return scope.map;
        }
    }
    ptr::null()
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;
    use std::io::Write;

    fn map(entries: &[(&[u8], &[u8])], base_url: Option<&[u8]>) -> CbmPathAliasMap {
        let raw = c_alloc_array::<CbmPathAlias>(entries.len());
        assert!(!raw.is_null());
        for (index, (alias, target)) in entries.iter().enumerate() {
            let alias_star = alias.iter().position(|byte| *byte == b'*');
            let target_star = target.iter().position(|byte| *byte == b'*');
            let entry = CbmPathAlias {
                alias_prefix: c_string(alias_star.map_or(*alias, |star| &alias[..star])),
                alias_suffix: c_string(
                    alias_star.map_or(b"".as_slice(), |star| &alias[star + 1..]),
                ),
                target_prefix: c_string(
                    &target_star.map_or(target.to_vec(), |star| target[..star].to_vec()),
                ),
                target_suffix: c_string(
                    target_star.map_or(b"".as_slice(), |star| &target[star + 1..]),
                ),
                has_wildcard: alias_star.is_some(),
            };
            unsafe { ptr::write(raw.add(index), entry) };
        }
        CbmPathAliasMap {
            entries: raw,
            count: entries.len() as c_int,
            base_url: base_url.map_or(ptr::null_mut(), c_string),
        }
    }

    #[test]
    fn resolves_specific_alias_and_extensions() {
        let raw = map(&[(b"@/lib/*", b"shared/*"), (b"@/*", b"src/*.ts")], None);
        let module = CString::new("@/lib/auth").expect("literal has no NUL");
        let resolved = unsafe { cbm_path_alias_resolve(&raw, module.as_ptr()) };
        assert_eq!(
            unsafe { CStr::from_ptr(resolved) }.to_bytes(),
            b"shared/auth"
        );
        unsafe { c_free(resolved) };
        unsafe { c_free(raw.entries) };
    }

    #[test]
    fn parser_accepts_comments_and_trailing_commas() {
        let dir = std::env::temp_dir().join("cbm-rust-path-alias-unit");
        let _ = fs::create_dir_all(&dir);
        let file_path = dir.join("tsconfig.json");
        let mut file = File::create(&file_path).expect("temporary config");
        file.write_all(
            br#"{
                // accepted by yyjson flags
                "compilerOptions": {
                    "baseUrl": ".",
                    "paths": { "@/*": ["src/*"], },
                },
            }"#,
        )
        .expect("write config");
        let map = load_tsconfig_file(&file_path, b"").expect("config map");
        let module = CString::new("@/main").expect("literal has no NUL");
        let resolved = unsafe { cbm_path_alias_resolve(map, module.as_ptr()) };
        assert_eq!(unsafe { CStr::from_ptr(resolved) }.to_bytes(), b"src/main");
        unsafe {
            c_free(resolved);
            free_map(map);
        }
        let _ = fs::remove_file(file_path);
        let _ = fs::remove_dir(dir);
    }

    #[test]
    fn parser_accepts_unicode_escape_in_alias_prefix() {
        let dir = std::env::temp_dir().join("cbm-rust-path-alias-unicode");
        let _ = fs::create_dir_all(&dir);
        let file_path = dir.join("tsconfig.json");
        let mut file = File::create(&file_path).expect("temporary config");
        file.write_all(
            br#"{
                "compilerOptions": {
                    "baseUrl": ".",
                    "paths": { "\u0040/*": ["lib/*"] },
                },
            }"#,
        )
        .expect("write config");
        let map = load_tsconfig_file(&file_path, b"").expect("config map");
        let module = CString::new("@/main").expect("literal has no NUL");
        let resolved = unsafe { cbm_path_alias_resolve(map, module.as_ptr()) };
        assert_eq!(unsafe { CStr::from_ptr(resolved) }.to_bytes(), b"lib/main");
        unsafe {
            c_free(resolved);
            free_map(map);
        }
        let _ = fs::remove_file(file_path);
        let _ = fs::remove_dir(dir);
    }
}
