#![allow(unsafe_code)]

use crate::pipeline::registry::{self, Registry, Resolution};
use std::collections::HashMap;
use std::ffi::{c_char, c_int, c_void, CStr};
use std::mem::size_of;
use std::ptr;
use std::slice;

unsafe extern "C" {
    fn malloc(size: usize) -> *mut c_void;
}

#[repr(C)]
pub struct CbmPipelineRegistryResolution {
    qualified_name: *const c_char,
    strategy: *const c_char,
    confidence: f64,
    candidate_count: c_int,
}

#[repr(C)]
pub struct CbmPipelineRegistryFuzzyResult {
    result: CbmPipelineRegistryResolution,
    ok: bool,
}

pub struct CbmPipelineRegistryHandle {
    registry: Registry,
    qn_strings: HashMap<Vec<u8>, Box<[u8]>>,
    labels: HashMap<Vec<u8>, Box<[u8]>>,
    by_name_ptrs: HashMap<Vec<u8>, Vec<*const c_char>>,
}

type ImportParts<'a> = (Vec<&'a [u8]>, Vec<&'a [u8]>);

static STRATEGY_IMPORT_MAP: &[u8] = b"import_map\0";
static STRATEGY_IMPORT_MAP_SUFFIX: &[u8] = b"import_map_suffix\0";
static STRATEGY_SAME_MODULE: &[u8] = b"same_module\0";
static STRATEGY_QUALIFIED_SUFFIX: &[u8] = b"qualified_suffix\0";
static STRATEGY_UNIQUE_NAME: &[u8] = b"unique_name\0";
static STRATEGY_SUFFIX_MATCH: &[u8] = b"suffix_match\0";
static STRATEGY_FUZZY: &[u8] = b"fuzzy\0";
static BAND_HIGH: &[u8] = b"high\0";
static BAND_MEDIUM: &[u8] = b"medium\0";
static BAND_SPECULATIVE: &[u8] = b"speculative\0";
static BAND_EMPTY: &[u8] = b"\0";

static PERL_BUILTINS: &[&[u8]] = &[
    b"abs",
    b"atan2",
    b"binmode",
    b"bless",
    b"caller",
    b"chdir",
    b"chmod",
    b"chomp",
    b"chop",
    b"chown",
    b"chr",
    b"chroot",
    b"close",
    b"closedir",
    b"cos",
    b"defined",
    b"delete",
    b"die",
    b"do",
    b"each",
    b"eof",
    b"eval",
    b"exec",
    b"exists",
    b"exit",
    b"fork",
    b"gmtime",
    b"goto",
    b"grep",
    b"hex",
    b"index",
    b"int",
    b"join",
    b"keys",
    b"last",
    b"lc",
    b"lcfirst",
    b"length",
    b"local",
    b"localtime",
    b"log",
    b"lstat",
    b"map",
    b"mkdir",
    b"my",
    b"next",
    b"oct",
    b"open",
    b"opendir",
    b"ord",
    b"our",
    b"pop",
    b"pos",
    b"print",
    b"printf",
    b"push",
    b"quotemeta",
    b"rand",
    b"read",
    b"readdir",
    b"readline",
    b"redo",
    b"ref",
    b"rename",
    b"require",
    b"return",
    b"reverse",
    b"rindex",
    b"rmdir",
    b"say",
    b"scalar",
    b"seek",
    b"shift",
    b"sin",
    b"sleep",
    b"sort",
    b"splice",
    b"split",
    b"sprintf",
    b"sqrt",
    b"srand",
    b"stat",
    b"substr",
    b"system",
    b"time",
    b"uc",
    b"ucfirst",
    b"undef",
    b"unlink",
    b"unshift",
    b"values",
    b"wantarray",
    b"warn",
    b"write",
];

fn c_string(bytes: &[u8]) -> Box<[u8]> {
    let mut value = Vec::with_capacity(bytes.len() + 1);
    value.extend_from_slice(bytes);
    value.push(0);
    value.into_boxed_slice()
}

unsafe fn c_bytes<'a>(value: *const c_char) -> Option<&'a [u8]> {
    if value.is_null() {
        return None;
    }
    Some(unsafe { CStr::from_ptr(value) }.to_bytes())
}

unsafe fn import_parts<'a>(
    keys: *const *const c_char,
    vals: *const *const c_char,
    count: usize,
) -> Option<ImportParts<'a>> {
    if count == 0 {
        return Some((Vec::new(), Vec::new()));
    }
    if keys.is_null() || vals.is_null() {
        return None;
    }
    let key_ptrs = unsafe { slice::from_raw_parts(keys, count) };
    let val_ptrs = unsafe { slice::from_raw_parts(vals, count) };
    let mut key_bytes = Vec::with_capacity(count);
    let mut val_bytes = Vec::with_capacity(count);
    for (key, val) in key_ptrs.iter().zip(val_ptrs.iter()) {
        key_bytes.push(unsafe { c_bytes(*key) }?);
        val_bytes.push(unsafe { c_bytes(*val) }?);
    }
    Some((key_bytes, val_bytes))
}

