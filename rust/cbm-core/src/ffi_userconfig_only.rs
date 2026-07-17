#![allow(unsafe_code)]

//! User-defined extension-to-language mappings 的 Rust ABI 實作。

use std::ffi::{CStr, CString};
use std::fs;
use std::os::raw::{c_char, c_int};
use std::ptr;

const CBM_LANG_COUNT: c_int = 159;
const MAX_CONFIG_SIZE: usize = 65_536;

unsafe extern "C" {
    fn cbm_app_config_dir() -> *const c_char;
    fn cbm_language_name(language: c_int) -> *const c_char;
}

#[repr(C)]
pub struct CbmUserext {
    pub ext: *mut c_char,
    pub lang: c_int,
}

#[repr(C)]
pub struct CbmUserconfig {
    pub entries: *mut CbmUserext,
    pub count: c_int,
}

static mut G_USERCONFIG: *const CbmUserconfig = ptr::null();

struct OwnedEntry {
    ext: CString,
    lang: c_int,
}

fn c_string_lossy(value: *const c_char) -> Option<String> {
    if value.is_null() {
        return None;
    }
    // SAFETY: 呼叫端需提供以 NUL 結尾且在此呼叫期間有效的 C 字串。
    Some(
        unsafe { CStr::from_ptr(value) }
            .to_string_lossy()
            .into_owned(),
    )
}

fn c_string_bytes(value: *const c_char) -> Option<&'static [u8]> {
    if value.is_null() {
        return None;
    }
    // SAFETY: 此函式只在立即比較期間使用呼叫端提供的有效 C 字串。
    Some(unsafe { CStr::from_ptr(value) }.to_bytes())
}

fn trim_c_text(value: &[u8]) -> &[u8] {
    let nul = value
        .iter()
        .position(|byte| *byte == 0)
        .unwrap_or(value.len());
    &value[..nul.min(63)]
}

fn ascii_case_eq(left: &[u8], right: &[u8]) -> bool {
    let left = trim_c_text(left);
    let right = trim_c_text(right);
    left.len() == right.len()
        && left
            .iter()
            .zip(right.iter())
            .all(|(a, b)| a.eq_ignore_ascii_case(b))
}

fn language_candidates(value: &[u8]) -> [&[u8]; 2] {
    let mut candidates = [value, &[]];
    let aliases: &[(&[u8], &[u8])] = &[
        (b"cpp", b"c++"),
        (b"csharp", b"c#"),
        (b"sh", b"bash"),
        (b"terraform", b"hcl"),
        (b"fsharp", b"f#"),
        (b"common lisp", b"common lisp"),
        (b"commonlisp", b"common lisp"),
        (b"lisp", b"common lisp"),
        (b"objective-c", b"objective-c"),
        (b"objc", b"objective-c"),
        (b"emacs lisp", b"emacs lisp"),
        (b"emacslisp", b"emacs lisp"),
    ];
    for (alias, canonical) in aliases {
        if ascii_case_eq(value, alias) {
            candidates[1] = canonical;
            break;
        }
    }
    candidates
}

fn language_from_string(value: &str) -> c_int {
    let value_bytes = value.as_bytes();
    if value_bytes.is_empty() {
        return CBM_LANG_COUNT;
    }
    let candidates = language_candidates(value_bytes);
    for candidate in candidates {
        if candidate.is_empty() {
            continue;
        }
        for language in 0..CBM_LANG_COUNT {
            // SAFETY: cbm_language_name returns a borrowed, NUL-terminated name for a valid enum.
            let name = unsafe { cbm_language_name(language) };
            let Some(name) = c_string_bytes(name) else {
                continue;
            };
            if ascii_case_eq(candidate, name) {
                return language;
            }
        }
    }
    CBM_LANG_COUNT
}

struct JsonParser<'a> {
    input: &'a [u8],
    position: usize,
}

impl<'a> JsonParser<'a> {
    fn new(input: &'a [u8]) -> Self {
        Self { input, position: 0 }
    }

    fn peek(&self) -> Option<u8> {
        self.input.get(self.position).copied()
    }

