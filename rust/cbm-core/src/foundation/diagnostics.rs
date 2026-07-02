//! 對齊 `src/foundation/diagnostics.c` 的 deterministic formatting helpers。

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct QuerySnapshot {
    pub count: i32,
    pub errors: i32,
    pub total_us: i64,
    pub avg_us: i64,
    pub max_us: i64,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Snapshot {
    pub uptime_s: i64,
    pub rss_bytes: usize,
    pub peak_rss_bytes: usize,
    pub heap_committed_bytes: usize,
    pub peak_committed_bytes: usize,
    pub page_faults: usize,
    pub fd_count: i32,
    pub queries: QuerySnapshot,
    pub pid: i32,
}

pub fn env_enabled_value(value: Option<&[u8]>) -> bool {
    matches!(value, Some(b"1" | b"true"))
}

pub fn query_stats_snapshot_values(
    count: i32,
    errors: i32,
    total_us: i64,
    max_us: i64,
) -> QuerySnapshot {
    QuerySnapshot {
        count,
        errors,
        total_us,
        avg_us: if count > 0 {
            total_us / i64::from(count)
        } else {
            0
        },
        max_us,
    }
}

pub fn format_path(tmpdir: Option<&[u8]>, pid: i32, ext: Option<&[u8]>) -> Option<Vec<u8>> {
    let tmpdir = tmpdir?;
    let ext = ext?;
    let mut out = Vec::with_capacity(tmpdir.len() + 32 + ext.len());
    out.extend_from_slice(tmpdir);
    out.extend_from_slice(b"/cbm-diagnostics-");
    out.extend_from_slice(pid.to_string().as_bytes());
    out.push(b'.');
    out.extend_from_slice(ext);
    Some(out)
}

pub fn format_json(snapshot: &Snapshot) -> Vec<u8> {
    let q = snapshot.queries;
    format!(
        "{{\n  \"uptime_s\": {},\n  \"rss_bytes\": {},\n  \"peak_rss_bytes\": {},\n  \
         \"heap_committed_bytes\": {},\n  \"peak_committed_bytes\": {},\n  \
         \"page_faults\": {},\n  \"fd_count\": {},\n  \"query_count\": {},\n  \
         \"query_errors\": {},\n  \"query_total_us\": {},\n  \"query_avg_us\": {},\n  \
         \"query_max_us\": {},\n  \"pid\": {}\n}}\n",
        snapshot.uptime_s,
        snapshot.rss_bytes,
        snapshot.peak_rss_bytes,
        snapshot.heap_committed_bytes,
        snapshot.peak_committed_bytes,
        snapshot.page_faults,
        snapshot.fd_count,
        q.count,
        q.errors,
        q.total_us,
        q.avg_us,
        q.max_us,
        snapshot.pid
    )
    .into_bytes()
}

pub fn format_ndjson(snapshot: &Snapshot) -> Vec<u8> {
    format!(
        "{{\"uptime_s\":{},\"rss\":{},\"peak_rss\":{},\"committed\":{},\
         \"peak_committed\":{},\"page_faults\":{},\"fd\":{},\"queries\":{}}}\n",
        snapshot.uptime_s,
        snapshot.rss_bytes,
        snapshot.peak_rss_bytes,
        snapshot.heap_committed_bytes,
        snapshot.peak_committed_bytes,
        snapshot.page_faults,
        snapshot.fd_count,
        snapshot.queries.count
    )
    .into_bytes()
}

#[cfg(test)]
mod tests {
    use super::*;

    fn fixture_snapshot() -> Snapshot {
        Snapshot {
            uptime_s: 17,
            rss_bytes: 1024,
            peak_rss_bytes: 2048,
            heap_committed_bytes: 4096,
            peak_committed_bytes: 8192,
            page_faults: 23,
            fd_count: 9,
            queries: QuerySnapshot {
                count: 3,
                errors: 1,
                total_us: 425,
                avg_us: 141,
                max_us: 250,
            },
            pid: 4242,
        }
    }

    #[test]
    fn env_enabled_matches_c_contract() {
        assert!(env_enabled_value(Some(b"1")));
        assert!(env_enabled_value(Some(b"true")));
        assert!(!env_enabled_value(None));
        assert!(!env_enabled_value(Some(b"")));
        assert!(!env_enabled_value(Some(b"0")));
        assert!(!env_enabled_value(Some(b"false")));
        assert!(!env_enabled_value(Some(b"TRUE")));
        assert!(!env_enabled_value(Some(b" true")));
    }

    #[test]
    fn query_snapshot_reduces_average() {
        assert_eq!(
            query_stats_snapshot_values(3, 1, 425, 250),
            QuerySnapshot {
                count: 3,
                errors: 1,
                total_us: 425,
                avg_us: 141,
                max_us: 250,
            }
        );
        assert_eq!(query_stats_snapshot_values(0, 0, 425, 250).avg_us, 0);
    }

    #[test]
    fn formatters_match_c_fixtures() {
        let snapshot = fixture_snapshot();
        assert_eq!(
            format_path(Some(b"/tmp"), 1234, Some(b"json")).unwrap(),
            b"/tmp/cbm-diagnostics-1234.json"
        );
        assert_eq!(
            format_json(&snapshot),
            b"{\n  \"uptime_s\": 17,\n  \"rss_bytes\": 1024,\n  \"peak_rss_bytes\": 2048,\n  \
              \"heap_committed_bytes\": 4096,\n  \"peak_committed_bytes\": 8192,\n  \
              \"page_faults\": 23,\n  \"fd_count\": 9,\n  \"query_count\": 3,\n  \
              \"query_errors\": 1,\n  \"query_total_us\": 425,\n  \"query_avg_us\": 141,\n  \
              \"query_max_us\": 250,\n  \"pid\": 4242\n}\n"
        );
        assert_eq!(
            format_ndjson(&snapshot),
            b"{\"uptime_s\":17,\"rss\":1024,\"peak_rss\":2048,\"committed\":4096,\
              \"peak_committed\":8192,\"page_faults\":23,\"fd\":9,\"queries\":3}\n"
        );
    }
}
