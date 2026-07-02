//! 對齊 `src/foundation/str_intern.c` 的 string interning pool。

use std::collections::HashMap;
use std::os::raw::c_char;
use std::rc::Rc;

#[derive(Debug, Default)]
pub struct InternPool {
    entries: HashMap<Vec<u8>, Rc<[u8]>>,
    total_bytes: usize,
}

impl InternPool {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn intern(&mut self, value: Option<&str>) -> Option<Rc<[u8]>> {
        let value = value?;
        self.intern_n(Some(value.as_bytes()), value.len())
    }

    pub fn intern_n(&mut self, value: Option<&[u8]>, len: usize) -> Option<Rc<[u8]>> {
        let value = value?;
        if len > value.len() {
            return None;
        }

        let bytes = &value[..len];
        if let Some(existing) = self.entries.get(bytes) {
            return Some(Rc::clone(existing));
        }

        let total_bytes = self.total_bytes.checked_add(len)?;
        let interned: Rc<[u8]> = Rc::from(bytes);
        self.entries.insert(bytes.to_vec(), Rc::clone(&interned));
        self.total_bytes = total_bytes;
        Some(interned)
    }

    pub fn count(&self) -> usize {
        self.entries.len()
    }

    pub fn bytes(&self) -> usize {
        self.total_bytes
    }
}

#[derive(Debug, Default)]
pub struct CInternPool {
    entries: HashMap<Vec<u8>, Box<[u8]>>,
    total_bytes: usize,
}

impl CInternPool {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn intern_n(&mut self, value: &[u8]) -> Option<*const c_char> {
        if value.len() > u32::MAX as usize {
            return None;
        }
        if let Some(existing) = self.entries.get(value) {
            return Some(existing.as_ptr().cast());
        }

        let stored_len = value.len().checked_add(1)?;
        let total_bytes = self.total_bytes.checked_add(value.len())?;

        self.entries.try_reserve(1).ok()?;

        let mut key = Vec::new();
        key.try_reserve_exact(value.len()).ok()?;
        key.extend_from_slice(value);

        let mut stored = Vec::new();
        stored.try_reserve_exact(stored_len).ok()?;
        stored.extend_from_slice(value);
        stored.push(0);
        let stored = stored.into_boxed_slice();
        let ptr = stored.as_ptr().cast();

        self.entries.insert(key, stored);
        self.total_bytes = total_bytes;
        Some(ptr)
    }

    pub fn count(&self) -> u32 {
        self.entries.len().min(u32::MAX as usize) as u32
    }

