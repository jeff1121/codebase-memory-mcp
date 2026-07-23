//! Store `safe_props` 的第一個 byte 保護 helper。

use std::os::raw::c_char;

static SAFE_PROPS_EMPTY: [c_char; 3] = [b'{' as c_char, b'}' as c_char, 0];

/// 若 `s` 為 null 或第一個 byte 為 NUL，回傳 process-lifetime 的 `"{}"`；否則原樣借用
/// 輸入指標。此 helper 不掃描、保存、配置或修改輸入。
///
/// # Safety
///
/// 非 null 的 `s` 必須至少可讀取第一個 byte。
#[must_use]
pub unsafe fn safe_props(s: *const c_char) -> *const c_char {
    if s.is_null() || unsafe { *s } == 0 {
        SAFE_PROPS_EMPTY.as_ptr()
    } else {
        s
    }
}

#[cfg(test)]
mod tests {
    use super::safe_props;
    use std::os::raw::c_char;
    use std::ptr;

    #[test]
    fn safe_props_preserves_first_byte_and_borrowed_pointer_contract() {
        let static_result = unsafe { safe_props(ptr::null()) };
        assert!(!static_result.is_null());
        assert_eq!(
            unsafe { std::slice::from_raw_parts(static_result.cast::<u8>(), 3) },
            b"{}\0"
        );

        let empty = [0, b'x' as c_char, 0];
        let empty_result = unsafe { safe_props(empty.as_ptr()) };
        assert_eq!(
            unsafe { std::slice::from_raw_parts(empty_result.cast::<u8>(), 3) },
            b"{}\0"
        );

        let nonempty = [b'x' as c_char, 0];
        assert_eq!(unsafe { safe_props(nonempty.as_ptr()) }, nonempty.as_ptr());

        let embedded_nul = [
            b'x' as c_char,
            0,
            b't' as c_char,
            b'a' as c_char,
            b'i' as c_char,
            0,
        ];
        assert_eq!(
            unsafe { safe_props(embedded_nul.as_ptr()) },
            embedded_nul.as_ptr()
        );

        let non_utf8 = [0xff_u8 as c_char, 0];
        assert_eq!(unsafe { safe_props(non_utf8.as_ptr()) }, non_utf8.as_ptr());
    }
}