    fn next(&mut self) -> Option<u8> {
        let value = self.peek()?;
        self.position += 1;
        Some(value)
    }

    fn skip_ws(&mut self) {
        while matches!(self.peek(), Some(b' ' | b'\n' | b'\r' | b'\t')) {
            self.position += 1;
        }
    }

    fn expect(&mut self, expected: u8) -> Result<(), ()> {
        if self.next() == Some(expected) {
            Ok(())
        } else {
            Err(())
        }
    }

    fn parse_hex4(&mut self) -> Result<u16, ()> {
        let mut value = 0u16;
        for _ in 0..4 {
            let byte = self.next().ok_or(())?;
            value = value.checked_mul(16).ok_or(())?
                + match byte {
                    b'0'..=b'9' => u16::from(byte - b'0'),
                    b'a'..=b'f' => u16::from(byte - b'a' + 10),
                    b'A'..=b'F' => u16::from(byte - b'A' + 10),
                    _ => return Err(()),
                };
        }
        Ok(value)
    }

    fn parse_string(&mut self) -> Result<String, ()> {
        self.expect(b'"')?;
        let mut bytes = Vec::new();
        loop {
            let byte = self.next().ok_or(())?;
            match byte {
                b'"' => return String::from_utf8(bytes).map_err(|_| ()),
                b'\\' => {
                    let escaped = self.next().ok_or(())?;
                    match escaped {
                        b'"' | b'\\' | b'/' => bytes.push(escaped),
                        b'b' => bytes.push(0x08),
                        b'f' => bytes.push(0x0c),
                        b'n' => bytes.push(b'\n'),
                        b'r' => bytes.push(b'\r'),
                        b't' => bytes.push(b'\t'),
                        b'u' => {
                            let first = self.parse_hex4()?;
                            let codepoint = if (0xd800..=0xdbff).contains(&first) {
                                if self.next() != Some(b'\\') || self.next() != Some(b'u') {
                                    return Err(());
                                }
                                let second = self.parse_hex4()?;
                                if !(0xdc00..=0xdfff).contains(&second) {
                                    return Err(());
                                }
                                0x1_0000
                                    + ((u32::from(first) - 0xd800) << 10)
                                    + (u32::from(second) - 0xdc00)
                            } else if (0xdc00..=0xdfff).contains(&first) {
                                return Err(());
                            } else {
                                u32::from(first)
                            };
                            let character = char::from_u32(codepoint).ok_or(())?;
                            let mut encoded = [0u8; 4];
                            bytes.extend_from_slice(character.encode_utf8(&mut encoded).as_bytes());
                        }
                        _ => return Err(()),
                    }
                }
                0..=0x1f => return Err(()),
                _ => bytes.push(byte),
            }
        }
    }

    fn parse_number(&mut self) -> Result<(), ()> {
        if self.peek() == Some(b'-') {
            self.position += 1;
        }
        match self.next() {
            Some(b'0') => {}
            Some(b'1'..=b'9') => {
                while matches!(self.peek(), Some(b'0'..=b'9')) {
                    self.position += 1;
                }
            }
            _ => return Err(()),
        }
        if self.peek() == Some(b'.') {
            self.position += 1;
            if !matches!(self.next(), Some(b'0'..=b'9')) {
                return Err(());
            }
            while matches!(self.peek(), Some(b'0'..=b'9')) {
                self.position += 1;
            }
        }
        if matches!(self.peek(), Some(b'e' | b'E')) {
            self.position += 1;
            if matches!(self.peek(), Some(b'+' | b'-')) {
                self.position += 1;
            }
            if !matches!(self.next(), Some(b'0'..=b'9')) {
                return Err(());
            }
            while matches!(self.peek(), Some(b'0'..=b'9')) {
                self.position += 1;
            }
        }
        Ok(())
    }

    fn parse_array(&mut self) -> Result<(), ()> {
        self.expect(b'[')?;
        self.skip_ws();
        if self.next() == Some(b']') {
            return Ok(());
        }
        self.position = self.position.saturating_sub(1);
        loop {
            self.skip_value()?;
            self.skip_ws();
            match self.next() {
                Some(b']') => return Ok(()),
                Some(b',') => {}
                _ => return Err(()),
            }
        }
    }