fn strategy_ptr(strategy: &str) -> *const c_char {
    let bytes = match strategy {
        "import_map" => STRATEGY_IMPORT_MAP,
        "import_map_suffix" => STRATEGY_IMPORT_MAP_SUFFIX,
        "same_module" => STRATEGY_SAME_MODULE,
        "qualified_suffix" => STRATEGY_QUALIFIED_SUFFIX,
        "unique_name" => STRATEGY_UNIQUE_NAME,
        "suffix_match" => STRATEGY_SUFFIX_MATCH,
        "fuzzy" => STRATEGY_FUZZY,
        _ => return ptr::null(),
    };
    bytes.as_ptr() as *const c_char
}

fn empty_resolution() -> CbmPipelineRegistryResolution {
    CbmPipelineRegistryResolution {
        qualified_name: ptr::null(),
        strategy: ptr::null(),
        confidence: 0.0,
        candidate_count: 0,
    }
}

impl CbmPipelineRegistryHandle {
    fn qn_ptr(&self, qualified_name: &[u8]) -> Option<*const c_char> {
        self.qn_strings
            .get(qualified_name)
            .map(|value| value.as_ptr() as *const c_char)
    }

    fn resolution(&self, value: &Resolution) -> CbmPipelineRegistryResolution {
        CbmPipelineRegistryResolution {
            qualified_name: self.qn_ptr(&value.qualified_name).unwrap_or(ptr::null()),
            strategy: strategy_ptr(value.strategy),
            confidence: value.confidence,
            candidate_count: value.candidate_count,
        }
    }
}

#[no_mangle]
pub extern "C" fn cbm_registry_new() -> *mut CbmPipelineRegistryHandle {
    Box::into_raw(Box::new(CbmPipelineRegistryHandle {
        registry: Registry::default(),
        qn_strings: HashMap::new(),
        labels: HashMap::new(),
        by_name_ptrs: HashMap::new(),
    }))
}

#[no_mangle]
/// # Safety
///
/// `registry` 必須是 null，或是 `cbm_registry_new` 回傳且尚未釋放的指標。
pub unsafe extern "C" fn cbm_registry_free(registry: *mut CbmPipelineRegistryHandle) {
    if !registry.is_null() {
        unsafe { drop(Box::from_raw(registry)) };
    }
}

#[no_mangle]
/// # Safety
///
/// 所有指標必須是 null 或有效的 NUL 結尾 C 字串；`registry` 必須仍有效。
pub unsafe extern "C" fn cbm_registry_add(
    registry: *mut CbmPipelineRegistryHandle,
    name: *const c_char,
    qualified_name: *const c_char,
    label: *const c_char,
) {
    let _ = name;
    if registry.is_null() || qualified_name.is_null() || label.is_null() {
        return;
    }
    let Some(qn) = (unsafe { c_bytes(qualified_name) }) else {
        return;
    };
    let Some(label) = (unsafe { c_bytes(label) }) else {
        return;
    };
    if qn.is_empty() {
        return;
    }

    let handle = unsafe { &mut *registry };
    if handle.registry.contains(qn) {
        return;
    }

    let qn_key = qn.to_vec();
    let simple = registry::simple_name(qn).to_vec();
    handle.registry.add(qn_key.clone());
    handle.qn_strings.insert(qn_key.clone(), c_string(qn));
    handle.labels.insert(qn_key.clone(), c_string(label));
    let qn_ptr = handle
        .qn_strings
        .get(&qn_key)
        .map_or(ptr::null(), |value| value.as_ptr() as *const c_char);
    handle.by_name_ptrs.entry(simple).or_default().push(qn_ptr);
}

#[no_mangle]
/// # Safety
///
/// `registry`、`callee_name`、import arrays 與輸出指標必須遵守既有 C ABI
/// 的有效記憶體契約；Rust 只在本次呼叫期間借用輸入字串。
pub unsafe extern "C" fn cbm_registry_resolve(
    registry: *const CbmPipelineRegistryHandle,
    callee_name: *const c_char,
    module_qn: *const c_char,
    import_map_keys: *const *const c_char,
    import_map_vals: *const *const c_char,
    import_map_count: c_int,
) -> CbmPipelineRegistryResolution {
    if registry.is_null() || callee_name.is_null() || import_map_count < 0 {
        return empty_resolution();
    }
    let Some(callee) = (unsafe { c_bytes(callee_name) }) else {
        return empty_resolution();
    };
    let module = unsafe { c_bytes(module_qn) }.unwrap_or(&[]);
    let Some((keys, vals)) =
        (unsafe { import_parts(import_map_keys, import_map_vals, import_map_count as usize) })
    else {
        return empty_resolution();
    };
    let handle = unsafe { &*registry };
    handle
        .registry
        .resolve(callee, module, &keys, &vals)
        .map_or_else(empty_resolution, |value| handle.resolution(&value))
}

