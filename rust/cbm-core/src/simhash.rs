//! `src/simhash/minhash.c` 的固定長度 MinHash 相似度計算。

pub const MINHASH_K: usize = 64;
pub const MINHASH_HEX_LEN: usize = MINHASH_K * 8;
pub const MINHASH_HEX_BUF: usize = MINHASH_HEX_LEN + 1;

#[must_use]
pub fn jaccard(a: Option<&[u32]>, b: Option<&[u32]>) -> f64 {
    let (Some(a), Some(b)) = (a, b) else {
        return 0.0;
    };
    if a.len() < MINHASH_K || b.len() < MINHASH_K {
        return 0.0;
    }

    let matching = a[..MINHASH_K]
        .iter()
        .zip(&b[..MINHASH_K])
        .filter(|(left, right)| left == right)
        .count();
    matching as f64 / MINHASH_K as f64
}

#[must_use]
pub fn to_hex(values: Option<&[u32]>, output_size: usize) -> Option<Vec<u8>> {
    let values = values?;
    if values.len() < MINHASH_K || output_size < MINHASH_HEX_BUF {
        return None;
    }

    let mut output = Vec::with_capacity(MINHASH_HEX_BUF);
    for value in &values[..MINHASH_K] {
        output.extend(format!("{value:08x}").as_bytes());
    }
    output.push(0);
    Some(output)
}

#[must_use]
pub fn from_hex(value: Option<&[u8]>) -> Option<[u32; MINHASH_K]> {
    let value = value?;
    if value.len() != MINHASH_HEX_LEN {
        return None;
    }

    let mut output = [0_u32; MINHASH_K];
    for (index, chunk) in value.chunks_exact(8).enumerate() {
        output[index] = parse_c_hex_chunk(chunk);
    }
    Some(output)
}

#[must_use]
pub fn parse_fp_from_props(props: Option<&[u8]>) -> Option<[u32; MINHASH_K]> {
    let props = props?;
    let needle = b"\"fp\":\"";
    let start = props
        .windows(needle.len())
        .position(|window| window == needle)?
        + needle.len();
    let remainder = &props[start..];
    let end = remainder.iter().position(|byte| *byte == b'"')?;
    if end != MINHASH_HEX_LEN {
        return None;
    }
    from_hex(Some(&remainder[..end]))
}

fn parse_c_hex_chunk(chunk: &[u8]) -> u32 {
    let mut index = 0;
    while chunk
        .get(index)
        .is_some_and(|byte| matches!(*byte, b' ' | b'\t' | b'\n' | b'\r' | 0x0b | 0x0c))
    {
        index += 1;
    }

    let negative = chunk.get(index) == Some(&b'-');
    if negative || chunk.get(index) == Some(&b'+') {
        index += 1;
    }
    if chunk.get(index) == Some(&b'0')
        && chunk
            .get(index + 1)
            .is_some_and(|byte| matches!(*byte, b'x' | b'X'))
    {
        index += 2;
    }

    let mut parsed = 0_u32;
    let mut found = false;
    while let Some(byte) = chunk.get(index) {
        let Some(nibble) = hex_nibble(*byte) else {
            break;
        };
        found = true;
        parsed = parsed.wrapping_mul(16).wrapping_add(nibble);
        index += 1;
    }
    if found && negative {
        0_u32.wrapping_sub(parsed)
    } else if found {
        parsed
    } else {
        0
    }
}

fn hex_nibble(byte: u8) -> Option<u32> {
    match byte {
        b'0'..=b'9' => Some(u32::from(byte - b'0')),
        b'a'..=b'f' => Some(u32::from(byte - b'a' + 10)),
        b'A'..=b'F' => Some(u32::from(byte - b'A' + 10)),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn jaccard_handles_null_and_fixed_width_inputs() {
        let values = [7_u32; MINHASH_K];
        assert_eq!(jaccard(None, Some(&values)), 0.0);
        assert_eq!(jaccard(Some(&values[..MINHASH_K - 1]), Some(&values)), 0.0);
        assert_eq!(jaccard(Some(&values), Some(&values)), 1.0);
    }

    #[test]
    fn jaccard_counts_matching_slots() {
        let left = [1_u32; MINHASH_K];
        let mut right = left;
        right[..16].fill(2);
        assert_eq!(jaccard(Some(&left), Some(&right)), 0.75);
    }

    #[test]
    fn hex_roundtrip_preserves_signature() {
        let mut values = [0_u32; MINHASH_K];
        for (index, value) in values.iter_mut().enumerate() {
            *value = 0xdead_0000 + index as u32;
        }
        let encoded = to_hex(Some(&values), MINHASH_HEX_BUF).expect("有效輸出緩衝區");
        assert_eq!(encoded.len(), MINHASH_HEX_BUF);
        assert_eq!(from_hex(Some(&encoded[..MINHASH_HEX_LEN])), Some(values));
    }
}

#[cfg(test)]
mod props_tests {
    use super::parse_fp_from_props;

    #[test]
    fn parses_fixed_length_fingerprint_property() {
        let mut props = b"{\"fp\":\"".to_vec();
        props.extend([b'0'; super::MINHASH_HEX_LEN]);
        props.extend_from_slice(b"\"}");
        assert!(parse_fp_from_props(Some(&props)).is_some());
        assert!(parse_fp_from_props(Some(b"{\"fp\":\"00\"}")).is_none());
        assert!(parse_fp_from_props(None).is_none());
    }
}
