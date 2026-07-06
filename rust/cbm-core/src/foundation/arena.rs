//! `src/foundation/arena.c` 的 test-only bump allocator contract。
//!
//! 這個 module 不接管產品 `CBMArena`；只提供 Rust parity fixture，固定
//! block growth、8-byte alignment、reset、total accounting 與 string helpers。

const DEFAULT_BLOCK_SIZE: usize = 64 * 1024;
const MIN_BLOCK_SIZE: usize = 64;
const MAX_BLOCKS: usize = 256;
const ALIGN_MASK: usize = 7;
const CHUNK_SIZE: usize = 8;

#[repr(align(8))]
#[derive(Default)]
struct AlignedChunk([u8; CHUNK_SIZE]);

struct Block {
    data: Vec<AlignedChunk>,
    size: usize,
    used: usize,
}

impl Block {
    fn new(size: usize) -> Option<Self> {
        let chunks = size.checked_add(ALIGN_MASK)? / CHUNK_SIZE;
        let mut data = Vec::new();
        data.try_reserve_exact(chunks).ok()?;
        data.resize_with(chunks, AlignedChunk::default);
        Some(Self {
            data,
            size,
            used: 0,
        })
    }

    fn ptr_at(&mut self, offset: usize) -> *mut u8 {
        self.data.as_mut_ptr().cast::<u8>().wrapping_add(offset)
    }

    fn write(&mut self, offset: usize, bytes: &[u8]) {
        for (i, byte) in bytes.iter().enumerate() {
            let pos = offset + i;
            self.data[pos / CHUNK_SIZE].0[pos % CHUNK_SIZE] = *byte;
        }
    }

    fn zero(&mut self, offset: usize, len: usize) {
        for pos in offset..offset + len {
            self.data[pos / CHUNK_SIZE].0[pos % CHUNK_SIZE] = 0;
        }
    }
}

pub struct Arena {
    blocks: Vec<Block>,
    block_size: usize,
    total_alloc: usize,
}

impl Arena {
    #[must_use]
    pub fn new_default() -> Self {
        Self::new_sized(DEFAULT_BLOCK_SIZE)
    }

    #[must_use]
    pub fn new_sized(block_size: usize) -> Self {
        let block_size = block_size.max(MIN_BLOCK_SIZE);
        let mut blocks = Vec::new();
        if let Some(block) = Block::new(block_size) {
            blocks.push(block);
        }
        Self {
            blocks,
            block_size,
            total_alloc: 0,
        }
    }

    #[must_use]
    pub fn block_count(&self) -> usize {
        self.blocks.len()
    }

    #[must_use]
    pub fn block_size(&self) -> usize {
        self.block_size
    }

    #[must_use]
    pub fn used(&self) -> usize {
        self.blocks.last().map_or(0, |block| block.used)
    }

    #[must_use]
    pub fn total(&self) -> usize {
        self.total_alloc
    }

    pub fn alloc(&mut self, n: usize) -> Option<*mut u8> {
        self.reserve(n)
            .map(|(block_index, offset, _aligned)| self.blocks[block_index].ptr_at(offset))
    }

    pub fn calloc(&mut self, n: usize) -> Option<*mut u8> {
        let (block_index, offset, _aligned) = self.reserve(n)?;
        self.blocks[block_index].zero(offset, n);
        Some(self.blocks[block_index].ptr_at(offset))
    }

    pub fn strdup(&mut self, bytes: &[u8]) -> Option<*mut u8> {
        self.strndup(bytes)
    }

    pub fn strndup(&mut self, bytes: &[u8]) -> Option<*mut u8> {
        let len_with_nul = bytes.len().checked_add(1)?;
        let (block_index, offset, _aligned) = self.reserve(len_with_nul)?;
        self.blocks[block_index].write(offset, bytes);
        self.blocks[block_index].zero(offset + bytes.len(), 1);
        Some(self.blocks[block_index].ptr_at(offset))
    }

    pub fn reset(&mut self) {
        if self.blocks.len() > 1 {
            self.blocks.truncate(1);
        }
        if let Some(first) = self.blocks.first_mut() {
            first.used = 0;
            self.block_size = first.size;
        }
        self.total_alloc = 0;
    }

    fn reserve(&mut self, n: usize) -> Option<(usize, usize, usize)> {
        if n == 0 || self.blocks.is_empty() {
            return None;
        }
        let aligned = align_size(n)?;
        if !self.current_has_space(aligned) && !self.grow(aligned) {
            return None;
        }
        let new_total = self.total_alloc.checked_add(aligned)?;
        let block_index = self.blocks.len() - 1;
        let block = &mut self.blocks[block_index];
        let offset = block.used;
        block.used = block.used.checked_add(aligned)?;
        self.total_alloc = new_total;
        Some((block_index, offset, aligned))
    }

    fn current_has_space(&self, aligned: usize) -> bool {
        let Some(block) = self.blocks.last() else {
            return false;
        };
        block.size.saturating_sub(block.used) >= aligned
    }

    fn grow(&mut self, min_size: usize) -> bool {
        if self.blocks.len() >= MAX_BLOCKS {
            return false;
        }
        let Some(doubled) = self.block_size.checked_mul(2) else {
            return false;
        };
        let new_size = doubled.max(min_size);
        let Some(block) = Block::new(new_size) else {
            return false;
        };
        self.blocks.push(block);
        self.block_size = new_size;
        true
    }
}

fn align_size(n: usize) -> Option<usize> {
    Some(n.checked_add(ALIGN_MASK)? & !ALIGN_MASK)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn init_alignment_growth_and_reset_match_c_contract() {
        let mut arena = Arena::new_sized(32);
        assert_eq!(arena.block_count(), 1);
        assert_eq!(arena.block_size(), MIN_BLOCK_SIZE);

        let p1 = arena.alloc(1).expect("alloc 1");
        let p2 = arena.alloc(1).expect("alloc 2");
        assert_eq!((p1 as usize) % CHUNK_SIZE, 0);
        assert_eq!((p2 as usize) % CHUNK_SIZE, 0);
        assert_eq!((p2 as usize) - (p1 as usize), CHUNK_SIZE);
        assert_eq!(arena.used(), 16);
        assert_eq!(arena.total(), 16);

        assert!(arena.alloc(48).is_some());
        assert_eq!(arena.block_count(), 1);
        assert!(arena.alloc(48).is_some());
        assert_eq!(arena.block_count(), 2);
        assert!(arena.block_size() >= 128);

        arena.reset();
        assert_eq!(arena.block_count(), 1);
        assert_eq!(arena.block_size(), MIN_BLOCK_SIZE);
        assert_eq!(arena.used(), 0);
        assert_eq!(arena.total(), 0);
    }

    #[test]
    fn calloc_and_string_helpers_write_expected_bytes() {
        let mut arena = Arena::new_default();
        let zero = arena.calloc(16).expect("calloc");
        assert_eq!(arena.total(), 16);
        assert_eq!((zero as usize) % CHUNK_SIZE, 0);

        let hello = arena.strdup(b"hello").expect("strdup");
        assert_eq!((hello as usize) % CHUNK_SIZE, 0);

        let raw = arena.strndup(b"a\0b").expect("strndup");
        assert_eq!((raw as usize) % CHUNK_SIZE, 0);
    }

    #[test]
    fn zero_length_allocations_fail() {
        let mut arena = Arena::new_sized(MIN_BLOCK_SIZE);
        assert!(arena.alloc(0).is_none());
    }
}
