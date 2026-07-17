//! 對齊 `src/ui/layout3d.c` 的純視覺化 helper。
//!
//! Barnes-Hut、SQLite 查詢、layout 結果配置與 graph 所有權仍由 C 負責。

#[must_use]
pub fn stellar_color(degree: i32) -> u32 {
    if degree <= 1 {
        return 0xff6050;
    }
    if degree <= 3 {
        return 0xff8855;
    }
    if degree <= 5 {
        return 0xffa060;
    }
    if degree <= 8 {
        return 0xffc070;
    }
    if degree <= 12 {
        return 0xffe080;
    }
    if degree <= 18 {
        return 0xfff0c0;
    }
    if degree <= 25 {
        return 0xfff8e8;
    }
    if degree <= 35 {
        return 0xe8e8ff;
    }
    if degree <= 50 {
        return 0xc0d0ff;
    }
    0x80a0ff
}

#[must_use]
pub fn size_for_label(label: Option<&[u8]>) -> f32 {
    match label {
        Some(b"Project") => 20.0,
        Some(b"Package" | b"Module") => 15.0,
        Some(b"Folder") => 12.0,
        Some(b"File") => 8.0,
        Some(b"Class" | b"Struct" | b"Interface") => 6.0,
        _ => 4.0,
    }
}

#[must_use]
pub fn fnv1a(value: Option<&[u8]>) -> u32 {
    let mut hash = 2_166_136_261u32;
    if let Some(value) = value {
        for byte in value {
            hash ^= u32::from(*byte);
            hash = hash.wrapping_mul(16_777_619);
        }
    }
    hash
}

pub fn rand_float(seed: &mut u32) -> f32 {
    *seed = seed.wrapping_mul(1_103_515_245).wrapping_add(12_345);
    (((*seed >> 16) & 0x7fff) as f32 / 32_768.0) - 0.5
}

#[must_use]
pub fn octant(ox: f32, oy: f32, oz: f32, x: f32, y: f32, z: f32) -> i32 {
    i32::from(x >= ox) | (i32::from(y >= oy) << 1) | (i32::from(z >= oz) << 2)
}

#[must_use]
pub fn child_center(ox: f32, oy: f32, oz: f32, half: f32, child: i32) -> (f32, f32, f32) {
    let quarter = half * 0.5;
    (
        ox + if child & 1 != 0 { quarter } else { -quarter },
        oy + if child & 2 != 0 { quarter } else { -quarter },
        oz + if child & 4 != 0 { quarter } else { -quarter },
    )
}

#[repr(C)]
pub struct NodeIdEntry {
    pub id: i64,
    pub idx: i32,
}

#[must_use]
pub fn find_node_index(map: &[NodeIdEntry], id: i64) -> i32 {
    let mut low = 0usize;
    let mut high = map.len();
    while low < high {
        let middle = low + (high - low) / 2;
        match map[middle].id.cmp(&id) {
            std::cmp::Ordering::Equal => return map[middle].idx,
            std::cmp::Ordering::Less => low = middle + 1,
            std::cmp::Ordering::Greater => high = middle,
        }
    }
    -1
}

#[must_use]
pub fn clamp_max_nodes(requested: i32, cap: i32) -> i32 {
    if requested <= 0 || requested > cap {
        cap
    } else {
        requested
    }
}

#[cfg(test)]
mod tests {
    use super::{
        child_center, clamp_max_nodes, find_node_index, fnv1a, octant, rand_float, size_for_label,
        stellar_color, NodeIdEntry,
    };

    #[test]
    fn stellar_color_matches_degree_bands() {
        assert_eq!(stellar_color(0), 0xff6050);
        assert_eq!(stellar_color(3), 0xff8855);
        assert_eq!(stellar_color(12), 0xffe080);
        assert_eq!(stellar_color(50), 0xc0d0ff);
        assert_eq!(stellar_color(51), 0x80a0ff);
    }

    #[test]
    fn size_for_label_matches_layout_contract() {
        assert_eq!(size_for_label(Some(b"Project")), 20.0);
        assert_eq!(size_for_label(Some(b"Module")), 15.0);
        assert_eq!(size_for_label(Some(b"Folder")), 12.0);
        assert_eq!(size_for_label(Some(b"File")), 8.0);
        assert_eq!(size_for_label(Some(b"Interface")), 6.0);
        assert_eq!(size_for_label(Some(b"Function")), 4.0);
        assert_eq!(size_for_label(None), 4.0);
    }

    #[test]
    fn deterministic_hash_and_jitter_match_contract() {
        assert_eq!(fnv1a(Some(b"abc")), 0x1a47_e90b);
        assert_eq!(fnv1a(None), 2_166_136_261);
        let mut seed = 1;
        let value = rand_float(&mut seed);
        assert!((-0.5..0.5).contains(&value));
        assert_ne!(seed, 1);
    }

    #[test]
    fn octree_coordinate_helpers_match_contract() {
        assert_eq!(octant(0.0, 0.0, 0.0, 1.0, -1.0, 1.0), 5);
        assert_eq!(child_center(10.0, 20.0, 30.0, 8.0, 5), (14.0, 16.0, 34.0));
    }

    #[test]
    fn node_id_search_matches_sorted_map_contract() {
        let map = [
            NodeIdEntry { id: 10, idx: 2 },
            NodeIdEntry { id: 20, idx: 4 },
            NodeIdEntry { id: 30, idx: 6 },
        ];
        assert_eq!(find_node_index(&map, 20), 4);
        assert_eq!(find_node_index(&map, 25), -1);
        assert_eq!(find_node_index(&[], 20), -1);
    }

    #[test]
    fn max_nodes_clamp_matches_contract() {
        assert_eq!(clamp_max_nodes(0, 2000), 2000);
        assert_eq!(clamp_max_nodes(2500, 2000), 2000);
        assert_eq!(clamp_max_nodes(100, 2000), 100);
    }
}