    fn parse_object(&mut self) -> Result<(), ()> {
        self.expect(b'{')?;
        self.skip_ws();
        if self.next() == Some(b'}') {
            return Ok(());
        }
        self.position = self.position.saturating_sub(1);
        loop {
            self.skip_ws();
            self.parse_string()?;
            self.skip_ws();
            self.expect(b':')?;
            self.skip_value()?;
            self.skip_ws();
            match self.next() {
                Some(b'}') => return Ok(()),
                Some(b',') => {}
                _ => return Err(()),
            }
        }
    }

    fn skip_value(&mut self) -> Result<(), ()> {
        self.skip_ws();
        match self.peek() {
            Some(b'"') => self.parse_string().map(|_| ()),
            Some(b'{') => self.parse_object(),
            Some(b'[') => self.parse_array(),
            Some(b't') if self.input.get(self.position..self.position + 4) == Some(b"true") => {
                self.position += 4;
                Ok(())
            }
            Some(b'f') if self.input.get(self.position..self.position + 5) == Some(b"false") => {
                self.position += 5;
                Ok(())
            }
            Some(b'n') if self.input.get(self.position..self.position + 4) == Some(b"null") => {
                self.position += 4;
                Ok(())
            }
            Some(b'-' | b'0'..=b'9') => self.parse_number(),
            _ => Err(()),
        }
    }

    fn parse_extra_object(&mut self) -> Result<Vec<(String, String)>, ()> {
        self.expect(b'{')?;
        self.skip_ws();
        if self.next() == Some(b'}') {
            return Ok(Vec::new());
        }
        self.position = self.position.saturating_sub(1);
        let mut entries = Vec::new();
        loop {
            self.skip_ws();
            let extension = self.parse_string()?;
            self.skip_ws();
            self.expect(b':')?;
            self.skip_ws();
            if self.peek() == Some(b'"') {
                let language = self.parse_string()?;
                entries.push((extension, language));
            } else {
                self.skip_value()?;
            }
            self.skip_ws();
            match self.next() {
                Some(b'}') => return Ok(entries),
                Some(b',') => {}
                _ => return Err(()),
            }
        }
    }

    fn parse_document(&mut self) -> Result<Option<Vec<(String, String)>>, ()> {
        self.skip_ws();
        if self.peek() != Some(b'{') {
            self.skip_value()?;
            self.skip_ws();
            return if self.position == self.input.len() {
                Ok(None)
            } else {
                Err(())
            };
        }

        self.expect(b'{')?;
        self.skip_ws();
        if self.next() == Some(b'}') {
            return Ok(Some(Vec::new()));
        }
        self.position = self.position.saturating_sub(1);

        let mut found = false;
        let mut entries = Vec::new();
        loop {
            self.skip_ws();
            let key = self.parse_string()?;
            self.skip_ws();
            self.expect(b':')?;
            self.skip_ws();
            if key == "extra_extensions" && !found {
                found = true;
                if self.peek() == Some(b'{') {
                    entries = self.parse_extra_object()?;
                } else {
                    self.skip_value()?;
                }
            } else {
                self.skip_value()?;
            }
            self.skip_ws();
            match self.next() {
                Some(b'}') => {
                    self.skip_ws();
                    return if self.position == self.input.len() {
                        Ok(Some(entries))
                    } else {
                        Err(())
                    };
                }
                Some(b',') => {}
                _ => return Err(()),
            }
        }
    }
}

fn load_config_file(path: &str, entries: &mut Vec<OwnedEntry>) {
    let Ok(data) = fs::read(path) else {
        return;
    };
    if data.is_empty() || data.len() > MAX_CONFIG_SIZE {
        return;
    }
    let Ok(Some(parsed)) = JsonParser::new(&data).parse_document() else {
        return;
    };
    for (extension, language) in parsed {
        let extension_bytes = extension.as_bytes();
        let extension_end = extension_bytes
            .iter()
            .position(|byte| *byte == 0)
            .unwrap_or(extension_bytes.len());
        if extension_end == 0 || extension_bytes[0] != b'.' {
            continue;
        }
        let lang = language_from_string(&language);
        if lang == CBM_LANG_COUNT {
            continue;
        }
        let Ok(ext) = CString::new(&extension_bytes[..extension_end]) else {
            continue;
        };
        entries.push(OwnedEntry { ext, lang });
    }
}

