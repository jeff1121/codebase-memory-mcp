//! 對齊 `src/cli/progress_sink.c` 的 progress sink 狀態機。

use std::ffi::CStr;
use std::sync::{Mutex, OnceLock};

const NOT_SET: i64 = -1;

#[repr(C)]
pub struct CFile {
    _private: [u8; 0],
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ProgressEmit {
    pub bytes: Vec<u8>,
    pub flush: bool,
}

impl ProgressEmit {
    fn new(bytes: Vec<u8>) -> Self {
        Self { bytes, flush: true }
    }
}

#[derive(Debug)]
pub struct ProgressSinkState {
    needs_newline: bool,
    gbuf_nodes: i64,
    gbuf_edges: i64,
}

impl Default for ProgressSinkState {
    fn default() -> Self {
        Self {
            needs_newline: false,
            gbuf_nodes: NOT_SET,
            gbuf_edges: NOT_SET,
        }
    }
}

impl ProgressSinkState {
    #[must_use]
    pub fn new() -> Self {
        Self::default()
    }

    pub fn reset(&mut self) {
        self.needs_newline = false;
        self.gbuf_nodes = NOT_SET;
        self.gbuf_edges = NOT_SET;
    }

    pub fn fini(&mut self) -> Option<ProgressEmit> {
        if !self.needs_newline {
            return None;
        }
        self.needs_newline = false;
        Some(ProgressEmit::new(b"\n".to_vec()))
    }

    pub fn handle_line(&mut self, line: &CStr) -> Option<ProgressEmit> {
        let line = line.to_bytes();
        let msg = extract_kv(line, b"msg")?;
        match msg {
            b"pipeline.discover" => Some(on_discover(line)),
            b"pipeline.route" => Some(on_route(line)),
            b"pass.start" => on_pass_start(line),
            b"pass.timing" => self.on_pass_timing(line),
            b"gbuf.dump" => {
                self.on_gbuf_dump(line);
                None
            }
            b"pipeline.done" => Some(self.on_done(line)),
            b"parallel.extract.progress" => Some(self.on_extract_progress(line)),
            _ => None,
        }
    }

    fn on_pass_timing(&mut self, line: &[u8]) -> Option<ProgressEmit> {
        let pass = extract_kv(line, b"pass")?;
        for phase in PHASES {
            if pass == phase.pass {
                let mut bytes = Vec::with_capacity(phase.label.len() + 2);
                if self.needs_newline {
                    bytes.push(b'\n');
                    self.needs_newline = false;
                }
                bytes.extend_from_slice(phase.label);
                bytes.push(b'\n');
                return Some(ProgressEmit::new(bytes));
            }
        }
        None
    }

    fn on_gbuf_dump(&mut self, line: &[u8]) {
        if let Some(nodes) = extract_kv(line, b"nodes") {
            self.gbuf_nodes = parse_i64_or_zero(nodes);
        }
        if let Some(edges) = extract_kv(line, b"edges") {
            self.gbuf_edges = parse_i64_or_zero(edges);
        }
    }

    fn on_done(&mut self, line: &[u8]) -> ProgressEmit {
        let mut bytes = Vec::new();
        if self.needs_newline {
            bytes.push(b'\n');
            self.needs_newline = false;
        }

        if self.gbuf_nodes >= 0 && self.gbuf_edges >= 0 {
            bytes.extend_from_slice(b"Done: ");
            append_i64(&mut bytes, self.gbuf_nodes);
            bytes.extend_from_slice(b" nodes, ");
            append_i64(&mut bytes, self.gbuf_edges);
            bytes.extend_from_slice(b" edges");
            if let Some(elapsed) = extract_kv(line, b"elapsed_ms") {
                bytes.extend_from_slice(b" (");
                bytes.extend_from_slice(elapsed);
                bytes.extend_from_slice(b" ms)");
            }
            bytes.push(b'\n');
        } else {
            bytes.extend_from_slice(b"Done.\n");
        }
        ProgressEmit::new(bytes)
    }

    fn on_extract_progress(&mut self, line: &[u8]) -> ProgressEmit {
        let done = extract_kv(line, b"done").map(parse_i64_or_zero);
        let total = extract_kv(line, b"total").map(parse_i64_or_zero);
        let mut bytes = Vec::new();
        if let (Some(done), Some(total)) = (done, total) {
            let pct = if total > 0 {
                ((i128::from(done) * 100) / i128::from(total)) as i64
            } else {
                0
            };
            bytes.extend_from_slice(b"\r  Extracting: ");
            append_i64(&mut bytes, done);
            bytes.extend_from_slice(b"/");
            append_i64(&mut bytes, total);
            bytes.extend_from_slice(b" files (");
            append_i64(&mut bytes, pct);
            bytes.extend_from_slice(b"%)");
        }
        self.needs_newline = true;
        ProgressEmit::new(bytes)
    }
}

struct PhaseLabel {
    pass: &'static [u8],
    label: &'static [u8],
}

const PHASES: &[PhaseLabel] = &[
    PhaseLabel {
        pass: b"parallel_extract",
        label: b"[2/9] Extracting definitions",
    },
    PhaseLabel {
        pass: b"registry_build",
        label: b"[3/9] Building registry",
    },
    PhaseLabel {
        pass: b"parallel_resolve",
        label: b"[4/9] Resolving calls & edges",
    },
    PhaseLabel {
        pass: b"tests",
        label: b"[5/9] Detecting tests",
    },
    PhaseLabel {
        pass: b"httplinks",
        label: b"[6/9] Scanning HTTP links",
    },
    PhaseLabel {
        pass: b"githistory_compute",
        label: b"[7/9] Analyzing git history",
    },
    PhaseLabel {
        pass: b"configlink",
        label: b"[8/9] Linking config files",
    },
    PhaseLabel {
        pass: b"dump",
        label: b"[9/9] Writing database",
    },
];

static STATE: OnceLock<Mutex<ProgressSinkState>> = OnceLock::new();

fn state() -> &'static Mutex<ProgressSinkState> {
    STATE.get_or_init(|| Mutex::new(ProgressSinkState::default()))
}

