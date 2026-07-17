use std::os::raw::c_int;

const POLL_BASE_MS: i64 = 5_000;
const POLL_FILE_STEP: i64 = 500;
const POLL_STEP_MS: i64 = 1_000;
const POLL_MAX_MS: i64 = 60_000;

#[must_use]
pub fn poll_interval_ms(file_count: c_int) -> c_int {
    let interval = POLL_BASE_MS + (i64::from(file_count) / POLL_FILE_STEP) * POLL_STEP_MS;
    interval.min(POLL_MAX_MS) as c_int
}

#[cfg(test)]
mod tests {
    use super::poll_interval_ms;

    #[test]
    fn poll_interval_matches_watcher_contract() {
        let cases = [
            (0, 5_000),
            (70, 5_000),
            (499, 5_000),
            (500, 6_000),
            (2_000, 9_000),
            (5_000, 15_000),
            (10_000, 25_000),
            (50_000, 60_000),
            (100_000, 60_000),
            (-1, 5_000),
        ];

        for (file_count, expected) in cases {
            assert_eq!(poll_interval_ms(file_count), expected);
        }
    }
}
