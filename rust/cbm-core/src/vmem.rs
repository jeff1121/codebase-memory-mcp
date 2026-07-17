//! `vmem.c` 的頁面對齊算術 helper。

#[must_use]
pub fn round_to_page(size: usize, page_size: usize) -> usize {
    if page_size == 0 {
        return 0;
    }
    size.wrapping_add(page_size - 1) & !(page_size - 1)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rounds_sizes_to_power_of_two_pages() {
        assert_eq!(round_to_page(0, 4096), 0);
        assert_eq!(round_to_page(1, 4096), 4096);
        assert_eq!(round_to_page(4096, 4096), 4096);
        assert_eq!(round_to_page(4097, 4096), 8192);
    }

    #[test]
    fn handles_zero_page_size_without_panicking() {
        assert_eq!(round_to_page(123, 0), 0);
    }
}
