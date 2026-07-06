//! `src/store/store.c` architecture/impact pure helpers 的 Rust parity/opt-in helper。
//!
//! 這裡只處理 byte-level QN/path/scalar contract；SQLite architecture query、
//! summary allocation 與 store lifetime 仍由 C store runtime 負責。

const QN_BUF_MAX: usize = 255;

#[must_use]
pub fn qn_to_package(qn: Option<&[u8]>) -> Vec<u8> {
    let Some(qn) = qn else {
        return Vec::new();
    };
    if qn.is_empty() {
        return Vec::new();
    }

    let dots = dot_positions(qn);
    if dots.len() >= 3 {
        let start = dots[1] + 1;
        let end = dots[2];
        if let Some(segment) = bounded_segment(qn, start, end) {
            return segment.to_vec();
        }
    }

    if !dots.is_empty() {
        let start = dots[0] + 1;
        let end = dots.get(1).copied().unwrap_or(qn.len());
        if let Some(segment) = bounded_segment(qn, start, end) {
            return segment.to_vec();
        }
    }

    Vec::new()
}

#[must_use]
pub fn qn_to_top_package(qn: Option<&[u8]>) -> Vec<u8> {
    let Some(qn) = qn else {
        return Vec::new();
    };
    if qn.is_empty() {
        return Vec::new();
    }

    let Some(first_dot) = qn.iter().position(|&byte| byte == b'.') else {
        return Vec::new();
    };
    let start = first_dot + 1;
    let end = qn[start..]
        .iter()
        .position(|&byte| byte == b'.')
        .map_or(qn.len(), |offset| start + offset);
    bounded_segment(qn, start, end).map_or_else(Vec::new, <[u8]>::to_vec)
}

#[must_use]
pub fn is_test_file_path(path: Option<&[u8]>) -> bool {
    let Some(path) = path else {
        return false;
    };
    !path.is_empty() && path.windows(4).any(|window| window == b"test")
}

#[must_use]
pub fn hop_to_risk(hop: i32) -> i32 {
    match hop {
        1 => 0,
        2 => 1,
        3 => 2,
        _ => 3,
    }
}

#[must_use]
pub fn risk_label(level: i32) -> &'static [u8] {
    match level {
        0 => b"CRITICAL",
        1 => b"HIGH",
        2 => b"MEDIUM",
        _ => b"LOW",
    }
}

fn dot_positions(qn: &[u8]) -> Vec<usize> {
    qn.iter()
        .enumerate()
        .filter_map(|(index, &byte)| (byte == b'.').then_some(index))
        .take(5)
        .collect()
}

fn bounded_segment(qn: &[u8], start: usize, end: usize) -> Option<&[u8]> {
    if start > end || end > qn.len() {
        return None;
    }
    let len = end - start;
    if len > 0 && len <= QN_BUF_MAX {
        Some(&qn[start..end])
    } else {
        None
    }
}

/// `src/store/store.c` `arch_path_prepare` 的 norm_out 正規化 parity。
///
/// 對應 C 行為：跳過前導空白（space/tab/\n/\r）、去一次前導 `./`、去前導 `/`，
/// 以 `cap - 1` 上限截斷（對齊 C `strncpy(norm_out, path, norm_sz - 1)`），再去尾端
/// ` `/`\t`/`/` 並折疊重複 `/`。path 未設定（null/全空白）或正規化後為空時回傳
/// `None`（對應 C 回傳 false）。截斷刻意先於 strip/collapse，以完全對齊 C。
#[must_use]
pub fn normalize_arch_path(path: Option<&[u8]>, cap: usize) -> Option<Vec<u8>> {
    let path = path?;
    if cap == 0 {
        return None;
    }
    let is_ws = |b: u8| matches!(b, b' ' | b'\t' | b'\n' | b'\r');

    let mut start = 0;
    while start < path.len() && is_ws(path[start]) {
        start += 1;
    }
    if start >= path.len() {
        return None;
    }
    let mut rest = &path[start..];
    if rest.starts_with(b"./") {
        rest = &rest[2..];
    }
    let mut lead = 0;
    while lead < rest.len() && rest[lead] == b'/' {
        lead += 1;
    }
    rest = &rest[lead..];
    if rest.is_empty() {
        return None;
    }

    let limit = cap - 1;
    let mut buf: Vec<u8> = rest.iter().take(limit).copied().collect();

    while matches!(buf.last(), Some(b' ' | b'\t' | b'/')) {
        buf.pop();
    }

    let mut out: Vec<u8> = Vec::with_capacity(buf.len());
    for &b in &buf {
        if b == b'/' && out.last() == Some(&b'/') {
            continue;
        }
        out.push(b);
    }
    if out.is_empty() {
        return None;
    }
    Some(out)
}

