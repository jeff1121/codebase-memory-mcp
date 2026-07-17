//! `src/pipeline/pass_gitdiff.c` 的 range parser parity。
//!
//! 只處理 unified diff hunk 使用的 `start,count` / `start` 純 scalar parser；
//! name-status/hunk 結構寫入、固定 C buffer、git diff I/O 與 pipeline side effects 仍由 C 負責。

const NAME_STATUS_LINE_BUF: usize = 1536;
const PATH_BUF: usize = 512;

#[derive(Debug, PartialEq, Eq)]
pub struct ParsedNameStatus {
    pub status: u8,
    pub path: Vec<u8>,
    pub old_path: Vec<u8>,
}

#[derive(Debug, PartialEq, Eq)]
pub struct ParsedHunk {
    pub path: Vec<u8>,
    pub start_line: i32,
    pub end_line: i32,
}

#[must_use]
pub fn parse_range(input: Option<&[u8]>) -> (i32, i32) {
    let Some(input) = input else {
        return (0, 1);
    };
    let Some(comma) = input.iter().position(|byte| *byte == b',') else {
        return (parse_strtol_i32(input), 1);
    };
    (
        parse_strtol_i32(&input[..comma]),
        parse_strtol_i32(&input[comma + 1..]),
    )
}

/// 解析 `git diff --name-status` 輸出，不執行 git 或任何 graph side effect。
#[must_use]
pub fn parse_name_status(input: Option<&[u8]>, max_out: usize) -> Vec<ParsedNameStatus> {
    let Some(input) = input else {
        return Vec::new();
    };
    if max_out == 0 {
        return Vec::new();
    }

    let mut result = Vec::new();
    for line in input.split(|byte| *byte == b'\n') {
        if result.len() >= max_out || line.is_empty() {
            continue;
        }
        let line = &line[..line.len().min(NAME_STATUS_LINE_BUF - 1)];
        let Some(tab1) = line.iter().position(|byte| *byte == b'\t') else {
            continue;
        };
        let status = line.first().copied().unwrap_or_default();
        let path1 = &line[tab1 + 1..];
        let (path1, path2) = path1
            .iter()
            .position(|byte| *byte == b'\t')
            .map_or((path1, None), |tab2| {
                (&path1[..tab2], Some(&path1[tab2 + 1..]))
            });
        let (path, old_path) = if status == b'R' {
            (path2.unwrap_or(path1), path1)
        } else {
            (path1, &[][..])
        };
        let path = truncate_c_string(path);
        let old_path = truncate_c_string(old_path);
        if !super::githistory::is_trackable_file(Some(&path)) {
            continue;
        }
        result.push(ParsedNameStatus {
            status,
            path,
            old_path,
        });
    }
    result
}

fn truncate_c_string(value: &[u8]) -> Vec<u8> {
    value[..value.len().min(PATH_BUF - 1)].to_vec()
}

/// 解析 `git diff --unified=0` 的 hunk header，不執行 git 或 graph side effect。
#[must_use]
pub fn parse_hunks(input: Option<&[u8]>, max_out: usize) -> Vec<ParsedHunk> {
    let Some(input) = input else {
        return Vec::new();
    };
    if max_out == 0 {
        return Vec::new();
    }

    let mut result = Vec::new();
    let mut current_file = Vec::new();
    for line in input.split(|byte| *byte == b'\n') {
        if result.len() >= max_out || line.is_empty() {
            continue;
        }
        if line.len() > 6 && line.starts_with(b"+++ b/") {
            current_file = truncate_c_string(&line[6..]);
            continue;
        }
        if line.len() < 2 || line[0] != b'@' || line[1] != b'@' || current_file.is_empty() {
            continue;
        }
        let Some(plus) = line.iter().position(|byte| *byte == b'+') else {
            continue;
        };
        if plus == 0 {
            continue;
        }
        let range_end = line[plus..]
            .windows(3)
            .position(|window| window == b" @@")
            .unwrap_or(line.len() - plus);
        let range = &line[plus + 1..plus + range_end];
        let (start, count) = parse_range(Some(range));
        if start <= 0 || !super::githistory::is_trackable_file(Some(&current_file)) {
            continue;
        }
        let end_line = start.saturating_add(count).saturating_sub(1).max(start);
        result.push(ParsedHunk {
            path: current_file.clone(),
            start_line: start,
            end_line,
        });
    }
    result
}

