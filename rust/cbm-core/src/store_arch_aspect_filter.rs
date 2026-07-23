//! `src/store/store.c` architecture aspect 篩選的無配置 parity helper。
//!
//! FFI 邊界先處理 C 指標與 NUL-terminated string；本模組只保留計數與 byte-level
//! 比對規則，不配置、不修改輸入，也不進行 I/O。

#[must_use]
pub fn wants_aspect<'a, I>(aspects: Option<I>, aspect_count: i32, name: &[u8]) -> bool
where
    I: IntoIterator<Item = &'a [u8]>,
{
    let Some(aspects) = aspects else {
        return true;
    };
    if aspect_count == 0 {
        return true;
    }
    if aspect_count < 0 {
        return false;
    }

    aspects
        .into_iter()
        .take(aspect_count as usize)
        .any(|aspect| aspect == b"all" || aspect == name)
}

#[cfg(test)]
mod tests {
    use super::wants_aspect;

    #[test]
    fn wants_aspect_matches_c_contract() {
        let entries: [&[u8]; 2] = [b"cache", b"all"];
        assert!(wants_aspect(
            None::<std::iter::Empty<&[u8]>>,
            -1,
            b"ignored"
        ));
        assert!(wants_aspect(Some(entries.iter().copied()), 0, b"ignored"));
        assert!(!wants_aspect(Some(entries.iter().copied()), -1, b"cache"));
        assert!(wants_aspect(Some(entries.iter().copied()), 2, b"ignored"));

        let name_entry: [&[u8]; 1] = [b"service"];
        assert!(wants_aspect(
            Some(name_entry.iter().copied()),
            1,
            b"service"
        ));
        assert!(!wants_aspect(
            Some(name_entry.iter().copied()),
            1,
            b"Service"
        ));
    }
}
