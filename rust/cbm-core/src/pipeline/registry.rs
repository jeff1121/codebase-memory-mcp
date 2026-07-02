use std::collections::HashMap;

const REG_MAX_CANDIDATES: usize = 256;
const REG_MIN_CANDIDATES: usize = 3;
const REG_RESOLVED: i32 = 1;
const CONF_IMPORT_MAP: f64 = 0.95;
const CONF_IMPORT_MAP_SUFFIX: f64 = 0.85;
const CONF_SAME_MODULE: f64 = 0.90;
const CONF_QUALIFIED_SUFFIX: f64 = 0.90;
const CONF_UNIQUE_NAME: f64 = 0.75;
const CONF_SUFFIX_MATCH: f64 = 0.55;
const DEFAULT_CONFIDENCE: f64 = 0.5;
const REG_HALF_PENALTY: f64 = 0.5;
const CANDIDATE_PENALTY_CAP: f64 = 3.0;
const REG_TEST_PENALTY: i32 = 1000;

#[derive(Clone, Debug, PartialEq)]
pub struct Entry {
    pub qualified_name: Vec<u8>,
}

#[derive(Clone, Debug, PartialEq)]
pub struct Resolution {
    pub qualified_name: Vec<u8>,
    pub strategy: &'static str,
    pub confidence: f64,
    pub candidate_count: i32,
}

#[derive(Default)]
pub struct Registry {
    entries: Vec<Entry>,
    exact: HashMap<Vec<u8>, usize>,
    by_name: HashMap<Vec<u8>, Vec<usize>>,
}

impl Registry {
    pub fn from_entries(entries: &[Entry]) -> Self {
        let mut registry = Self::default();
        for entry in entries {
            registry.add(entry.qualified_name.clone());
        }
        registry
    }

    pub fn add(&mut self, qualified_name: Vec<u8>) {
        if qualified_name.is_empty() || self.exact.contains_key(&qualified_name) {
            return;
        }
        let idx = self.entries.len();
        let simple = simple_name(&qualified_name).to_vec();
        self.entries.push(Entry {
            qualified_name: qualified_name.clone(),
        });
        self.exact.insert(qualified_name, idx);
        self.by_name.entry(simple).or_default().push(idx);
    }

    pub fn resolve(
        &self,
        callee_name: &[u8],
        module_qn: &[u8],
        import_keys: &[&[u8]],
        import_vals: &[&[u8]],
    ) -> Option<Resolution> {
        if callee_name.is_empty() {
            return None;
        }
        let (prefix, suffix) = split_callee(callee_name);
        self.resolve_import_map(prefix, suffix, import_keys, import_vals)
            .or_else(|| self.resolve_same_module(callee_name, suffix, module_qn))
            .or_else(|| self.resolve_name_lookup(callee_name, module_qn, import_vals))
    }

    fn exact_qn(&self, qn: &[u8]) -> Option<&[u8]> {
        self.exact
            .get(qn)
            .map(|idx| self.entries[*idx].qualified_name.as_slice())
    }

    fn resolve_import_map(
        &self,
        prefix: &[u8],
        suffix: Option<&[u8]>,
        import_keys: &[&[u8]],
        import_vals: &[&[u8]],
    ) -> Option<Resolution> {
        if import_keys.is_empty() || import_vals.is_empty() {
            return None;
        }
        let resolved = import_keys
            .iter()
            .zip(import_vals.iter())
            .find_map(|(key, val)| (*key == prefix).then_some(*val))?;
        let candidate = join_dot(resolved, suffix.unwrap_or(prefix));
        if let Some(qn) = self.exact_qn(&candidate) {
            return Some(Resolution {
                qualified_name: qn.to_vec(),
                strategy: "import_map",
                confidence: CONF_IMPORT_MAP,
                candidate_count: REG_RESOLVED,
            });
        }
        let suffix = suffix?;
        let mut resolved_dot = resolved.to_vec();
        resolved_dot.push(b'.');
        let mut dot_suffix = Vec::with_capacity(suffix.len() + 1);
        dot_suffix.push(b'.');
        dot_suffix.extend_from_slice(suffix);
        let arr = self.by_name.get(simple_name(suffix))?;
        for idx in arr {
            let qn = self.entries[*idx].qualified_name.as_slice();
            if qn.starts_with(&resolved_dot) && qn.ends_with(&dot_suffix) {
                return Some(Resolution {
                    qualified_name: qn.to_vec(),
                    strategy: "import_map_suffix",
                    confidence: CONF_IMPORT_MAP_SUFFIX,
                    candidate_count: REG_RESOLVED,
                });
            }
        }
        None
    }

    fn resolve_same_module(
        &self,
        callee_name: &[u8],
        suffix: Option<&[u8]>,
        module_qn: &[u8],
    ) -> Option<Resolution> {
        for name in [Some(callee_name), suffix].into_iter().flatten() {
            let candidate = join_dot(module_qn, name);
            if let Some(qn) = self.exact_qn(&candidate) {
                return Some(Resolution {
                    qualified_name: qn.to_vec(),
                    strategy: "same_module",
                    confidence: CONF_SAME_MODULE,
                    candidate_count: REG_RESOLVED,
                });
            }
        }
        None
    }

