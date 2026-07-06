//! 對齊 `src/foundation/profile.c` 的 deterministic helper contract。
//!
//! 這裡只固定 env gate 與 elapsed/rate 計算；clock 與 log emission 仍留在 C。

const US_PER_SEC: i64 = 1_000_000;
const NS_PER_US: i64 = 1_000;
const US_PER_MS: i64 = 1_000;

#[must_use]
pub fn env_enabled(value: Option<&[u8]>) -> bool {
    matches!(value, Some(bytes) if !bytes.is_empty() && bytes[0] != b'0')
}

#[must_use]
pub fn elapsed_us(start_sec: i64, start_nsec: i64, now_sec: i64, now_nsec: i64) -> i64 {
    ((now_sec - start_sec) * US_PER_SEC) + ((now_nsec - start_nsec) / NS_PER_US)
}

#[must_use]
pub fn elapsed_ms(us: i64) -> i64 {
    us / US_PER_MS
}

#[must_use]
pub fn rate_per_s(items: i64, us: i64) -> Option<i64> {
    if items > 0 && us > 0 {
        Some(((items as f64) * (US_PER_SEC as f64) / (us as f64)) as i64)
    } else {
        None
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn env_gate_matches_c_profile_contract() {
        assert!(!env_enabled(None));
        assert!(!env_enabled(Some(b"")));
        assert!(!env_enabled(Some(b"0")));
        assert!(!env_enabled(Some(b"00")));
        assert!(env_enabled(Some(b"1")));
        assert!(env_enabled(Some(b"false")));
    }

    #[test]
    fn elapsed_and_rate_match_c_math() {
        let us = elapsed_us(10, 900_000_000, 12, 100_250_000);
        assert_eq!(us, 1_200_250);
        assert_eq!(elapsed_ms(us), 1200);
        assert_eq!(rate_per_s(12, 3_000), Some(4000));
        assert_eq!(rate_per_s(12, 0), None);
        assert_eq!(rate_per_s(0, 3_000), None);
    }
}