fn parse_i32_prefix(input: &[u8]) -> Option<(i32, &[u8])> {
    let mut end = 0usize;
    let mut sign = 1i64;
    if input.first() == Some(&b'-') {
        sign = -1;
        end = 1;
    } else if input.first() == Some(&b'+') {
        end = 1;
    }
    let digits_start = end;
    while end < input.len() && input[end].is_ascii_digit() {
        end += 1;
    }
    if end == digits_start {
        return None;
    }
    let value = input[digits_start..end]
        .iter()
        .fold(0i64, |value, digit| value * 10 + i64::from(digit - b'0'))
        .saturating_mul(sign)
        .clamp(i64::from(i32::MIN), i64::from(i32::MAX)) as i32;
    Some((value, &input[end..]))
}

fn parse_strtol_i32(input: &[u8]) -> i32 {
    let input = input
        .iter()
        .position(|byte| !byte.is_ascii_whitespace())
        .map_or(&[][..], |start| &input[start..]);
    parse_i32_prefix(input).map_or(0, |(value, _)| value)
}

#[cfg(test)]
mod tests {
    use super::{
        parse_hunks, parse_name_status, parse_range, ParsedHunk, ParsedNameStatus, PATH_BUF,
    };

    #[test]
    fn parses_start_and_count() {
        assert_eq!(parse_range(Some(b"10,5")), (10, 5));
        assert_eq!(parse_range(Some(b"52,2")), (52, 2));
    }

    #[test]
    fn defaults_missing_count_to_one() {
        assert_eq!(parse_range(Some(b"10")), (10, 1));
        assert_eq!(parse_range(Some(b" 42\t")), (42, 1));
    }

    #[test]
    fn keeps_zero_and_signed_values() {
        assert_eq!(parse_range(Some(b"1,0")), (1, 0));
        assert_eq!(parse_range(Some(b"+7,-2")), (7, -2));
    }

    #[test]
    fn parses_each_comma_separated_part_like_strtol() {
        assert_eq!(parse_range(Some(b"1, 1")), (1, 1));
        assert_eq!(parse_range(Some(b"x,5")), (0, 5));
    }

    #[test]
    fn invalid_or_null_input_matches_safe_abi_fallback() {
        assert_eq!(parse_range(Some(b"not-a-range")), (0, 1));
        assert_eq!(parse_range(None), (0, 1));
    }

    #[test]
    fn parses_and_filters_name_status_entries() {
        assert_eq!(
            parse_name_status(
                Some(b"M\tinternal/store/nodes.go\nA\tpackage-lock.json\nR100\tsrc/old.go\tsrc/new.go\n"),
                16,
            ),
            vec![
                ParsedNameStatus {
                    status: b'M',
                    path: b"internal/store/nodes.go".to_vec(),
                    old_path: Vec::new(),
                },
                ParsedNameStatus {
                    status: b'R',
                    path: b"src/new.go".to_vec(),
                    old_path: b"src/old.go".to_vec(),
                },
            ]
        );
    }

    #[test]
    fn respects_limits_and_safe_inputs() {
        assert!(parse_name_status(None, 16).is_empty());
        assert!(parse_name_status(Some(b"M\tmain.go\n"), 0).is_empty());
        assert_eq!(
            parse_name_status(Some(b"M\tmain.go\nA\tother.go\n"), 1).len(),
            1
        );
    }

    #[test]
    fn filters_name_status_after_c_buffer_truncation() {
        let mut input = b"M\t".to_vec();
        input.extend(std::iter::repeat(b'a').take(508));
        input.extend_from_slice(b".png\n");

        let entries = parse_name_status(Some(&input), 1);
        assert_eq!(entries.len(), 1);
        assert_eq!(entries[0].path.len(), PATH_BUF - 1);
        assert!(entries[0].path.ends_with(b".pn"));
    }

    #[test]
    fn parses_hunks_and_filters_untrackable_files() {
        let input = b"+++ b/main.go\n@@ -10,3 +10,5 @@ func main() {\n+++ b/binary.png\n@@ -1 +1 @@ ignored\n+++ b/utils.go\n@@ -50,0 +52,2 @@ func helper() {\n";
        assert_eq!(
            parse_hunks(Some(input), 16),
            vec![
                ParsedHunk {
                    path: b"main.go".to_vec(),
                    start_line: 10,
                    end_line: 14,
                },
                ParsedHunk {
                    path: b"utils.go".to_vec(),
                    start_line: 52,
                    end_line: 53,
                },
            ]
        );
    }

    #[test]
    fn handles_hunk_limits_and_deletions() {
        assert!(parse_hunks(None, 16).is_empty());
        assert!(parse_hunks(Some(b"+++ b/file.go\n@@ -10,3 +10,0 @@\n"), 0).is_empty());
        assert_eq!(
            parse_hunks(Some(b"+++ b/file.go\n@@ -10,3 +10,0 @@\n"), 16),
            vec![ParsedHunk {
                path: b"file.go".to_vec(),
                start_line: 10,
                end_line: 10,
            }]
        );
    }
}
