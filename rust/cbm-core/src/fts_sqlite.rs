#![allow(unsafe_code)]

//! SQLite FTS callback 的 direct ABI，刻意與通用 FFI export 分離。

#[repr(C)]
pub struct CbmSqliteContext {
    _private: [u8; 0],
}

#[repr(C)]
pub struct CbmSqliteValue {
    _private: [u8; 0],
}

unsafe extern "C" {
    fn sqlite3_value_text(value: *mut CbmSqliteValue) -> *const u8;
    fn sqlite3_result_text(
        ctx: *mut CbmSqliteContext,
        value: *const std::os::raw::c_char,
        len: std::os::raw::c_int,
        destructor: Option<unsafe extern "C" fn(*mut std::ffi::c_void)>,
    );
    fn sqlite3_result_error_nomem(ctx: *mut CbmSqliteContext);
    fn sqlite3_malloc(size: std::os::raw::c_int) -> *mut std::ffi::c_void;
    fn sqlite3_free(ptr: *mut std::ffi::c_void);
}

const CBM_SQLITE_EMPTY_TEXT: &[u8] = b"\0";

unsafe fn cbm_sqlite_result_text_owned(ctx: *mut CbmSqliteContext, text: &[u8]) {
    if ctx.is_null() {
        return;
    }
    let Ok(len) = std::os::raw::c_int::try_from(text.len()) else {
        unsafe { sqlite3_result_error_nomem(ctx) };
        return;
    };
    if text.is_empty() {
        unsafe {
            sqlite3_result_text(ctx, CBM_SQLITE_EMPTY_TEXT.as_ptr().cast(), 0, None);
        }
        return;
    }

    let allocation = unsafe { sqlite3_malloc(len) };
    if allocation.is_null() {
        unsafe { sqlite3_result_error_nomem(ctx) };
        return;
    }
    unsafe {
        std::ptr::copy_nonoverlapping(text.as_ptr(), allocation.cast::<u8>(), text.len());
        sqlite3_result_text(ctx, allocation.cast(), len, Some(sqlite3_free));
    }
}

/// # Safety
///
/// `ctx` 與 `argv` 必須由 SQLite 在純量 callback 的有效期間提供。
#[no_mangle]
pub unsafe extern "C" fn cbm_store_sqlite_camel_split(
    ctx: *mut CbmSqliteContext,
    argc: std::os::raw::c_int,
    argv: *mut *mut CbmSqliteValue,
) {
    let input = if argc < 1 || argv.is_null() {
        None
    } else {
        let value = unsafe { *argv };
        let text = unsafe { sqlite3_value_text(value) };
        if text.is_null() {
            None
        } else {
            Some(unsafe { std::ffi::CStr::from_ptr(text.cast()).to_bytes() })
        }
    };
    let output = crate::store::tokenization::camel_split_bytes(input);
    unsafe { cbm_sqlite_result_text_owned(ctx, &output) };
}
