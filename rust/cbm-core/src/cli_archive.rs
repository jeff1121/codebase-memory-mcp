//! `src/cli/cli.c` archive parser 的純 byte helper parity。
//!
//! gzip 解壓、tar/zip entry 邊界、buffer ownership、檔案 I/O 與安裝流程仍由 C 負責。

pub const TAR_BLOCK_SIZE: usize = 512;

#[must_use]
pub fn is_tar_end_of_archive(header: Option<&[u8]>) -> bool {
    let Some(header) = header else {
        return false;
    };
    header.len() >= TAR_BLOCK_SIZE && header[..TAR_BLOCK_SIZE].iter().all(|byte| *byte == 0)
}

#[must_use]
pub fn zip_read_u16(data: Option<&[u8]>, offset: usize) -> Option<u16> {
    let data = data?;
    let bytes = data.get(offset..)?.get(..2)?;
    Some(u16::from_le_bytes([bytes[0], bytes[1]]))
}

#[must_use]
pub fn zip_read_u32(data: Option<&[u8]>, offset: usize) -> Option<u32> {
    let data = data?;
    let bytes = data.get(offset..)?.get(..4)?;
    Some(u32::from_le_bytes([bytes[0], bytes[1], bytes[2], bytes[3]]))
}

#[cfg(test)]
mod tests {
    use super::{is_tar_end_of_archive, zip_read_u16, zip_read_u32};

    #[test]
    fn tar_end_matches_c_contract() {
        assert!(is_tar_end_of_archive(Some(&[0; 512])));
        let mut header = [0_u8; 512];
        header[0] = 1;
        assert!(!is_tar_end_of_archive(Some(&header)));
        assert!(!is_tar_end_of_archive(Some(&[0; 511])));
        assert!(!is_tar_end_of_archive(None));
    }

    #[test]
    fn zip_u16_matches_c_little_endian_contract() {
        assert_eq!(zip_read_u16(Some(&[0x34, 0x12]), 0), Some(0x1234));
        assert_eq!(zip_read_u16(Some(&[0, 0x34, 0x12]), 1), Some(0x1234));
        assert_eq!(zip_read_u16(Some(&[0x34]), 0), None);
        assert_eq!(zip_read_u16(None, 0), None);
    }

    #[test]
    fn zip_u32_matches_c_little_endian_contract() {
        assert_eq!(
            zip_read_u32(Some(&[0x78, 0x56, 0x34, 0x12]), 0),
            Some(0x1234_5678)
        );
        assert_eq!(
            zip_read_u32(Some(&[0, 0x78, 0x56, 0x34, 0x12]), 1),
            Some(0x1234_5678)
        );
        assert_eq!(zip_read_u32(Some(&[0, 0, 0]), 0), None);
    }
}