    pub fn bytes(&self) -> usize {
        self.total_bytes
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn as_str(value: &Rc<[u8]>) -> &str {
        std::str::from_utf8(value).expect("test data is valid utf-8")
    }

    #[test]
    fn basic_interning_matches_c_cases() {
        let mut pool = InternPool::new();
        assert_eq!(pool.count(), 0);

        let hello = pool.intern(Some("hello")).expect("interned");
        assert_eq!(as_str(&hello), "hello");
        assert_eq!(pool.count(), 1);

        let hello_again = pool.intern(Some("hello")).expect("interned");
        assert!(Rc::ptr_eq(&hello, &hello_again));
        assert_eq!(pool.count(), 1);

        let world = pool.intern(Some("world")).expect("interned");
        assert!(!Rc::ptr_eq(&hello, &world));
        assert_eq!(as_str(&world), "world");
        assert_eq!(pool.count(), 2);
    }

    #[test]
    fn intern_n_matches_prefix_and_empty_cases() {
        let mut pool = InternPool::new();

        let prefix = pool
            .intern_n(Some(b"hello world"), 5)
            .expect("interned prefix");
        assert_eq!(as_str(&prefix), "hello");

        let full = pool.intern(Some("hello")).expect("interned full");
        assert!(Rc::ptr_eq(&prefix, &full));

        let empty = pool.intern_n(Some(b"anything"), 0).expect("interned empty");
        assert_eq!(as_str(&empty), "");
        let empty_again = pool.intern(Some("")).expect("interned empty");
        assert!(Rc::ptr_eq(&empty, &empty_again));
        assert_eq!(pool.count(), 2);
    }

    #[test]
    fn tracks_unique_bytes_only() {
        let mut pool = InternPool::new();
        assert_eq!(pool.bytes(), 0);

        pool.intern(Some("abc"));
        assert_eq!(pool.bytes(), 3);

        pool.intern(Some("defgh"));
        assert_eq!(pool.bytes(), 8);

        pool.intern(Some("abc"));
        assert_eq!(pool.bytes(), 8);

        pool.intern(Some(""));
        assert_eq!(pool.bytes(), 8);

        pool.intern(Some("x"));
        assert_eq!(pool.bytes(), 9);
    }

    #[test]
    fn handles_many_strings_and_pointer_stability() {
        let mut pool = InternPool::new();
        let sentinel = pool.intern(Some("sentinel_value")).expect("interned");

        for i in 0..10_000 {
            pool.intern(Some(&format!("filler_{i:06}")));
        }

        assert_eq!(pool.count(), 10_001);
        assert_eq!(as_str(&sentinel), "sentinel_value");
        let sentinel_again = pool.intern(Some("sentinel_value")).expect("interned");
        assert!(Rc::ptr_eq(&sentinel, &sentinel_again));
    }

    #[test]
    fn same_prefix_different_lengths_are_distinct() {
        let mut pool = InternPool::new();
        let src = b"abcdefgh";

        let s3 = pool.intern_n(Some(src), 3).expect("interned");
        let s5 = pool.intern_n(Some(src), 5).expect("interned");
        let s8 = pool.intern_n(Some(src), 8).expect("interned");

        assert_eq!(as_str(&s3), "abc");
        assert_eq!(as_str(&s5), "abcde");
        assert_eq!(as_str(&s8), "abcdefgh");
        assert!(!Rc::ptr_eq(&s3, &s5));
        assert!(!Rc::ptr_eq(&s5, &s8));
        assert!(!Rc::ptr_eq(&s3, &s8));
        assert_eq!(pool.count(), 3);
    }

    #[test]
    fn null_and_invalid_inputs_match_safe_policy() {
        let mut pool = InternPool::new();

        assert_eq!(pool.intern(None), None);
        assert_eq!(pool.intern_n(None, 0), None);
        assert_eq!(pool.intern_n(Some(b"abc"), 4), None);
        assert_eq!(pool.count(), 0);
        assert_eq!(pool.bytes(), 0);
    }

    #[test]
    fn byte_level_dedup_handles_embedded_nul() {
        let mut pool = InternPool::new();
        let first = pool.intern_n(Some(b"a\0b"), 3).expect("interned");
        let second = pool.intern_n(Some(b"a\0bzzz"), 3).expect("interned");
        let shorter = pool.intern_n(Some(b"a\0b"), 2).expect("interned");

        assert!(Rc::ptr_eq(&first, &second));
        assert!(!Rc::ptr_eq(&first, &shorter));
        assert_eq!(&*first, b"a\0b");
        assert_eq!(&*shorter, b"a\0");
        assert_eq!(pool.count(), 2);
        assert_eq!(pool.bytes(), 5);
    }

    #[test]
    fn pools_are_independent() {
        let mut first_pool = InternPool::new();
        let mut second_pool = InternPool::new();

        let first = first_pool.intern(Some("shared")).expect("interned");
        let second = second_pool.intern(Some("shared")).expect("interned");

        assert_eq!(as_str(&first), "shared");
        assert_eq!(as_str(&second), "shared");
        assert!(!Rc::ptr_eq(&first, &second));
    }

    #[test]
    fn c_pool_returns_stable_nul_terminated_pointers() {
        let mut pool = CInternPool::new();
        let first = pool.intern_n(b"hello").expect("interned");
        let second = pool.intern_n(b"hello").expect("interned");
        let other = pool.intern_n(b"world").expect("interned");

        assert_eq!(first, second);
        assert_ne!(first, other);
        assert_eq!(pool.count(), 2);
        assert_eq!(pool.bytes(), 10);

        for i in 0..10_000 {
            pool.intern_n(format!("filler_{i:06}").as_bytes());
        }

        let again = pool.intern_n(b"hello").expect("interned");
        assert_eq!(first, again);
    }

    #[test]
    fn c_pool_dedups_embedded_nul_by_full_byte_sequence() {
        let mut pool = CInternPool::new();
        let first = pool.intern_n(b"a\0b").expect("interned");
        let second = pool.intern_n(b"a\0b").expect("interned");
        let shorter = pool.intern_n(b"a\0").expect("interned");

        assert_eq!(first, second);
        assert_ne!(first, shorter);
        assert_eq!(pool.count(), 2);
        assert_eq!(pool.bytes(), 5);
    }
}
