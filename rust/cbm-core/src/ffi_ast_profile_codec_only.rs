//! `ast_profile` 的純資料編碼與向量化 direct-only replacement。
//!
//! AST traversal 仍由 C/Tree-sitter 執行；本模組只取代 profile 的
//! comma-separated serialization、反序列化與 normalized vector 函式。

#![allow(unsafe_code)]

use std::ffi::CStr;
use std::os::raw::{c_char, c_int};
use std::ptr;

const FIELD_COUNT: usize = 25;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct CbmAstProfile {
    pub if_count: u16,
    pub for_count: u16,
    pub while_count: u16,
    pub switch_count: u16,
    pub try_count: u16,
    pub return_count: u16,
    pub max_nesting_depth: u16,
    pub avg_nesting_depth_x10: u16,
    pub comparison_ops: u16,
    pub arithmetic_ops: u16,
    pub logical_ops: u16,
    pub assignment_count: u16,
    pub string_literals: u16,
    pub number_literals: u16,
    pub bool_literals: u16,
    pub param_count: u16,
    pub params_in_returns: u16,
    pub params_in_conditions: u16,
    pub variable_reassigns: u16,
    pub unique_operators: u16,
    pub unique_operands: u16,
    pub total_operators: u16,
    pub total_operands: u16,
    pub body_lines: u16,
    pub body_tokens: u16,
}

impl CbmAstProfile {
    fn fields(self) -> [u16; FIELD_COUNT] {
        [
            self.if_count,
            self.for_count,
            self.while_count,
            self.switch_count,
            self.try_count,
            self.return_count,
            self.max_nesting_depth,
            self.avg_nesting_depth_x10,
            self.comparison_ops,
            self.arithmetic_ops,
            self.logical_ops,
            self.assignment_count,
            self.string_literals,
            self.number_literals,
            self.bool_literals,
            self.param_count,
            self.params_in_returns,
            self.params_in_conditions,
            self.variable_reassigns,
            self.unique_operators,
            self.unique_operands,
            self.total_operators,
            self.total_operands,
            self.body_lines,
            self.body_tokens,
        ]
    }

    fn set_fields(&mut self, fields: [u16; FIELD_COUNT]) {
        self.if_count = fields[0];
        self.for_count = fields[1];
        self.while_count = fields[2];
        self.switch_count = fields[3];
        self.try_count = fields[4];
        self.return_count = fields[5];
        self.max_nesting_depth = fields[6];
        self.avg_nesting_depth_x10 = fields[7];
        self.comparison_ops = fields[8];
        self.arithmetic_ops = fields[9];
        self.logical_ops = fields[10];
        self.assignment_count = fields[11];
        self.string_literals = fields[12];
        self.number_literals = fields[13];
        self.bool_literals = fields[14];
        self.param_count = fields[15];
        self.params_in_returns = fields[16];
        self.params_in_conditions = fields[17];
        self.variable_reassigns = fields[18];
        self.unique_operators = fields[19];
        self.unique_operands = fields[20];
        self.total_operators = fields[21];
        self.total_operands = fields[22];
        self.body_lines = fields[23];
        self.body_tokens = fields[24];
    }
}

fn parse_decimal(bytes: &[u8], mut pos: usize) -> Option<(u64, usize)> {
    while pos < bytes.len() && bytes[pos].is_ascii_whitespace() {
        pos += 1;
    }

    let negative = if bytes.get(pos) == Some(&b'-') {
        pos += 1;
        true
    } else {
        if bytes.get(pos) == Some(&b'+') {
            pos += 1;
        }
        false
    };

    let mut value = 0_u64;
    let mut has_digit = false;
    while pos < bytes.len() && bytes[pos].is_ascii_digit() {
        has_digit = true;
        value = value
            .wrapping_mul(10)
            .wrapping_add(u64::from(bytes[pos] - b'0'));
        pos += 1;
    }
    if !has_digit {
        return None;
    }
    if negative {
        value = 0_u64.wrapping_sub(value);
    }
    Some((value, pos))
}