fn with_state<R>(f: impl FnOnce(&mut ProgressSinkState) -> R) -> R {
    let mut guard = match state().lock() {
        Ok(guard) => guard,
        Err(poisoned) => poisoned.into_inner(),
    };
    f(&mut guard)
}

pub fn reset() {
    with_state(|state| state.reset());
}

pub fn fini() -> Option<ProgressEmit> {
    with_state(|state| state.fini())
}

pub fn handle_line(line: &CStr) -> Option<ProgressEmit> {
    with_state(|state| state.handle_line(line))
}

fn on_discover(line: &[u8]) -> ProgressEmit {
    let mut bytes = Vec::new();
    if let Some(files) = extract_kv(line, b"files") {
        bytes.extend_from_slice(b"  Discovering files (");
        bytes.extend_from_slice(files);
        bytes.extend_from_slice(b" found)\n");
    } else {
        bytes.extend_from_slice(b"  Discovering files...\n");
    }
    ProgressEmit::new(bytes)
}

fn on_route(line: &[u8]) -> ProgressEmit {
    let mut bytes = Vec::new();
    match extract_kv(line, b"path") {
        Some(path) if path == b"incremental" => {
            bytes.extend_from_slice(b"  Starting incremental index\n")
        }
        _ => bytes.extend_from_slice(b"  Starting full index\n"),
    }
    ProgressEmit::new(bytes)
}

fn on_pass_start(line: &[u8]) -> Option<ProgressEmit> {
    match extract_kv(line, b"pass") {
        Some(pass) if pass == b"structure" => Some(ProgressEmit::new(
            b"[1/9] Building file structure\n".to_vec(),
        )),
        _ => None,
    }
}

fn extract_kv<'a>(line: &'a [u8], key: &[u8]) -> Option<&'a [u8]> {
    if line.is_empty() || key.is_empty() {
        return None;
    }

    let mut i = 0;
    while i < line.len() {
        if (i == 0 || line[i - 1] == b' ')
            && line[i..].starts_with(key)
            && i + key.len() < line.len()
            && line[i + key.len()] == b'='
        {
            let start = i + key.len() + 1;
            let mut end = start;
            while end < line.len() && line[end] != b' ' {
                end += 1;
            }
            return Some(&line[start..end]);
        }
        i += 1;
    }
    None
}

fn parse_i64_or_zero(value: &[u8]) -> i64 {
    std::str::from_utf8(value)
        .ok()
        .and_then(|text| text.parse::<i64>().ok())
        .unwrap_or(0)
}

fn append_i64(bytes: &mut Vec<u8>, value: i64) {
    bytes.extend_from_slice(value.to_string().as_bytes());
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CStr;

    #[test]
    fn formats_known_events_like_the_c_sink() {
        let mut state = ProgressSinkState::new();

        let discover = CStr::from_bytes_with_nul(b"level=info msg=pipeline.discover files=12\0")
            .expect("valid C string");
        let route = CStr::from_bytes_with_nul(b"level=info msg=pipeline.route path=incremental\0")
            .expect("valid C string");
        let pass_start = CStr::from_bytes_with_nul(b"level=info msg=pass.start pass=structure\0")
            .expect("valid C string");
        let progress =
            CStr::from_bytes_with_nul(b"level=info msg=parallel.extract.progress done=2 total=5\0")
                .expect("valid C string");
        let timing =
            CStr::from_bytes_with_nul(b"level=info msg=pass.timing pass=parallel_extract\0")
                .expect("valid C string");
        let dump = CStr::from_bytes_with_nul(b"level=info msg=gbuf.dump nodes=10 edges=20\0")
            .expect("valid C string");
        let done = CStr::from_bytes_with_nul(b"level=info msg=pipeline.done elapsed_ms=42\0")
            .expect("valid C string");

        assert_eq!(
            state.handle_line(discover).expect("discover emit").bytes,
            b"  Discovering files (12 found)\n"
        );
        assert_eq!(
            state.handle_line(route).expect("route emit").bytes,
            b"  Starting incremental index\n"
        );
        assert_eq!(
            state
                .handle_line(pass_start)
                .expect("pass start emit")
                .bytes,
            b"[1/9] Building file structure\n"
        );
        assert_eq!(
            state.handle_line(progress).expect("progress emit").bytes,
            b"\r  Extracting: 2/5 files (40%)"
        );
        assert_eq!(
            state.handle_line(timing).expect("timing emit").bytes,
            b"\n[2/9] Extracting definitions\n"
        );
        assert!(state.handle_line(dump).is_none());
        assert_eq!(
            state.handle_line(done).expect("done emit").bytes,
            b"Done: 10 nodes, 20 edges (42 ms)\n"
        );
    }

    #[test]
    fn flushes_pending_progress_on_fini() {
        let mut state = ProgressSinkState::new();
        let progress =
            CStr::from_bytes_with_nul(b"level=info msg=parallel.extract.progress done=2 total=5\0")
                .expect("valid C string");

        assert_eq!(
            state.handle_line(progress).expect("progress emit").bytes,
            b"\r  Extracting: 2/5 files (40%)"
        );
        assert_eq!(state.fini().expect("newline emit").bytes, b"\n");
        assert!(state.fini().is_none());
    }
}
