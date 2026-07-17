#![allow(unsafe_code)]

use std::collections::HashMap;
use std::ffi::{c_char, c_void, CStr};
use std::ptr;

struct HashEntry {
    key: *const c_char,
    value: *mut c_void,
}

pub struct CbmHashTable {
    entries: HashMap<Vec<u8>, HashEntry>,
}

type CbmHashTableIterFn =
    unsafe extern "C" fn(key: *const c_char, value: *mut c_void, userdata: *mut c_void);

unsafe fn key_bytes(key: *const c_char) -> Option<Vec<u8>> {
    if key.is_null() {
        return None;
    }
    Some(unsafe { CStr::from_ptr(key) }.to_bytes().to_vec())
}

#[no_mangle]
pub extern "C" fn cbm_ht_create(initial_capacity: u32) -> *mut CbmHashTable {
    let mut entries = HashMap::new();
    if initial_capacity > 0 && entries.try_reserve(initial_capacity as usize).is_err() {
        return ptr::null_mut();
    }
    Box::into_raw(Box::new(CbmHashTable { entries }))
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須是 null，或是由 `cbm_ht_create` 回傳且尚未釋放的指標。
pub unsafe extern "C" fn cbm_ht_free(ht: *mut CbmHashTable) {
    if ht.is_null() {
        return;
    }
    drop(unsafe { Box::from_raw(ht) });
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須是 null，或是有效的 hash table；`key` 必須是有效的
/// NUL-terminated C string。table 不會複製或釋放 `key`，呼叫端必須讓
/// 該指標在 entry 存活期間保持有效。
pub unsafe extern "C" fn cbm_ht_set(
    ht: *mut CbmHashTable,
    key: *const c_char,
    value: *mut c_void,
) -> *mut c_void {
    if ht.is_null() {
        return ptr::null_mut();
    }
    let Some(key_bytes) = (unsafe { key_bytes(key) }) else {
        return ptr::null_mut();
    };
    let table = unsafe { &mut *ht };
    if let Some(entry) = table.entries.get_mut(&key_bytes) {
        let previous = entry.value;
        entry.key = key;
        entry.value = value;
        return previous;
    }
    table.entries.insert(key_bytes, HashEntry { key, value });
    ptr::null_mut()
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須是 null，或是有效的 hash table；`key` 必須是 null，或是
/// 有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_ht_get(ht: *const CbmHashTable, key: *const c_char) -> *mut c_void {
    if ht.is_null() {
        return ptr::null_mut();
    }
    let Some(key_bytes) = (unsafe { key_bytes(key) }) else {
        return ptr::null_mut();
    };
    unsafe { &*ht }
        .entries
        .get(&key_bytes)
        .map_or(ptr::null_mut(), |entry| entry.value)
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須是 null，或是有效的 hash table；`key` 必須是 null，或是
/// 有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_ht_has(ht: *const CbmHashTable, key: *const c_char) -> bool {
    if ht.is_null() {
        return false;
    }
    let Some(key_bytes) = (unsafe { key_bytes(key) }) else {
        return false;
    };
    unsafe { &*ht }.entries.contains_key(&key_bytes)
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須是 null，或是有效的 hash table；`key` 必須是 null，或是
/// 有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_ht_get_key(
    ht: *const CbmHashTable,
    key: *const c_char,
) -> *const c_char {
    if ht.is_null() {
        return ptr::null();
    }
    let Some(key_bytes) = (unsafe { key_bytes(key) }) else {
        return ptr::null();
    };
    unsafe { &*ht }
        .entries
        .get(&key_bytes)
        .map_or(ptr::null(), |entry| entry.key)
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須是 null，或是有效的 hash table；`key` 必須是 null，或是
/// 有效的 NUL-terminated C string。
pub unsafe extern "C" fn cbm_ht_delete(ht: *mut CbmHashTable, key: *const c_char) -> *mut c_void {
    if ht.is_null() {
        return ptr::null_mut();
    }
    let Some(key_bytes) = (unsafe { key_bytes(key) }) else {
        return ptr::null_mut();
    };
    unsafe { &mut *ht }
        .entries
        .remove(&key_bytes)
        .map_or(ptr::null_mut(), |entry| entry.value)
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須是 null，或是有效的 hash table。
pub unsafe extern "C" fn cbm_ht_count(ht: *const CbmHashTable) -> u32 {
    if ht.is_null() {
        return 0;
    }
    unsafe { &*ht }.entries.len().min(u32::MAX as usize) as u32
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須是 null，或是有效的 hash table。`fn` 若非 null，必須是
/// 可呼叫的 C callback；callback 收到的 key pointer 只在 entry 存活時有效。
pub unsafe extern "C" fn cbm_ht_foreach(
    ht: *const CbmHashTable,
    callback: Option<CbmHashTableIterFn>,
    userdata: *mut c_void,
) {
    let Some(callback) = callback else {
        return;
    };
    if ht.is_null() {
        return;
    }
    let entries: Vec<_> = unsafe { &*ht }
        .entries
        .values()
        .map(|entry| (entry.key, entry.value))
        .collect();
    for (key, value) in entries {
        unsafe { callback(key, value, userdata) };
    }
}

#[no_mangle]
/// # Safety
///
/// `ht` 必須是 null，或是有效的 hash table。
pub unsafe extern "C" fn cbm_ht_clear(ht: *mut CbmHashTable) {
    if ht.is_null() {
        return;
    }
    unsafe { &mut *ht }.entries.clear();
}