    fn resolve_name_lookup(
        &self,
        callee_name: &[u8],
        module_qn: &[u8],
        import_vals: &[&[u8]],
    ) -> Option<Resolution> {
        let arr = self.by_name.get(simple_name(callee_name))?;
        if arr.is_empty() || arr.len() > REG_MAX_CANDIDATES {
            return None;
        }
        if arr.len() > 1 {
            if let Some(idx) = self.qualified_suffix_match(arr, callee_name) {
                return Some(Resolution {
                    qualified_name: self.entries[idx].qualified_name.clone(),
                    strategy: "qualified_suffix",
                    confidence: CONF_QUALIFIED_SUFFIX,
                    candidate_count: REG_RESOLVED,
                });
            }
        }
        if arr.len() == 1 {
            let qn = self.entries[arr[0]].qualified_name.as_slice();
            let mut confidence = CONF_UNIQUE_NAME;
            if !import_vals.is_empty() && !is_import_reachable(qn, import_vals) {
                confidence *= DEFAULT_CONFIDENCE;
            }
            return Some(Resolution {
                qualified_name: qn.to_vec(),
                strategy: "unique_name",
                confidence,
                candidate_count: REG_RESOLVED,
            });
        }
        if !import_vals.is_empty() {
            return self.resolve_multi_with_imports(arr, module_qn, import_vals);
        }
        let best = best_by_import_distance(self, arr.iter().copied(), module_qn)?;
        Some(Resolution {
            qualified_name: self.entries[best].qualified_name.clone(),
            strategy: "suffix_match",
            confidence: candidate_count_penalty(CONF_SUFFIX_MATCH, arr.len()),
            candidate_count: arr.len() as i32,
        })
    }

    fn resolve_multi_with_imports(
        &self,
        arr: &[usize],
        module_qn: &[u8],
        import_vals: &[&[u8]],
    ) -> Option<Resolution> {
        let filtered: Vec<usize> = arr
            .iter()
            .copied()
            .filter(|idx| {
                is_import_reachable(self.entries[*idx].qualified_name.as_slice(), import_vals)
            })
            .take(REG_MAX_CANDIDATES)
            .collect();
        if filtered.len() == 1 {
            return Some(Resolution {
                qualified_name: self.entries[filtered[0]].qualified_name.clone(),
                strategy: "suffix_match",
                confidence: candidate_count_penalty(CONF_SUFFIX_MATCH, arr.len()),
                candidate_count: arr.len() as i32,
            });
        }
        if filtered.len() > 1 {
            let best = best_by_import_distance(self, filtered.iter().copied(), module_qn)?;
            return Some(Resolution {
                qualified_name: self.entries[best].qualified_name.clone(),
                strategy: "suffix_match",
                confidence: candidate_count_penalty(CONF_SUFFIX_MATCH, filtered.len()),
                candidate_count: filtered.len() as i32,
            });
        }
        let best = best_by_import_distance(self, arr.iter().copied(), module_qn)?;
        Some(Resolution {
            qualified_name: self.entries[best].qualified_name.clone(),
            strategy: "suffix_match",
            confidence: candidate_count_penalty(CONF_SUFFIX_MATCH * REG_HALF_PENALTY, arr.len()),
            candidate_count: arr.len() as i32,
        })
    }

    fn qualified_suffix_match(&self, arr: &[usize], callee_name: &[u8]) -> Option<usize> {
        let dotted = normalize_qualified(callee_name);
        if !dotted.contains(&b'.') {
            return None;
        }
        let mut matched = None;
        for idx in arr {
            let qn = self.entries[*idx].qualified_name.as_slice();
            if qn.len() < dotted.len() {
                continue;
            }
            let tail_start = qn.len() - dotted.len();
            if &qn[tail_start..] != dotted.as_slice() {
                continue;
            }
            if tail_start > 0 && qn[tail_start - 1] != b'.' {
                continue;
            }
            if matched.is_some() {
                return None;
            }
            matched = Some(*idx);
        }
        matched
    }
}

fn simple_name(qn: &[u8]) -> &[u8] {
    let dot = qn.iter().rposition(|b| *b == b'.').map_or(0, |idx| idx + 1);
    let mut seg = dot;
    let mut i = 0;
    while i + 1 < qn.len() {
        if qn[i] == b':' && qn[i + 1] == b':' {
            seg = i + 2;
            i += 2;
        } else {
            i += 1;
        }
    }
    &qn[seg..]
}