#[no_mangle]
/// # Safety
///
/// `registry`、`callee_name` 與 import arrays 必須是 null 或有效的 C 字串指標。
pub unsafe extern "C" fn cbm_registry_fuzzy_resolve(
    registry: *const CbmPipelineRegistryHandle,
    callee_name: *const c_char,
    module_qn: *const c_char,
    import_map_keys: *const *const c_char,
    import_map_vals: *const *const c_char,
    import_map_count: c_int,
) -> CbmPipelineRegistryFuzzyResult {
    let no_match = || CbmPipelineRegistryFuzzyResult {
        result: empty_resolution(),
        ok: false,
    };
    if registry.is_null() || callee_name.is_null() || import_map_count < 0 {
        return no_match();
    }
    let Some(callee) = (unsafe { c_bytes(callee_name) }) else {
        return no_match();
    };
    let module = unsafe { c_bytes(module_qn) }.unwrap_or(&[]);
    let Some((_, vals)) =
        (unsafe { import_parts(import_map_keys, import_map_vals, import_map_count as usize) })
    else {
        return no_match();
    };
    let handle = unsafe { &*registry };
    handle
        .registry
        .fuzzy_resolve(callee, module, &vals)
        .map_or_else(no_match, |value| CbmPipelineRegistryFuzzyResult {
            result: handle.resolution(&value),
            ok: true,
        })
}

#[no_mangle]
/// # Safety
///
/// `registry` 與 `qualified_name` 必須是 null 或有效的 C 字串指標。
pub unsafe extern "C" fn cbm_registry_exists(
    registry: *const CbmPipelineRegistryHandle,
    qualified_name: *const c_char,
) -> bool {
    if registry.is_null() || qualified_name.is_null() {
        return false;
    }
    let Some(qn) = (unsafe { c_bytes(qualified_name) }) else {
        return false;
    };
    unsafe { &*registry }.registry.contains(qn)
}

#[no_mangle]
/// # Safety
///
/// `registry` 與 `qualified_name` 必須是 null 或有效的 C 字串指標。
pub unsafe extern "C" fn cbm_registry_label_of(
    registry: *const CbmPipelineRegistryHandle,
    qualified_name: *const c_char,
) -> *const c_char {
    if registry.is_null() || qualified_name.is_null() {
        return ptr::null();
    }
    let Some(qn) = (unsafe { c_bytes(qualified_name) }) else {
        return ptr::null();
    };
    unsafe { &*registry }
        .labels
        .get(qn)
        .map_or(ptr::null(), |value| value.as_ptr() as *const c_char)
}

#[no_mangle]
/// # Safety
///
/// `registry` 與輸出指標必須遵守既有 C ABI；回傳陣列由 registry 擁有，呼叫端不可釋放。
pub unsafe extern "C" fn cbm_registry_find_by_name(
    registry: *const CbmPipelineRegistryHandle,
    name: *const c_char,
    out: *mut *const *const c_char,
    count: *mut c_int,
) -> c_int {
    if registry.is_null() || name.is_null() || out.is_null() || count.is_null() {
        return -1;
    }
    let Some(name) = (unsafe { c_bytes(name) }) else {
        unsafe {
            *out = ptr::null();
            *count = 0;
        }
        return 0;
    };
    let handle = unsafe { &*registry };
    let Some(values) = handle.by_name_ptrs.get(name) else {
        unsafe {
            *out = ptr::null();
            *count = 0;
        }
        return 0;
    };
    unsafe {
        *out = values.as_ptr();
        *count = values.len() as c_int;
    }
    0
}

#[no_mangle]
/// # Safety
///
/// `registry` 必須是 null 或有效的 `cbm_registry_new` handle。
pub unsafe extern "C" fn cbm_registry_size(registry: *const CbmPipelineRegistryHandle) -> c_int {
    if registry.is_null() {
        return 0;
    }
    unsafe { &*registry }.registry.size() as c_int
}

