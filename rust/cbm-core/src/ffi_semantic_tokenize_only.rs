//! `semantic.c` 的 token extraction direct-only replacement。
//!
//! 保留 C 的 ASCII delimiter/camel-case 規則、128-byte token buffer、
//! abbreviation expansion 與 C allocator ownership。

#![allow(unsafe_code)]

use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_void};
use std::ptr;

const TOKEN_BUF_LEN: usize = 128;

unsafe extern "C" {
    fn malloc(size: usize) -> *mut c_void;
}

const ABBREVIATIONS: &[(&[u8], &[u8])] = &[
    (b"err", b"error"),
    (b"exc", b"exception"),
    (b"ex", b"exception"),
    (b"ctx", b"context"),
    (b"cfg", b"config"),
    (b"conf", b"configuration"),
    (b"env", b"environment"),
    (b"opt", b"option"),
    (b"opts", b"options"),
    (b"req", b"request"),
    (b"res", b"response"),
    (b"resp", b"response"),
    (b"rsp", b"response"),
    (b"hdr", b"header"),
    (b"hdrs", b"headers"),
    (b"str", b"string"),
    (b"fmt", b"format"),
    (b"msg", b"message"),
    (b"txt", b"text"),
    (b"lbl", b"label"),
    (b"desc", b"description"),
    (b"buf", b"buffer"),
    (b"arr", b"array"),
    (b"vec", b"vector"),
    (b"lst", b"list"),
    (b"dict", b"dictionary"),
    (b"tbl", b"table"),
    (b"stk", b"stack"),
    (b"que", b"queue"),
    (b"fn", b"function"),
    (b"func", b"function"),
    (b"cb", b"callback"),
    (b"proc", b"procedure"),
    (b"ctor", b"constructor"),
    (b"dtor", b"destructor"),
    (b"db", b"database"),
    (b"col", b"column"),
    (b"tbl", b"table"),
    (b"stmt", b"statement"),
    (b"txn", b"transaction"),
    (b"trx", b"transaction"),
    (b"repo", b"repository"),
    (b"auth", b"authentication"),
    (b"authz", b"authorization"),
    (b"perm", b"permission"),
    (b"cred", b"credential"),
    (b"tok", b"token"),
    (b"pwd", b"password"),
    (b"val", b"value"),
    (b"num", b"number"),
    (b"int", b"integer"),
    (b"bool", b"boolean"),
    (b"flt", b"float"),
    (b"dbl", b"double"),
    (b"idx", b"index"),
    (b"iter", b"iterator"),
    (b"elem", b"element"),
    (b"cnt", b"count"),
    (b"len", b"length"),
    (b"sz", b"size"),
    (b"pos", b"position"),
    (b"off", b"offset"),
    (b"cap", b"capacity"),
    (b"init", b"initialize"),
    (b"deinit", b"deinitialize"),
    (b"alloc", b"allocate"),
    (b"dealloc", b"deallocate"),
    (b"del", b"delete"),
    (b"rm", b"remove"),
    (b"impl", b"implementation"),
    (b"iface", b"interface"),
    (b"abs", b"abstract"),
    (b"decl", b"declaration"),
    (b"param", b"parameter"),
    (b"arg", b"argument"),
    (b"attr", b"attribute"),
    (b"prop", b"property"),
    (b"ret", b"return"),
    (b"src", b"source"),
    (b"dst", b"destination"),
    (b"tgt", b"target"),
    (b"orig", b"original"),
    (b"prev", b"previous"),
    (b"cur", b"current"),
    (b"tmp", b"temporary"),
    (b"temp", b"temporary"),
    (b"conn", b"connection"),
    (b"sess", b"session"),
    (b"sock", b"socket"),
    (b"addr", b"address"),
    (b"url", b"uniform"),
    (b"srv", b"server"),
    (b"cli", b"client"),
    (b"svc", b"service"),
    (b"ep", b"endpoint"),
    (b"mgr", b"manager"),
    (b"ctrl", b"controller"),
    (b"hdlr", b"handler"),
    (b"sched", b"scheduler"),
    (b"disp", b"dispatcher"),
    (b"reg", b"registry"),
    (b"chan", b"channel"),
    (b"sem", b"semaphore"),
    (b"mtx", b"mutex"),
    (b"wg", b"waitgroup"),
    (b"sig", b"signal"),
    (b"evt", b"event"),
    (b"sub", b"subscriber"),
    (b"pub", b"publisher"),
    (b"spec", b"specification"),
    (b"mock", b"mock"),
    (b"stub", b"stub"),
    (b"assert", b"assertion"),
    (b"log", b"logging"),
    (b"lvl", b"level"),
    (b"dbg", b"debug"),
    (b"wrn", b"warning"),
    (b"inf", b"info"),
    (b"ts", b"timestamp"),
    (b"dur", b"duration"),
    (b"ttl", b"timetolive"),
    (b"ver", b"version"),
    (b"ns", b"namespace"),
    (b"pkg", b"package"),
    (b"mod", b"module"),
    (b"lib", b"library"),
    (b"dep", b"dependency"),
    (b"ref", b"reference"),
    (b"ptr", b"pointer"),
    (b"obj", b"object"),
    (b"doc", b"document"),
    (b"cmd", b"command"),
    (b"ops", b"operations"),
    (b"util", b"utility"),
    (b"hlp", b"helper"),
    (b"ext", b"extension"),
];

fn is_delimiter(byte: u8) -> bool {
    matches!(
        byte,
        b'.' | b'/' | b'_' | b'-' | b' ' | b'(' | b')' | b',' | b':'
    )
}