fn split_callee(callee: &[u8]) -> (&[u8], Option<&[u8]>) {
    let dot = callee.iter().position(|b| *b == b'.').map(|idx| (idx, 1));
    let colons = callee
        .windows(2)
        .position(|w| w == b"::")
        .map(|idx| (idx, 2));
    let sep = match (dot, colons) {
        (Some(d), Some(c)) => Some(if c.0 < d.0 { c } else { d }),
        (Some(d), None) => Some(d),
        (None, Some(c)) => Some(c),
        (None, None) => None,
    };
    if let Some((idx, len)) = sep {
        (&callee[..idx], Some(&callee[idx + len..]))
    } else {
        (callee, None)
    }
}

fn join_dot(left: &[u8], right: &[u8]) -> Vec<u8> {
    let mut out = Vec::with_capacity(left.len() + right.len() + 1);
    out.extend_from_slice(left);
    out.push(b'.');
    out.extend_from_slice(right);
    out
}

fn contains_bytes(haystack: &[u8], needle: &[u8]) -> bool {
    needle.is_empty()
        || haystack
            .windows(needle.len())
            .any(|window| window == needle)
}

fn is_test_qn(qn: &[u8]) -> bool {
    const NEEDLES: &[&[u8]] = &[
        b"Test", b"test", b"Mock", b"mock", b"Stub", b"stub", b"Fake", b"fake", b"Fixture", b"spec",
    ];
    NEEDLES.iter().any(|needle| contains_bytes(qn, needle))
}

fn common_prefix_len(a: &[u8], b: &[u8]) -> i32 {
    a.split(|byte| *byte == b'.')
        .zip(b.split(|byte| *byte == b'.'))
        .take_while(|(left, right)| left == right)
        .count() as i32
}

fn candidate_score(candidate_qn: &[u8], module_qn: &[u8]) -> i32 {
    let mut score = 0;
    if !is_test_qn(candidate_qn) {
        score += REG_TEST_PENALTY;
    }
    score + common_prefix_len(candidate_qn, module_qn)
}

fn best_by_import_distance<I>(registry: &Registry, candidates: I, module_qn: &[u8]) -> Option<usize>
where
    I: IntoIterator<Item = usize>,
{
    let mut best = None;
    let mut best_score = i32::MIN;
    for idx in candidates {
        let score = candidate_score(registry.entries[idx].qualified_name.as_slice(), module_qn);
        if score > best_score {
            best_score = score;
            best = Some(idx);
        }
    }
    best
}

fn module_prefix(qn: &[u8]) -> &[u8] {
    qn.iter()
        .rposition(|b| *b == b'.')
        .map_or(qn, |idx| &qn[..idx])
}

fn is_import_reachable(candidate_qn: &[u8], import_vals: &[&[u8]]) -> bool {
    let cand_mod = module_prefix(candidate_qn);
    import_vals
        .iter()
        .any(|val| contains_bytes(cand_mod, val) || contains_bytes(val, cand_mod))
}

fn candidate_count_penalty(base: f64, count: usize) -> f64 {
    if count <= REG_MIN_CANDIDATES {
        return base;
    }
    base * 1.0_f64.min(CANDIDATE_PENALTY_CAP / count as f64)
}

fn normalize_qualified(callee: &[u8]) -> Vec<u8> {
    let mut out = Vec::with_capacity(callee.len());
    let mut i = 0;
    while i < callee.len() {
        if i + 1 < callee.len() && callee[i] == b':' && callee[i + 1] == b':' {
            out.push(b'.');
            i += 2;
        } else {
            out.push(callee[i]);
            i += 1;
        }
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    fn e(qn: &str) -> Entry {
        Entry {
            qualified_name: qn.as_bytes().to_vec(),
        }
    }

    #[test]
    fn resolves_qualified_suffix() {
        let r = Registry::from_entries(&[
            e("proj.lib.App.Alpha.save"),
            e("proj.lib.App.Beta.save"),
            e("proj.lib.App.Gamma.save"),
        ]);
        let res = r
            .resolve(b"App::Beta::save", b"proj.lib.App.Caller", &[], &[])
            .unwrap();
        assert_eq!(res.qualified_name, b"proj.lib.App.Beta.save");
        assert_eq!(res.strategy, "qualified_suffix");
    }

    #[test]
    fn candidate_cap_still_allows_same_module() {
        let mut entries = Vec::new();
        for i in 0..257 {
            entries.push(e(&format!("proj.mod{i}.flags")));
        }
        let r = Registry::from_entries(&entries);
        assert!(r.resolve(b"flags", b"proj.other", &[], &[]).is_none());
        let res = r.resolve(b"flags", b"proj.mod7", &[], &[]).unwrap();
        assert_eq!(res.qualified_name, b"proj.mod7.flags");
        assert_eq!(res.strategy, "same_module");
    }

    #[test]
    fn suffix_match_tie_keeps_first_candidate() {
        let r = Registry::from_entries(&[
            e("proj.alpha.Process"),
            e("proj.beta.Process"),
            e("proj.gamma.Process"),
        ]);
        let res = r.resolve(b"Process", b"proj.caller", &[], &[]).unwrap();
        assert_eq!(res.qualified_name, b"proj.alpha.Process");
        assert_eq!(res.strategy, "suffix_match");
    }
}