fn dedup_project_over_global(entries: &mut Vec<OwnedEntry>, global_count: &mut usize) {
    for project_index in *global_count..entries.len() {
        let mut global_index = 0;
        while global_index < *global_count {
            if entries[global_index].ext.as_bytes() == entries[project_index].ext.as_bytes() {
                let last_global = *global_count - 1;
                entries[global_index].ext = CString::new(Vec::new()).expect("empty CString");
                if global_index != last_global {
                    entries.swap(global_index, last_global);
                }
                entries[last_global].ext = CString::new(Vec::new()).expect("empty CString");
                *global_count -= 1;
                break;
            }
            global_index += 1;
        }
    }
    entries.retain(|entry| !entry.ext.as_bytes().is_empty());
}

fn build_config(entries: Vec<OwnedEntry>) -> *mut CbmUserconfig {
    let mut c_entries = Vec::with_capacity(entries.len());
    for entry in entries {
        c_entries.push(CbmUserext {
            ext: entry.ext.into_raw(),
            lang: entry.lang,
        });
    }
    let count = c_entries.len() as c_int;
    let entries_ptr = if c_entries.is_empty() {
        ptr::null_mut()
    } else {
        let ptr = c_entries.as_mut_ptr();
        std::mem::forget(c_entries);
        ptr
    };
    Box::into_raw(Box::new(CbmUserconfig {
        entries: entries_ptr,
        count,
    }))
}

/// 設定全域 userconfig 指標；指標所有權仍由呼叫端保留。
///
/// # Safety
/// `cfg` 必須是 NULL 或在後續查詢期間仍有效的 `cbm_userconfig_t` 指標。
#[no_mangle]
pub unsafe extern "C" fn cbm_set_user_lang_config(cfg: *const CbmUserconfig) {
    // SAFETY: 此函式只保存呼叫端保證在使用期間有效的 borrowed pointer。
    unsafe { G_USERCONFIG = cfg };
}

/// 取得目前的 borrowed userconfig 指標。
///
/// # Safety
/// 此函式沒有額外的呼叫端前置條件；回傳指標的有效期由最近一次設定者管理。
#[no_mangle]
pub unsafe extern "C" fn cbm_get_user_lang_config() -> *const CbmUserconfig {
    // SAFETY: 讀取本模組維護的 process-global pointer。
    unsafe { G_USERCONFIG }
}

/// 載入 global 與 project userconfig，並以 project 設定覆蓋同名 extension。
///
/// # Safety
/// `repo_path` 必須是 NULL 或有效的 NUL 結尾 C 字串，且在此呼叫期間保持有效。
#[no_mangle]
pub unsafe extern "C" fn cbm_userconfig_load(repo_path: *const c_char) -> *mut CbmUserconfig {
    let config_base = {
        // SAFETY: C 函式回傳 process-owned、NUL 結尾的 borrowed path。
        let path = unsafe { cbm_app_config_dir() };
        c_string_lossy(path).unwrap_or_else(|| "/tmp".to_string())
    };
    let mut entries = Vec::new();
    let global_path = format!("{config_base}/codebase-memory-mcp/config.json");
    load_config_file(&global_path, &mut entries);
    let global_count = entries.len();

    if let Some(repo_path) = c_string_lossy(repo_path) {
        if !repo_path.is_empty() {
            let project_path = format!("{repo_path}/.codebase-memory.json");
            load_config_file(&project_path, &mut entries);
        }
    }

    let mut global_count = global_count;
    dedup_project_over_global(&mut entries, &mut global_count);
    build_config(entries)
}