/// 將 profile 編碼為與 C 實作相同的逗號分隔格式。
///
/// # Safety
/// `profile` 必須指向有效的 `CbmAstProfile`；若 `bufsize > 0`，`buf`
/// 必須指向至少 `bufsize` bytes 的可寫入記憶體。
#[no_mangle]
pub unsafe extern "C" fn cbm_ast_profile_to_str(
    profile: *const CbmAstProfile,
    buf: *mut c_char,
    bufsize: c_int,
) {
    if profile.is_null() || buf.is_null() || bufsize < 1 {
        if !buf.is_null() && bufsize > 0 {
            *buf = 0;
        }
        return;
    }

    let fields = (*profile).fields();
    let encoded = format!(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}",
        fields[0],
        fields[1],
        fields[2],
        fields[3],
        fields[4],
        fields[5],
        fields[6],
        fields[7],
        fields[8],
        fields[9],
        fields[10],
        fields[11],
        fields[12],
        fields[13],
        fields[14],
        fields[15],
        fields[16],
        fields[17],
        fields[18],
        fields[19],
        fields[20],
        fields[21],
        fields[22],
        fields[23],
        fields[24]
    );
    let copy_len = encoded.len().min((bufsize as usize) - 1);
    ptr::copy_nonoverlapping(encoded.as_ptr() as *const c_char, buf, copy_len);
    *buf.add(copy_len) = 0;
}

/// 從與 C 實作相同的逗號分隔格式解碼 profile。
///
/// # Safety
/// `str` 必須指向以 NUL 結尾的有效字串，`out` 必須指向可寫入的
/// `CbmAstProfile`。
#[no_mangle]
pub unsafe extern "C" fn cbm_ast_profile_from_str(
    str: *const c_char,
    out: *mut CbmAstProfile,
) -> bool {
    if str.is_null() || out.is_null() {
        return false;
    }

    let bytes = CStr::from_ptr(str).to_bytes();
    let mut fields = [0_u16; FIELD_COUNT];
    let mut pos = 0;
    for (index, field) in fields.iter_mut().enumerate() {
        let Some((value, next)) = parse_decimal(bytes, pos) else {
            return false;
        };
        *field = value as u16;
        pos = next;
        if index + 1 < FIELD_COUNT {
            if bytes.get(pos) != Some(&b',') {
                return false;
            }
            pos += 1;
        }
    }

    (*out).set_fields(fields);
    true
}

/// 將 profile 欄位正規化為與 C 實作相同的 25 維向量。
///
/// # Safety
/// `profile` 必須指向有效的 `CbmAstProfile`，`out` 必須至少有 25 個
/// 可寫入的 `float` 元素。
#[no_mangle]
pub unsafe extern "C" fn cbm_ast_profile_to_vector(profile: *const CbmAstProfile, out: *mut f32) {
    if profile.is_null() || out.is_null() {
        return;
    }

    let fields = (*profile).fields();
    let maxima = [
        100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 20.0, 200.0, 100.0, 100.0, 100.0, 100.0, 100.0,
        100.0, 100.0, 20.0, 100.0, 100.0, 100.0, 200.0, 200.0, 200.0, 200.0, 2000.0, 2000.0,
    ];
    for index in 0..FIELD_COUNT {
        *out.add(index) = f32::from(fields[index]) / maxima[index];
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn sample() -> CbmAstProfile {
        let mut profile = CbmAstProfile::default();
        profile.set_fields([
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
            25,
        ]);
        profile
    }

    #[test]
    fn serializes_and_round_trips() {
        let profile = sample();
        let mut encoded = [0_i8; 200];
        unsafe {
            cbm_ast_profile_to_str(&profile, encoded.as_mut_ptr(), encoded.len() as c_int);
        }
        let encoded = unsafe { CStr::from_ptr(encoded.as_ptr()) };
        let mut decoded = CbmAstProfile::default();
        assert!(unsafe { cbm_ast_profile_from_str(encoded.as_ptr(), &mut decoded) });
        assert_eq!(decoded, profile);
    }

    #[test]
    fn rejects_missing_fields() {
        let encoded = c"1,2";
        let mut decoded = CbmAstProfile::default();
        assert!(!unsafe { cbm_ast_profile_from_str(encoded.as_ptr(), &mut decoded) });
    }

    #[test]
    fn truncates_like_snprintf() {
        let profile = sample();
        let mut encoded = [b'x' as i8; 4];
        unsafe {
            cbm_ast_profile_to_str(&profile, encoded.as_mut_ptr(), encoded.len() as c_int);
        }
        assert_eq!(encoded, [b'1' as i8, b',' as i8, b'2' as i8, 0]);
    }

    #[test]
    fn normalizes_the_vector() {
        let profile = sample();
        let mut vector = [0.0_f32; FIELD_COUNT];
        unsafe {
            cbm_ast_profile_to_vector(&profile, vector.as_mut_ptr());
        }
        assert_eq!(vector[0], 0.01);
        assert_eq!(vector[6], 7.0 / 20.0);
        assert_eq!(vector[23], 24.0 / 2000.0);
        assert_eq!(vector[24], 25.0 / 2000.0);
    }
}
