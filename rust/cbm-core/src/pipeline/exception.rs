//! pipeline exception-name classifier。
//!
//! 這只固定 `is_checked_exception()` 的 byte-level contract；實際 THROWS /
//! RAISES 邊與 graph mutation 仍由 C pipeline passes 負責。

#[must_use]
pub fn is_checked_exception(name: Option<&[u8]>) -> bool {
    let Some(name) = name else {
        return false;
    };
    !contains_any(name, [b"Error", b"Panic", b"error", b"panic"])
}

fn contains_any<const N: usize>(haystack: &[u8], needles: [&[u8]; N]) -> bool {
    needles.iter().any(|needle| {
        !needle.is_empty()
            && haystack
                .windows(needle.len())
                .any(|window| window == *needle)
    })
}

#[cfg(test)]
mod tests {
    use super::is_checked_exception;

    #[test]
    fn is_checked_exception_matches_c_contract() {
        assert!(!is_checked_exception(None));
        assert!(is_checked_exception(Some(b"")));
        assert!(is_checked_exception(Some(b"NotFoundException")));
        assert!(!is_checked_exception(Some(b"NotFoundError")));
        assert!(!is_checked_exception(Some(b"panic")));
        assert!(!is_checked_exception(Some(b"ClientPanic")));
        assert!(!is_checked_exception(Some(b"custom error")));
        assert!(is_checked_exception(Some(b"CheckedThing")));
    }
}