unsafe fn write_pointer_array(values: &[*const c_char], out: *mut *const *const c_char) -> c_int {
    if out.is_null() {
        return 0;
    }
    if values.is_empty() {
        unsafe { *out = ptr::null() };
        return 0;
    }
    let Some(bytes) = values.len().checked_mul(size_of::<*const c_char>()) else {
        unsafe { *out = ptr::null() };
        return 0;
    };
    let buffer = unsafe { malloc(bytes) } as *mut *const c_char;
    if buffer.is_null() {
        unsafe { *out = ptr::null() };
        return 0;
    }
    unsafe {
        ptr::copy_nonoverlapping(values.as_ptr(), buffer, values.len());
        *out = buffer;
    }
    values.len() as c_int
}

#[no_mangle]
/// # Safety
///
/// `registry`、`suffix` 與 `out` 必須遵守既有 C ABI；`*out` 回傳的陣列由 C
/// `free` 釋放，陣列中的字串仍由 registry 擁有。
pub unsafe extern "C" fn cbm_registry_find_ending_with(
    registry: *const CbmPipelineRegistryHandle,
    suffix: *const c_char,
    out: *mut *const *const c_char,
) -> c_int {
    if registry.is_null() || suffix.is_null() || out.is_null() {
        if !out.is_null() {
            unsafe { *out = ptr::null() };
        }
        return 0;
    }
    let Some(suffix) = (unsafe { c_bytes(suffix) }) else {
        unsafe { *out = ptr::null() };
        return 0;
    };
    let handle = unsafe { &*registry };
    let values: Vec<*const c_char> = handle
        .registry
        .find_ending_with(suffix)
        .into_iter()
        .filter_map(|qn| handle.qn_ptr(qn))
        .collect();
    unsafe { write_pointer_array(&values, out) }
}

#[no_mangle]
/// # Safety
///
/// `candidate_qn` 與 import array 中的每個指標必須是有效的 C 字串；count 不可為負。
pub unsafe extern "C" fn cbm_registry_is_import_reachable(
    candidate_qn: *const c_char,
    import_vals: *const *const c_char,
    import_count: c_int,
) -> bool {
    if candidate_qn.is_null() || import_count < 0 {
        return false;
    }
    let Some(candidate) = (unsafe { c_bytes(candidate_qn) }) else {
        return false;
    };
    if import_count == 0 {
        return registry::is_import_reachable(candidate, &[]);
    }
    if import_vals.is_null() {
        return false;
    }
    let val_ptrs = unsafe { slice::from_raw_parts(import_vals, import_count as usize) };
    let mut vals = Vec::with_capacity(import_count as usize);
    for value in val_ptrs {
        let Some(value) = (unsafe { c_bytes(*value) }) else {
            return false;
        };
        vals.push(value);
    }
    registry::is_import_reachable(candidate, &vals)
}

#[no_mangle]
pub extern "C" fn cbm_confidence_band(score: f64) -> *const c_char {
    let value = if score >= 0.7 {
        BAND_HIGH
    } else if score >= 0.45 {
        BAND_MEDIUM
    } else if score >= 0.25 {
        BAND_SPECULATIVE
    } else {
        BAND_EMPTY
    };
    value.as_ptr() as *const c_char
}

#[no_mangle]
/// # Safety
///
/// `name` 必須是 null 或有效的 NUL 結尾 C 字串。
pub unsafe extern "C" fn cbm_perl_is_builtin(name: *const c_char) -> bool {
    let Some(name) = (unsafe { c_bytes(name) }) else {
        return false;
    };
    PERL_BUILTINS.contains(&name)
}

#[no_mangle]
/// # Safety
///
/// `callee_name` 與 `strategy` 必須是 null 或有效的 NUL 結尾 C 字串。
pub unsafe extern "C" fn cbm_perl_suppress_generic_match(
    is_perl: bool,
    is_method: bool,
    callee_name: *const c_char,
    strategy: *const c_char,
) -> bool {
    if !is_perl || (!is_method && !unsafe { cbm_perl_is_builtin(callee_name) }) {
        return false;
    }
    let Some(strategy) = (unsafe { c_bytes(strategy) }) else {
        return false;
    };
    if strategy.is_empty()
        || strategy == b"same_module"
        || strategy == b"import_map"
        || strategy == b"import_map_suffix"
    {
        return false;
    }
    true
}

#[no_mangle]
pub extern "C" fn cbm_registry_reach_cache_begin(_estimated_capacity: c_int) {}

#[no_mangle]
pub extern "C" fn cbm_registry_reach_cache_end() {}

#[no_mangle]
pub extern "C" fn cbm_registry_import_map_cache_begin(
    _keys: *const *const c_char,
    _vals: *const *const c_char,
    _count: c_int,
) {
}

#[no_mangle]
pub extern "C" fn cbm_registry_import_map_cache_end() {}

#[no_mangle]
pub extern "C" fn cbm_registry_resolve_cache_begin(_estimated_capacity: c_int) {}

#[no_mangle]
pub extern "C" fn cbm_registry_resolve_cache_end() {}
