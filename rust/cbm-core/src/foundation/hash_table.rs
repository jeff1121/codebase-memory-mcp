//! 對齊 `src/foundation/hash_table.c` 的 borrowed-pointer hash table 契約。
//!
//! 此模組只建模契約語意，供 test-only `cbm_rs_ht_*` FFI parity fixture 使用，
//! 不改動產品預設 C 路徑（C 的 Verstable 實作仍是唯一產品路徑）。固定下列
//! 契約，對齊 `tests/test_hash_table.c`：
//!
//! - key 以「內容位元組」比對，而非指標身分（含非 UTF-8 高位元 key）。
//! - `get_key` 回傳最近一次 set 傳入的 stored key 指標；覆寫會更新該指標。
//! - value 指標為 borrowed；null value（0）仍是有效存在項，`has` 為 true。

use std::collections::HashMap;

/// 對齊 C `CBMHashTable`：key 以內容位元組比對，value 與 stored key 指標
/// 皆為 borrowed（呼叫端擁有記憶體），以 `usize` 保存指標位元值。
#[derive(Default)]
pub struct HashTable {
    /// content bytes（不含 NUL 終止符）-> (stored key ptr, value ptr)。
    map: HashMap<Vec<u8>, (usize, usize)>,
}

impl HashTable {
    #[must_use]
    pub fn new() -> Self {
        Self {
            map: HashMap::new(),
        }
    }

    /// 對齊 `cbm_ht_set`：插入或覆寫。覆寫時同時更新 stored key 指標
    /// （對齊 C 的 `ht_overwrite_replaces_key_pointer_contract`），並回傳
    /// 前一個 value 指標；無前值回傳 `None`。
    pub fn set(&mut self, key: &[u8], key_ptr: usize, value_ptr: usize) -> Option<usize> {
        self.map
            .insert(key.to_vec(), (key_ptr, value_ptr))
            .map(|(_old_key_ptr, old_value_ptr)| old_value_ptr)
    }

    /// 對齊 `cbm_ht_get`：回傳 value 指標；不存在回傳 `None`。value 指標
    /// 可能為 0（null），仍是有效存在項（見 `has`）。
    #[must_use]
    pub fn get(&self, key: &[u8]) -> Option<usize> {
        self.map.get(key).map(|&(_k, v)| v)
    }

    /// 對齊 `cbm_ht_has`：僅檢查存在性，與 value 是否為 null 無關。
    #[must_use]
    pub fn has(&self, key: &[u8]) -> bool {
        self.map.contains_key(key)
    }

    /// 對齊 `cbm_ht_get_key`：回傳 stored key 指標（最近一次 set 傳入者）。
    #[must_use]
    pub fn get_key(&self, key: &[u8]) -> Option<usize> {
        self.map.get(key).map(|&(k, _v)| k)
    }

    /// 對齊 `cbm_ht_delete`：移除並回傳 value 指標；不存在回傳 `None`。
    pub fn delete(&mut self, key: &[u8]) -> Option<usize> {
        self.map.remove(key).map(|(_k, v)| v)
    }

    /// 對齊 `cbm_ht_count`。
    #[must_use]
    pub fn count(&self) -> usize {
        self.map.len()
    }

    /// 對齊 `cbm_ht_clear`。
    pub fn clear(&mut self) {
        self.map.clear();
    }

    /// 供 foreach FFI 使用：以 (stored key ptr, value ptr) 逐項列舉，每項
    /// 恰好一次；順序未定義（對齊 C foreach 不保證順序）。
    pub fn entries(&self) -> impl Iterator<Item = (usize, usize)> + '_ {
        self.map.values().copied()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn set_get_overwrite_returns_prev_and_updates_key_ptr() {
        let mut ht = HashTable::new();
        assert_eq!(ht.set(b"key", 0x1000, 0xAA), None);
        assert_eq!(ht.get(b"key"), Some(0xAA));
        assert_eq!(ht.get_key(b"key"), Some(0x1000));

        // 覆寫：回傳前值，且 stored key 指標更新為新指標。
        assert_eq!(ht.set(b"key", 0x2000, 0xBB), Some(0xAA));
        assert_eq!(ht.get(b"key"), Some(0xBB));
        assert_eq!(ht.get_key(b"key"), Some(0x2000));
        assert_eq!(ht.count(), 1);
    }

    #[test]
    fn null_value_is_a_present_entry() {
        let mut ht = HashTable::new();
        assert_eq!(ht.set(b"nullable", 0x10, 0), None);
        assert_eq!(ht.get(b"nullable"), Some(0));
        assert!(ht.has(b"nullable"));
        assert_eq!(ht.count(), 1);
    }

    #[test]
    fn content_keyed_including_high_bit_bytes() {
        let mut ht = HashTable::new();
        let key: &[u8] = &[0xff, 0x80, b'k', 0xfe];
        ht.set(key, 0x30, 0x99);
        assert!(ht.has(&[0xff, 0x80, b'k', 0xfe]));
        assert_eq!(ht.get(&[0xff, 0x80, b'k', 0xfe]), Some(0x99));
        assert_eq!(ht.get(b"other"), None);
    }

    #[test]
    fn delete_clear_and_foreach_exactly_once() {
        let mut ht = HashTable::new();
        ht.set(b"a", 0x1, 10);
        ht.set(b"b", 0x2, 20);
        ht.set(b"c", 0x3, 30);
        assert_eq!(ht.count(), 3);

        let sum: usize = ht.entries().map(|(_k, v)| v).sum();
        assert_eq!(sum, 60);

        assert_eq!(ht.delete(b"b"), Some(20));
        assert_eq!(ht.delete(b"b"), None);
        assert_eq!(ht.count(), 2);

        ht.clear();
        assert_eq!(ht.count(), 0);
        assert!(!ht.has(b"a"));
    }
}