/// `src/store/store.c` `file_ext` 的 parity：取 path 最後一個 `.` 起的副檔名
/// （含 `.`），轉小寫（ASCII）。無 `.`、或副檔名長度 >= `cap`（對齊 C 的
/// `len >= sizeof(buf)`，非截斷而是拒絕）時回傳 `None`。
#[must_use]
pub fn file_ext_lower(path: Option<&[u8]>, cap: usize) -> Option<Vec<u8>> {
    let path = path?;
    let dot = path.iter().rposition(|&b| b == b'.')?;
    let ext = &path[dot..];
    if ext.len() >= cap {
        return None;
    }
    Some(ext.iter().map(|&b| b.to_ascii_lowercase()).collect())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn text(bytes: &[u8]) -> &str {
        std::str::from_utf8(bytes).unwrap()
    }

    #[test]
    fn normalize_arch_path_matches_c_contract() {
        let n = |p: &[u8]| normalize_arch_path(Some(p), 512);
        assert_eq!(n(b"apps/foo"), Some(b"apps/foo".to_vec()));
        assert_eq!(n(b"./apps/foo"), Some(b"apps/foo".to_vec()));
        assert_eq!(n(b"/apps/foo"), Some(b"apps/foo".to_vec()));
        assert_eq!(n(b".//apps"), Some(b"apps".to_vec()));
        assert_eq!(n(b"  \tapps/foo"), Some(b"apps/foo".to_vec()));
        assert_eq!(n(b"apps/foo/"), Some(b"apps/foo".to_vec()));
        assert_eq!(n(b"apps/foo  "), Some(b"apps/foo".to_vec()));
        assert_eq!(n(b"apps/foo\t/"), Some(b"apps/foo".to_vec()));
        assert_eq!(n(b"apps//foo"), Some(b"apps/foo".to_vec()));
        assert_eq!(n(b"apps///foo//bar"), Some(b"apps/foo/bar".to_vec()));
        assert_eq!(normalize_arch_path(None, 512), None);
        assert_eq!(n(b""), None);
        assert_eq!(n(b"   "), None);
        assert_eq!(n(b"./"), None);
        assert_eq!(n(b"///"), None);
        assert_eq!(n(b"./ "), None);
        // 截斷先於 strip/collapse
        assert_eq!(
            normalize_arch_path(Some(b"abcdef"), 5),
            Some(b"abcd".to_vec())
        );
        assert_eq!(
            normalize_arch_path(Some(b"abc/def"), 5),
            Some(b"abc".to_vec())
        );
        assert_eq!(normalize_arch_path(Some(b"abc"), 0), None);
        assert_eq!(normalize_arch_path(Some(b"abc"), 1), None);
    }

    #[test]
    fn file_ext_lower_matches_c_contract() {
        let f = |p: &[u8]| file_ext_lower(Some(p), 16);
        assert_eq!(f(b"main.py"), Some(b".py".to_vec()));
        assert_eq!(f(b"App.JAVA"), Some(b".java".to_vec()));
        assert_eq!(f(b"a.TAR.GZ"), Some(b".gz".to_vec()));
        assert_eq!(f(b"Makefile"), None);
        assert_eq!(file_ext_lower(None, 16), None);
        assert_eq!(f(b".gitignore"), Some(b".gitignore".to_vec()));
        assert_eq!(f(b"noext."), Some(b".".to_vec()));
        // 長度 >= cap 拒絕（對齊 C `len >= sizeof(buf)`）
        assert_eq!(file_ext_lower(Some(b"a.abcdefghijklmno"), 16), None);
        assert_eq!(
            file_ext_lower(Some(b"a.abcdefghijklmn"), 16),
            Some(b".abcdefghijklmn".to_vec())
        );
    }

    #[test]
    fn qn_package_helpers_match_c_contract() {
        assert_eq!(
            text(&qn_to_package(Some(
                b"project.internal.store.search.Search"
            ))),
            "store"
        );
        assert_eq!(
            text(&qn_to_package(Some(b"project.src.utils.helper.foo"))),
            "utils"
        );
        assert_eq!(text(&qn_to_package(Some(b"project.main.foo"))), "main");
        assert_eq!(text(&qn_to_package(Some(b"project.cmd"))), "cmd");
        assert_eq!(text(&qn_to_package(Some(b"standalone"))), "");
        assert_eq!(text(&qn_to_package(Some(b""))), "");
        assert_eq!(text(&qn_to_package(None)), "");

        assert_eq!(
            text(&qn_to_top_package(Some(
                b"project.internal.store.search.Search"
            ))),
            "internal"
        );
        assert_eq!(
            text(&qn_to_top_package(Some(b"project.src.components.Button"))),
            "src"
        );
        assert_eq!(text(&qn_to_top_package(Some(b"project.cmd"))), "cmd");
        assert_eq!(text(&qn_to_top_package(Some(b"standalone"))), "");
        assert_eq!(text(&qn_to_top_package(Some(b""))), "");
        assert_eq!(text(&qn_to_top_package(None)), "");
    }

    #[test]
    fn qn_package_helpers_keep_buffer_boundaries() {
        let ok_segment = vec![b'a'; QN_BUF_MAX];
        let ok_qn = [
            b"project.dir.".as_slice(),
            ok_segment.as_slice(),
            b".Func".as_slice(),
        ]
        .concat();
        assert_eq!(qn_to_package(Some(&ok_qn)), ok_segment);

        let too_long_segment = vec![b'a'; QN_BUF_MAX + 1];
        let too_long_qn = [
            b"project.dir.".as_slice(),
            too_long_segment.as_slice(),
            b".Func".as_slice(),
        ]
        .concat();
        assert_eq!(qn_to_package(Some(&too_long_qn)), b"dir");

        let top_qn = [
            b"project.".as_slice(),
            ok_segment.as_slice(),
            b".Func".as_slice(),
        ]
        .concat();
        assert_eq!(qn_to_top_package(Some(&top_qn)), ok_segment);
    }

    #[test]
    fn test_file_predicate_matches_substring_contract() {
        assert!(is_test_file_path(Some(b"test_handler.py")));
        assert!(is_test_file_path(Some(b"src/__tests__/handler.js")));
        assert!(is_test_file_path(Some(b"testdata/fixture.json")));
        assert!(is_test_file_path(Some(b"contest/file.c")));
        assert!(!is_test_file_path(Some(b"handler.spec.ts")));
        assert!(!is_test_file_path(Some(b"TestOnly.java")));
        assert!(!is_test_file_path(Some(b"")));
        assert!(!is_test_file_path(None));
    }

    #[test]
    fn risk_helpers_match_c_contract() {
        assert_eq!(hop_to_risk(1), 0);
        assert_eq!(hop_to_risk(2), 1);
        assert_eq!(hop_to_risk(3), 2);
        assert_eq!(hop_to_risk(0), 3);
        assert_eq!(hop_to_risk(100), 3);
        assert_eq!(hop_to_risk(-1), 3);

        assert_eq!(risk_label(0), b"CRITICAL");
        assert_eq!(risk_label(1), b"HIGH");
        assert_eq!(risk_label(2), b"MEDIUM");
        assert_eq!(risk_label(3), b"LOW");
        assert_eq!(risk_label(99), b"LOW");
    }
}
