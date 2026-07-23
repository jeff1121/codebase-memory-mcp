//! Store package list 的 byte-exact membership helper。

/// 在前 `count` 項中，以大小寫敏感的 bytes 精確比對 `pkg`。
///
/// `count <= 0` 時不消耗 iterator，直接回傳 `false`。
#[must_use]
pub fn contains<'a>(pkg: &[u8], count: i32, list: impl IntoIterator<Item = &'a [u8]>) -> bool {
    if count <= 0 {
        return false;
    }

    list.into_iter()
        .take(count as usize)
        .any(|entry| entry == pkg)
}

#[cfg(test)]
mod tests {
    use super::contains;

    #[test]
    fn contains_matches_c_contract() {
        let packages: &[&[u8]] = &[b"first", b"last", b"first"];

        assert!(contains(b"first", 3, packages.iter().copied()));
        assert!(contains(b"last", 3, packages.iter().copied()));
        assert!(contains(b"first", 3, packages.iter().copied()));
        assert!(!contains(b"unknown", 3, packages.iter().copied()));
        assert!(!contains(b"first", 0, packages.iter().copied()));
        assert!(!contains(b"first", -1, packages.iter().copied()));
        assert!(!contains(b"First", 3, packages.iter().copied()));
        assert!(!contains(b"fir", 3, packages.iter().copied()));
        assert!(!contains(b"irst", 3, packages.iter().copied()));
        assert!(!contains(b"last", 1, packages.iter().copied()));

        let empty: &[&[u8]] = &[b""];
        assert!(contains(b"", 1, empty.iter().copied()));
    }
}
