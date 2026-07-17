//! `semantic.c` 的 module-proximity direct-only replacement。
//!
//! 這個 helper 只依賴兩個 C string，計算共同目錄 prefix 的 ranking boost；
//! semantic corpus、vector 與 graph processing 仍由 C 提供。

#![allow(unsafe_code)]

use std::ffi::CStr;
use std::os::raw::{c_char, c_float};

const PROX_MAX_BOOST: c_float = 0.10;

/// 計算兩個路徑的共同目錄 proximity multiplier。
///
/// # Safety
/// `path_a` 與 `path_b` 若非 NULL，必須各自指向有效的 NUL 結尾 C string。
#[no_mangle]
pub unsafe extern "C" fn cbm_sem_proximity(
    path_a: *const c_char,
    path_b: *const c_char,
) -> c_float {
    if path_a.is_null() || path_b.is_null() {
        return 1.0;
    }
    let path_a = CStr::from_ptr(path_a).to_bytes();
    let path_b = CStr::from_ptr(path_b).to_bytes();
    let shared = path_a
        .iter()
        .zip(path_b.iter())
        .take_while(|(left, right)| left == right)
        .filter(|(byte, _)| **byte == b'/')
        .count();
    let total_a = path_a.iter().filter(|byte| **byte == b'/').count();
    let total_b = path_b.iter().filter(|byte| **byte == b'/').count();
    let max_total = total_a.max(total_b);
    if max_total == 0 {
        return 1.0;
    }
    1.0 + (shared as c_float / max_total as c_float) * PROX_MAX_BOOST
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;

    #[test]
    fn handles_null_and_flat_paths() {
        let path = CString::new("file.c").expect("literal has no NUL");
        assert_eq!(
            unsafe { cbm_sem_proximity(std::ptr::null(), path.as_ptr()) },
            1.0
        );
        let other = CString::new("other.c").expect("literal has no NUL");
        assert_eq!(
            unsafe { cbm_sem_proximity(path.as_ptr(), other.as_ptr()) },
            1.0
        );
    }

    #[test]
    fn rewards_shared_directory_prefix() {
        let left = CString::new("/repo/src/left.c").expect("literal has no NUL");
        let right = CString::new("/repo/lib/right.c").expect("literal has no NUL");
        let score = unsafe { cbm_sem_proximity(left.as_ptr(), right.as_ptr()) };
        assert!((score - 1.066_666_7).abs() < 1.0e-6);
    }

    #[test]
    fn same_path_reaches_maximum_boost() {
        let path = CString::new("/repo/src/file.c").expect("literal has no NUL");
        assert!((unsafe { cbm_sem_proximity(path.as_ptr(), path.as_ptr()) } - 1.1).abs() < 1.0e-6);
    }
}
