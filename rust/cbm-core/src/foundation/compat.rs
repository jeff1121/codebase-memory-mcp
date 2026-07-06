//! 對齊 `src/foundation/compat_*` 的 deterministic helper contract。
//!
//! 這裡只固定 flags、caps 與 pure-value validation；實際 OS regex/thread/fs
//! side effects 仍留在 C。

pub const REG_EXTENDED: i32 = 1;
pub const REG_ICASE: i32 = 2;
pub const REG_NOSUB: i32 = 4;
pub const REG_NEWLINE: i32 = 8;
pub const REG_KNOWN_MASK: i32 = REG_EXTENDED | REG_ICASE | REG_NOSUB | REG_NEWLINE;

pub const REG_OK: i32 = 0;
pub const REG_NOMATCH: i32 = -1;

pub const REG_MATCH_CAP: i32 = 32;
pub const DEFAULT_STACK_SIZE: usize = 8 * 1024 * 1024;

#[must_use]
pub fn regex_known_flags(flags: i32) -> i32 {
    flags & REG_KNOWN_MASK
}

#[must_use]
pub fn regex_match_cap(nmatch: i32, has_matches: bool) -> i32 {
    if nmatch <= 0 || !has_matches {
        0
    } else {
        nmatch.min(REG_MATCH_CAP)
    }
}

#[must_use]
pub fn regex_status_matched(matched: bool) -> i32 {
    if matched {
        REG_OK
    } else {
        REG_NOMATCH
    }
}

#[must_use]
pub fn thread_effective_stack_size(stack_size: usize) -> usize {
    if stack_size == 0 {
        DEFAULT_STACK_SIZE
    } else {
        stack_size
    }
}

#[must_use]
pub fn aligned_alloc_precheck(alignment: usize, size: usize, pointer_size: usize) -> bool {
    size > 0 && pointer_size > 0 && alignment >= pointer_size && alignment.is_power_of_two()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn regex_helpers_match_c_wrapper_bounds() {
        assert_eq!(regex_known_flags(0xff), REG_KNOWN_MASK);
        assert_eq!(regex_match_cap(-1, true), 0);
        assert_eq!(regex_match_cap(0, true), 0);
        assert_eq!(regex_match_cap(2, false), 0);
        assert_eq!(regex_match_cap(2, true), 2);
        assert_eq!(regex_match_cap(40, true), REG_MATCH_CAP);
        assert_eq!(regex_status_matched(true), REG_OK);
        assert_eq!(regex_status_matched(false), REG_NOMATCH);
    }

    #[test]
    fn thread_and_alignment_helpers_match_c_contract() {
        assert_eq!(thread_effective_stack_size(0), DEFAULT_STACK_SIZE);
        assert_eq!(thread_effective_stack_size(64 * 1024), 64 * 1024);
        assert!(aligned_alloc_precheck(16, 128, 8));
        assert!(!aligned_alloc_precheck(4, 128, 8));
        assert!(!aligned_alloc_precheck(24, 128, 8));
        assert!(!aligned_alloc_precheck(16, 0, 8));
    }
}