fn is_ascii_alnum(byte: u8) -> bool {
    byte.is_ascii_alphanumeric()
}

fn ascii_lower(byte: u8) -> u8 {
    if byte.is_ascii_uppercase() {
        byte + (b'a' - b'A')
    } else {
        byte
    }
}

fn duplicate_for_c(bytes: &[u8]) -> *mut c_char {
    let ptr = unsafe { malloc(bytes.len() + 1) } as *mut c_char;
    if ptr.is_null() {
        return ptr;
    }
    unsafe {
        ptr::copy_nonoverlapping(bytes.as_ptr() as *const c_char, ptr, bytes.len());
        *ptr.add(bytes.len()) = 0;
    }
    ptr
}

unsafe fn flush_token(
    buffer: &mut [u8; TOKEN_BUF_LEN],
    length: &mut usize,
    out: *mut *mut c_char,
    count: &mut usize,
    max_out: usize,
) {
    if *length > 0 && *count < max_out {
        *out.add(*count) = duplicate_for_c(&buffer[..*length]);
        *count += 1;
    }
    *length = 0;
}

fn abbreviation(token: &[u8]) -> Option<&'static [u8]> {
    ABBREVIATIONS
        .iter()
        .find(|(abbrev, _)| *abbrev == token)
        .map(|(_, expanded)| *expanded)
}

/// 依 C 契約拆解 name 並配置由 C `free` 回收的 token strings。
///
/// # Safety
/// `name` 若非 NULL，必須指向有效的 NUL 結尾 C string；`out` 若非 NULL，
/// 必須指向至少 `max_out` 個可寫入的 `char*` slots。回傳的非 NULL pointers
/// 必須由 C allocator 回收。
#[no_mangle]
pub unsafe extern "C" fn cbm_sem_tokenize(
    name: *const c_char,
    out: *mut *mut c_char,
    max_out: c_int,
) -> c_int {
    if name.is_null() || out.is_null() || max_out <= 0 {
        return 0;
    }
    let max_out = max_out as usize;
    let bytes = CStr::from_ptr(name).to_bytes();
    let mut buffer = [0_u8; TOKEN_BUF_LEN];
    let mut length = 0_usize;
    let mut count = 0_usize;

    for (index, &byte) in bytes.iter().enumerate() {
        if count >= max_out {
            break;
        }
        let split = is_delimiter(byte);
        let camel = index > 0 && byte.is_ascii_uppercase() && bytes[index - 1].is_ascii_lowercase();
        if split || camel {
            flush_token(&mut buffer, &mut length, out, &mut count, max_out);
            if split {
                continue;
            }
        }
        if length < TOKEN_BUF_LEN - 1 && is_ascii_alnum(byte) {
            buffer[length] = ascii_lower(byte);
            length += 1;
        }
    }
    flush_token(&mut buffer, &mut length, out, &mut count, max_out);

    let original_count = count;
    for index in 0..original_count {
        if count >= max_out {
            break;
        }
        let token = *out.add(index);
        if token.is_null() {
            continue;
        }
        let token = CStr::from_ptr(token).to_bytes();
        if let Some(expanded) = abbreviation(token) {
            *out.add(count) = duplicate_for_c(expanded);
            count += 1;
        }
    }
    count as c_int
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;

    unsafe extern "C" {
        fn free(ptr: *mut c_void);
    }

    unsafe fn free_tokens(tokens: &mut [*mut c_char], count: usize) {
        for token in tokens.iter_mut().take(count) {
            if !token.is_null() {
                free(*token as *mut c_void);
                *token = ptr::null_mut();
            }
        }
    }

    unsafe fn read_tokens(tokens: &[*mut c_char], count: usize) -> Vec<String> {
        tokens
            .iter()
            .take(count)
            .map(|token| CStr::from_ptr(*token).to_string_lossy().into_owned())
            .collect()
    }

    #[test]
    fn splits_camel_and_delimiter_tokens() {
        let name = CString::new("getHTTPResponse_ctx").expect("literal has no NUL");
        let mut tokens = [ptr::null_mut(); 16];
        let count = unsafe { cbm_sem_tokenize(name.as_ptr(), tokens.as_mut_ptr(), 16) } as usize;
        let values = unsafe { read_tokens(&tokens, count) };
        assert_eq!(values, ["get", "httpresponse", "ctx", "context"]);
        unsafe { free_tokens(&mut tokens, count) };
    }

    #[test]
    fn keeps_ascii_alnum_and_expands_abbreviations() {
        let name = CString::new("err-42").expect("literal has no NUL");
        let mut tokens = [ptr::null_mut(); 8];
        let count = unsafe { cbm_sem_tokenize(name.as_ptr(), tokens.as_mut_ptr(), 8) } as usize;
        let values = unsafe { read_tokens(&tokens, count) };
        assert_eq!(values, ["err", "42", "error"]);
        unsafe { free_tokens(&mut tokens, count) };
    }

    #[test]
    fn respects_output_capacity_and_null_inputs() {
        let name = CString::new("one_two_three").expect("literal has no NUL");
        let mut tokens = [ptr::null_mut(); 2];
        assert_eq!(
            unsafe { cbm_sem_tokenize(name.as_ptr(), tokens.as_mut_ptr(), 2) },
            2
        );
        unsafe { free_tokens(&mut tokens, 2) };
        assert_eq!(
            unsafe { cbm_sem_tokenize(ptr::null(), tokens.as_mut_ptr(), 2) },
            0
        );
    }
}