/// 查詢指定 extension 的 user-defined language。
///
/// # Safety
/// `cfg` 可為 NULL；非 NULL 時必須指向 `cbm_userconfig_load` 回傳的有效配置，
/// `ext` 必須是 NULL 或有效的 NUL 結尾 C 字串。
#[no_mangle]
pub unsafe extern "C" fn cbm_userconfig_lookup(
    cfg: *const CbmUserconfig,
    ext: *const c_char,
) -> c_int {
    if cfg.is_null() || ext.is_null() {
        return CBM_LANG_COUNT;
    }
    // SAFETY: 呼叫端需提供有效的 C 結構與字串；結構由本模組配置。
    let cfg = unsafe { &*cfg };
    let Some(ext) = c_string_bytes(ext) else {
        return CBM_LANG_COUNT;
    };
    if ext.is_empty() || cfg.count <= 0 || cfg.entries.is_null() {
        return CBM_LANG_COUNT;
    }
    let count = cfg.count as usize;
    // SAFETY: `entries` 指向 `cfg.count` 個本模組配置的元素。
    let entries = unsafe { std::slice::from_raw_parts(cfg.entries, count) };
    entries
        .iter()
        .find(|entry| {
            if entry.ext.is_null() {
                return false;
            }
            // SAFETY: 每個 ext 由 CString::into_raw 配置並維持 NUL 結尾。
            unsafe { CStr::from_ptr(entry.ext) }.to_bytes() == ext
        })
        .map_or(CBM_LANG_COUNT, |entry| entry.lang)
}

/// 釋放 `cbm_userconfig_load` 回傳的配置；NULL-safe。
///
/// # Safety
/// `cfg` 必須是 NULL 或仍未被釋放的 `cbm_userconfig_load` 回傳值。
#[no_mangle]
pub unsafe extern "C" fn cbm_userconfig_free(cfg: *mut CbmUserconfig) {
    if cfg.is_null() {
        return;
    }
    // SAFETY: cfg 必須由 cbm_userconfig_load 回傳且只釋放一次。
    let cfg = unsafe { Box::from_raw(cfg) };
    if cfg.count > 0 && !cfg.entries.is_null() {
        let count = cfg.count as usize;
        // SAFETY: entries 與 count 來自 build_config 的同一個 Vec 配置。
        let entries = unsafe { std::slice::from_raw_parts_mut(cfg.entries, count) };
        for entry in entries.iter_mut() {
            if !entry.ext.is_null() {
                // SAFETY: ext 由 CString::into_raw 配置，且每個指標只出現一次。
                drop(unsafe { CString::from_raw(entry.ext) });
                entry.ext = ptr::null_mut();
            }
        }
        // SAFETY: 重建並丟棄 build_config 忘記的 Vec allocation。
        drop(unsafe { Vec::from_raw_parts(cfg.entries, count, count) });
    }
}

#[cfg(test)]
mod tests {
    use super::{ascii_case_eq, JsonParser};

    #[test]
    fn parses_extra_extensions_and_ignores_non_strings() {
        let input = br#"{
            "name": "fixture",
            "extra_extensions": {".blade.php": "php", ".ignored": 42, ".mjs": "javascript"}
        }"#;
        let parsed = JsonParser::new(input).parse_document().unwrap().unwrap();
        assert_eq!(parsed.len(), 2);
        assert_eq!(parsed[0], (".blade.php".to_string(), "php".to_string()));
        assert_eq!(parsed[1], (".mjs".to_string(), "javascript".to_string()));
    }

    #[test]
    fn accepts_unicode_escapes_and_rejects_invalid_documents() {
        let input = br#"{"extra_extensions":{".x\u0079z":"r\u0075st"}}"#;
        let parsed = JsonParser::new(input).parse_document().unwrap().unwrap();
        assert_eq!(parsed[0].0, ".xyz");
        assert_eq!(parsed[0].1, "rust");
        assert!(JsonParser::new(br#"{"extra_extensions":{".x":"rust"}"#)
            .parse_document()
            .is_err());
    }

    #[test]
    fn preserves_valid_non_object_root_as_empty_config() {
        assert!(JsonParser::new(br#"[1, true, null]"#)
            .parse_document()
            .unwrap()
            .is_none());
        assert!(ascii_case_eq(b"CShArP", b"csharp"));
    }
}
