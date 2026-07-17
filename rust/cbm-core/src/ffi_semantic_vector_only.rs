//! `semantic.c` 的 dense-vector primitive direct-only replacement。
//!
//! 本模組只取代 768 維向量的 cosine、normalize 與 scaled-add；
//! pretrained lookup、random indexing、corpus 與 diffusion 仍由 C 提供。

#![allow(unsafe_code)]

use std::os::raw::c_float;

const DIM: usize = 768;
const DENOM_EPS: f32 = 1.0e-10;

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct CbmSemVec {
    pub v: [c_float; DIM],
}

/// 計算兩個 768 維 dense vectors 的 cosine similarity。
///
/// # Safety
/// `a` 與 `b` 若非 NULL，必須各自指向有效的 `CbmSemVec`。
#[no_mangle]
pub unsafe extern "C" fn cbm_sem_cosine(a: *const CbmSemVec, b: *const CbmSemVec) -> c_float {
    if a.is_null() || b.is_null() {
        return 0.0;
    }
    let a = &*a;
    let b = &*b;
    let dot: f32 = a.v.iter().zip(b.v.iter()).map(|(x, y)| x * y).sum();
    let mag_a: f32 = a.v.iter().map(|value| value * value).sum();
    let mag_b: f32 = b.v.iter().map(|value| value * value).sum();
    let denom = mag_a.sqrt() * mag_b.sqrt();
    if denom < DENOM_EPS {
        0.0
    } else {
        dot / denom
    }
}

/// 將 dense vector 正規化為 unit length；接近零向量時維持原值。
///
/// # Safety
/// `vector` 若非 NULL，必須指向可寫入的有效 `CbmSemVec`。
#[no_mangle]
pub unsafe extern "C" fn cbm_sem_normalize(vector: *mut CbmSemVec) {
    if vector.is_null() {
        return;
    }
    let vector = &mut *vector;
    let magnitude: f32 = vector
        .v
        .iter()
        .map(|value| value * value)
        .sum::<f32>()
        .sqrt();
    if magnitude < DENOM_EPS {
        return;
    }
    let inverse = 1.0 / magnitude;
    for value in &mut vector.v {
        *value *= inverse;
    }
}

/// 將 `scale * src` 加到 destination vector。
///
/// # Safety
/// `dst` 與 `src` 若非 NULL，必須各自指向有效的 `CbmSemVec`。
#[no_mangle]
pub unsafe extern "C" fn cbm_sem_vec_add_scaled(
    dst: *mut CbmSemVec,
    src: *const CbmSemVec,
    scale: c_float,
) {
    if dst.is_null() || src.is_null() {
        return;
    }
    for index in 0..DIM {
        let src_value = *src.cast::<c_float>().add(index);
        *dst.cast::<c_float>().add(index) += scale * src_value;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn cosine_handles_unit_orthogonal_and_null_vectors() {
        let mut a = CbmSemVec { v: [0.0; DIM] };
        let mut b = CbmSemVec { v: [0.0; DIM] };
        a.v[0] = 1.0;
        b.v[0] = 1.0;
        assert!((unsafe { cbm_sem_cosine(&a, &b) } - 1.0).abs() < f32::EPSILON);
        b.v[0] = 0.0;
        b.v[1] = 1.0;
        assert_eq!(unsafe { cbm_sem_cosine(&a, &b) }, 0.0);
        assert_eq!(unsafe { cbm_sem_cosine(std::ptr::null(), &b) }, 0.0);
    }

    #[test]
    fn normalize_matches_euclidean_unit_length() {
        let mut vector = CbmSemVec { v: [0.0; DIM] };
        vector.v[0] = 3.0;
        vector.v[1] = 4.0;
        unsafe { cbm_sem_normalize(&mut vector) };
        assert!((vector.v[0] - 0.6).abs() < 1.0e-6);
        assert!((vector.v[1] - 0.8).abs() < 1.0e-6);
        assert!((vector.v.iter().map(|value| value * value).sum::<f32>() - 1.0).abs() < 1.0e-6);
    }

    #[test]
    fn scaled_add_preserves_dimension_order() {
        let mut dst = CbmSemVec { v: [0.0; DIM] };
        let mut src = CbmSemVec { v: [0.0; DIM] };
        dst.v[0] = 2.0;
        src.v[0] = 3.0;
        src.v[DIM - 1] = -4.0;
        unsafe { cbm_sem_vec_add_scaled(&mut dst, &src, 0.5) };
        assert_eq!(dst.v[0], 3.5);
        assert_eq!(dst.v[DIM - 1], -2.0);
    }
}
