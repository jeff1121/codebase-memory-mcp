#![allow(unsafe_code)]

//! `src/pipeline/similarity.c` 副檔名借用指標 helper 的 Rust 實作。

use core::ffi::c_char;
use std::ffi::CStr;

static EMPTY_C_STRING: [u8; 1] = [0];

fn empty_c_string() -> *const c_char {
    EMPTY_C_STRING.as_ptr().cast()
}

unsafe fn similarity_file_ext(path: *const c_char) -> *const c_char {
    if path.is_null() {
        return empty_c_string();
    }

    // SAFETY: 非 NULL 呼叫者必須提供有效且 NUL 終止的 C 字串；這是既有 C ABI 前置條件。
    let bytes = unsafe { CStr::from_ptr(path) }.to_bytes();
    let Some(offset) = bytes.iter().rposition(|byte| *byte == b'.') else {
        return empty_c_string();
    };

    // SAFETY: `offset` 位於上方已驗證、由同一個 C 字串借用的 bytes 範圍內。
    unsafe { path.cast::<u8>().add(offset).cast() }
}

#[no_mangle]
pub unsafe extern "C" fn cbm_rs_pipeline_similarity_file_ext_v1(
    path: *const c_char,
) -> *const c_char {
    // SAFETY: 維持 C ABI 的指標有效性前置條件；NULL 在 helper 內不解參並回傳靜態空字串。
    unsafe { similarity_file_ext(path) }
}

#[cfg(feature = "pipeline-similarity-file-ext-only")]
#[no_mangle]
pub unsafe extern "C" fn cbm_pipeline_similarity_file_ext(path: *const c_char) -> *const c_char {
    // SAFETY: 與 v1 ABI 完全相同的指標契約。
    unsafe { similarity_file_ext(path) }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ptr;

    unsafe fn call(input: &[u8]) -> *const c_char {
        assert_eq!(input.last(), Some(&0));
        // SAFETY: 所有測試輸入均具有至少一個結尾 NUL。
        unsafe { similarity_file_ext(input.as_ptr().cast()) }
    }

    fn assert_static_empty(result: *const c_char) {
        assert_eq!(result, empty_c_string());
        // SAFETY: 靜態空字串永久有效且以 NUL 終止。
        assert_eq!(unsafe { CStr::from_ptr(result) }.to_bytes(), b"");
    }

    #[test]
    fn similarity_file_ext_preserves_c_byte_and_borrowed_pointer_contract() {
        // SAFETY: NULL 是 helper 明確處理的 ABI 輸入。
        assert_static_empty(unsafe { similarity_file_ext(ptr::null()) });

        let empty = b"\0";
        let plain = b"plain\0";
        // SAFETY: 測試資料皆為 NUL 終止 C 字串。
        assert_static_empty(unsafe { call(empty) });
        // SAFETY: 測試資料皆為 NUL 終止 C 字串。
        assert_static_empty(unsafe { call(plain) });

        let multiple = b"archive.tar.gz\0";
        // SAFETY: 測試資料皆為 NUL 終止 C 字串。
        let multiple_result = unsafe { call(multiple) };
        // SAFETY: offset 11 位於 `multiple` 的可讀範圍內。
        assert_eq!(multiple_result, unsafe { multiple.as_ptr().add(11).cast() });
        // SAFETY: 回傳指標仍指向原本 NUL 終止輸入。
        assert_eq!(
            unsafe { CStr::from_ptr(multiple_result) }.to_bytes(),
            b".gz"
        );

        let dotfile = b".env\0";
        // SAFETY: 測試資料皆為 NUL 終止 C 字串。
        let dotfile_result = unsafe { call(dotfile) };
        assert_eq!(dotfile_result, dotfile.as_ptr().cast());

        let tail_dot = b"name.\0";
        // SAFETY: 測試資料皆為 NUL 終止 C 字串。
        let tail_dot_result = unsafe { call(tail_dot) };
        // SAFETY: offset 4 位於 `tail_dot` 的可讀範圍內。
        assert_eq!(tail_dot_result, unsafe { tail_dot.as_ptr().add(4).cast() });

        let separator_dot = b"dir.name/file\0";
        // SAFETY: 測試資料皆為 NUL 終止 C 字串。
        let separator_result = unsafe { call(separator_dot) };
        // SAFETY: offset 3 位於 `separator_dot` 的可讀範圍內。
        assert_eq!(separator_result, unsafe {
            separator_dot.as_ptr().add(3).cast()
        });

        let embedded_nul = b"before.rs\0.ignored\0";
        // SAFETY: 測試資料首個 NUL 前為有效 C 字串。
        let embedded_result = unsafe { call(embedded_nul) };
        // SAFETY: offset 6 位於首個 NUL 前的可讀範圍內。
        assert_eq!(embedded_result, unsafe {
            embedded_nul.as_ptr().add(6).cast()
        });
        // SAFETY: 回傳指標仍指向首個 NUL 終止的原輸入區段。
        assert_eq!(
            unsafe { CStr::from_ptr(embedded_result) }.to_bytes(),
            b".rs"
        );

        let non_utf8 = b"\xffname.rs\0";
        // SAFETY: 測試資料皆為 NUL 終止 C 字串。
        let non_utf8_result = unsafe { call(non_utf8) };
        // SAFETY: offset 5 位於 `non_utf8` 的可讀範圍內。
        assert_eq!(non_utf8_result, unsafe { non_utf8.as_ptr().add(5).cast() });
    }
}
