//! 對齊 `src/foundation/platform.c` 與 Linux cgroup helpers 的低風險邏輯。

use std::fs::File;
use std::io::Read;
use std::path::Path;
#[cfg(unix)]
use std::{
    ffi::OsStr,
    os::unix::ffi::{OsStrExt, OsStringExt},
};

const SMALL_FILE_LIMIT: usize = 63;
const ENV_TMP_VALUE_LIMIT: usize = 255;

pub fn normalize_path_sep_bytes(path: &mut [u8]) {
    for byte in path.iter_mut() {
        if *byte == b'\\' {
            *byte = b'/';
        }
    }
    canonicalize_drive(path);
}

fn canonicalize_drive(path: &mut [u8]) {
    if path.len() < 2 {
        return;
    }
    let has_drive_suffix = path.len() == 2 || path.get(2).copied() == Some(b'/');
    if path[0].is_ascii_lowercase() && path[1] == b':' && has_drive_suffix {
        path[0] = path[0].to_ascii_uppercase();
    }
}

pub fn safe_getenv_bytes(name: Option<&[u8]>, fallback: Option<&[u8]>) -> Option<Vec<u8>> {
    let name = name?;
    env_value_bytes(name).or_else(|| fallback.map(<[u8]>::to_vec))
}

pub fn get_home_dir_bytes() -> Option<Vec<u8>> {
    let mut home =
        non_empty_env_tmp_bytes(b"HOME").or_else(|| non_empty_env_tmp_bytes(b"USERPROFILE"))?;
    normalize_path_sep_bytes(&mut home);
    Some(home)
}

#[cfg(windows)]
pub fn app_config_dir_bytes() -> Option<Vec<u8>> {
    if let Some(mut appdata) = non_empty_env_tmp_bytes(b"APPDATA") {
        normalize_path_sep_bytes(&mut appdata);
        return Some(appdata);
    }
    let mut home = get_home_dir_bytes()?;
    home.extend_from_slice(b"/AppData/Roaming");
    Some(home)
}

#[cfg(not(windows))]
pub fn app_config_dir_bytes() -> Option<Vec<u8>> {
    if let Some(config) = non_empty_env_tmp_bytes(b"XDG_CONFIG_HOME") {
        return Some(config);
    }
    let mut home = get_home_dir_bytes()?;
    home.extend_from_slice(b"/.config");
    Some(home)
}

#[cfg(windows)]
pub fn app_local_dir_bytes() -> Option<Vec<u8>> {
    if let Some(mut local) = non_empty_env_tmp_bytes(b"LOCALAPPDATA") {
        normalize_path_sep_bytes(&mut local);
        return Some(local);
    }
    let mut home = get_home_dir_bytes()?;
    home.extend_from_slice(b"/AppData/Local");
    Some(home)
}

#[cfg(not(windows))]
pub fn app_local_dir_bytes() -> Option<Vec<u8>> {
    app_config_dir_bytes()
}

pub fn resolve_cache_dir_bytes() -> Option<Vec<u8>> {
    if let Some(mut cache) = non_empty_env_tmp_bytes(b"CBM_CACHE_DIR") {
        normalize_path_sep_bytes(&mut cache);
        return Some(cache);
    }
    let mut home = get_home_dir_bytes()?;
    home.extend_from_slice(b"/.cache/codebase-memory-mcp");
    Some(home)
}

fn non_empty_env_tmp_bytes(name: &[u8]) -> Option<Vec<u8>> {
    env_tmp_bytes(name).filter(|value| !value.is_empty())
}

fn env_tmp_bytes(name: &[u8]) -> Option<Vec<u8>> {
    let mut value = env_value_bytes(name)?;
    value.truncate(ENV_TMP_VALUE_LIMIT);
    Some(value)
}

#[cfg(unix)]
fn env_value_bytes(name: &[u8]) -> Option<Vec<u8>> {
    if name.contains(&b'=') || name.contains(&0) {
        return None;
    }
    std::env::var_os(OsStr::from_bytes(name)).map(OsStringExt::into_vec)
}

#[cfg(windows)]
fn env_value_bytes(name: &[u8]) -> Option<Vec<u8>> {
    if name.contains(&b'=') || name.contains(&0) {
        return None;
    }
    let name = std::str::from_utf8(name).ok()?;
    std::env::var_os(name).map(|value| value.to_string_lossy().into_owned().into_bytes())
}

#[cfg(not(any(unix, windows)))]
fn env_value_bytes(name: &[u8]) -> Option<Vec<u8>> {
    if name.contains(&b'=') || name.contains(&0) {
        return None;
    }
    let name = std::str::from_utf8(name).ok()?;
    std::env::var_os(name).map(|value| value.to_string_lossy().into_owned().into_bytes())
}

pub fn detect_cgroup_cpus(root: &Path) -> i32 {
    if let Some(buf) = read_small_file(&root.join("cpu.max")) {
        if buf.starts_with(b"max") {
            return -1;
        }
        let mut parts = buf.split(|byte| byte.is_ascii_whitespace());
        let quota = parts.next().and_then(parse_i64_prefix);
        let period = parts.next().and_then(parse_i64_prefix);
        if let (Some(quota), Some(period)) = (quota, period) {
            if quota > 0 && period > 0 {
                let n = (quota + period - 1) / period;
                return i32::try_from(n.max(1)).unwrap_or(i32::MAX);
            }
        }
        return -1;
    }

    let Some(buf) = read_small_file(&root.join("cpu/cpu.cfs_quota_us")) else {
        return -1;
    };
    let Some(quota) = parse_i64_prefix(&buf) else {
        return -1;
    };
    if quota <= 0 {
        return -1;
    }

    let Some(buf) = read_small_file(&root.join("cpu/cpu.cfs_period_us")) else {
        return -1;
    };
    let Some(period) = parse_i64_prefix(&buf) else {
        return -1;
    };
    if period <= 0 {
        return -1;
    }

    let n = (quota + period - 1) / period;
    i32::try_from(n.max(1)).unwrap_or(i32::MAX)
}

pub fn detect_cgroup_mem(root: &Path) -> usize {
    if let Some(buf) = read_small_file(&root.join("memory.max")) {
        if buf.starts_with(b"max") {
            return 0;
        }
        return parse_u64_prefix(&buf)
            .filter(|value| *value > 0)
            .and_then(|value| usize::try_from(value).ok())
            .unwrap_or(0);
    }

    let Some(buf) = read_small_file(&root.join("memory/memory.limit_in_bytes")) else {
        return 0;
    };
    let Some(n) = parse_u64_prefix(&buf) else {
        return 0;
    };
    if n == 0 || n >= (u64::MAX / 2) {
        return 0;
    }
    usize::try_from(n).unwrap_or(usize::MAX)
}

fn read_small_file(path: &Path) -> Option<Vec<u8>> {
    let mut file = File::open(path).ok()?;
    let mut buf = vec![0u8; SMALL_FILE_LIMIT];
    let mut n = file.read(&mut buf).ok()?;
    buf.truncate(n);
    while n > 0 && matches!(buf[n - 1], b'\n' | b' ' | b'\t') {
        n -= 1;
    }
    buf.truncate(n);
    Some(buf)
}

fn parse_i64_prefix(buf: &[u8]) -> Option<i64> {
    let mut idx = skip_ascii_whitespace(buf, 0);
    let mut sign = 1i64;
    if idx < buf.len() {
        match buf[idx] {
            b'-' => {
                sign = -1;
                idx += 1;
            }
            b'+' => idx += 1,
            _ => {}
        }
    }

    let start = idx;
    let mut value = 0i64;
    while idx < buf.len() && buf[idx].is_ascii_digit() {
        value = value
            .saturating_mul(10)
            .saturating_add((buf[idx] - b'0') as i64);
        idx += 1;
    }
    (idx > start).then_some(value.saturating_mul(sign))
}

fn parse_u64_prefix(buf: &[u8]) -> Option<u64> {
    let mut idx = skip_ascii_whitespace(buf, 0);
    if idx < buf.len() && buf[idx] == b'+' {
        idx += 1;
    }

    let start = idx;
    let mut value = 0u64;
    while idx < buf.len() && buf[idx].is_ascii_digit() {
        value = value
            .saturating_mul(10)
            .saturating_add((buf[idx] - b'0') as u64);
        idx += 1;
    }
    (idx > start).then_some(value)
}

fn skip_ascii_whitespace(buf: &[u8], mut idx: usize) -> usize {
    while idx < buf.len() && buf[idx].is_ascii_whitespace() {
        idx += 1;
    }
    idx
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::OsString;
    use std::fs;
    use std::sync::Mutex;
    use std::time::{SystemTime, UNIX_EPOCH};

    static ENV_LOCK: Mutex<()> = Mutex::new(());

    #[test]
    fn normalize_path_sep_matches_c_cases() {
        let mut path = b"c:\\Users\\test".to_vec();
        normalize_path_sep_bytes(&mut path);
        assert_eq!(path, b"C:/Users/test");

        let mut bare_drive = b"z:".to_vec();
        normalize_path_sep_bytes(&mut bare_drive);
        assert_eq!(bare_drive, b"Z:");

        let mut non_drive = b"abc:def\\x".to_vec();
        normalize_path_sep_bytes(&mut non_drive);
        assert_eq!(non_drive, b"abc:def/x");
    }

    #[test]
    fn env_dirs_match_c_precedence_and_normalization() {
        let _lock = ENV_LOCK.lock().unwrap();
        let _env = EnvSnapshot::new(&[
            "HOME",
            "USERPROFILE",
            "XDG_CONFIG_HOME",
            "APPDATA",
            "LOCALAPPDATA",
            "CBM_CACHE_DIR",
            "CBM_RS_TEST_ENV",
        ]);

        std::env::set_var("CBM_RS_TEST_ENV", "abcdef");
        assert_eq!(
            safe_getenv_bytes(Some(b"CBM_RS_TEST_ENV"), Some(b"fallback")).unwrap(),
            b"abcdef"
        );
        std::env::remove_var("CBM_RS_TEST_ENV");
        assert_eq!(
            safe_getenv_bytes(Some(b"CBM_RS_TEST_ENV"), Some(b"fallback")).unwrap(),
            b"fallback"
        );
        assert!(safe_getenv_bytes(Some(b"CBM_RS_TEST_ENV"), None).is_none());
        assert!(safe_getenv_bytes(None, Some(b"fallback")).is_none());

        std::env::set_var("HOME", "c:\\Home\\Primary");
        std::env::set_var("USERPROFILE", "d:\\Profile");
        std::env::remove_var("XDG_CONFIG_HOME");
        std::env::remove_var("APPDATA");
        std::env::remove_var("LOCALAPPDATA");
        std::env::remove_var("CBM_CACHE_DIR");

        assert_eq!(get_home_dir_bytes().unwrap(), b"C:/Home/Primary");
        #[cfg(windows)]
        {
            assert_eq!(
                app_config_dir_bytes().unwrap(),
                b"C:/Home/Primary/AppData/Roaming"
            );
            assert_eq!(
                app_local_dir_bytes().unwrap(),
                b"C:/Home/Primary/AppData/Local"
            );
        }
        #[cfg(not(windows))]
        {
            assert_eq!(app_config_dir_bytes().unwrap(), b"C:/Home/Primary/.config");
            assert_eq!(app_local_dir_bytes().unwrap(), b"C:/Home/Primary/.config");
        }
        assert_eq!(
            resolve_cache_dir_bytes().unwrap(),
            b"C:/Home/Primary/.cache/codebase-memory-mcp"
        );

        std::env::remove_var("HOME");
        assert_eq!(get_home_dir_bytes().unwrap(), b"D:/Profile");

        std::env::set_var("CBM_CACHE_DIR", "e:\\Cache\\Root");
        assert_eq!(resolve_cache_dir_bytes().unwrap(), b"E:/Cache/Root");

        #[cfg(windows)]
        {
            std::env::set_var("APPDATA", "f:\\Config\\Roaming");
            std::env::set_var("LOCALAPPDATA", "g:\\Config\\Local");
            assert_eq!(app_config_dir_bytes().unwrap(), b"F:/Config/Roaming");
            assert_eq!(app_local_dir_bytes().unwrap(), b"G:/Config/Local");
        }
        #[cfg(not(windows))]
        {
            std::env::set_var("XDG_CONFIG_HOME", "/tmp/cbm-rs-config");
            assert_eq!(app_config_dir_bytes().unwrap(), b"/tmp/cbm-rs-config");
            assert_eq!(app_local_dir_bytes().unwrap(), b"/tmp/cbm-rs-config");
        }
    }

    #[test]
    fn env_dir_helpers_keep_c_tmp_truncation() {
        let _lock = ENV_LOCK.lock().unwrap();
        let _env = EnvSnapshot::new(&["HOME", "USERPROFILE", "XDG_CONFIG_HOME", "CBM_CACHE_DIR"]);

        let long_home = "a".repeat(300);
        std::env::set_var("HOME", &long_home);
        std::env::remove_var("USERPROFILE");
        std::env::remove_var("XDG_CONFIG_HOME");
        std::env::remove_var("CBM_CACHE_DIR");

        assert_eq!(get_home_dir_bytes().unwrap().len(), ENV_TMP_VALUE_LIMIT);
        #[cfg(windows)]
        assert_eq!(
            app_config_dir_bytes().unwrap().len(),
            ENV_TMP_VALUE_LIMIT + "/AppData/Roaming".len()
        );
        #[cfg(not(windows))]
        assert_eq!(
            app_config_dir_bytes().unwrap().len(),
            ENV_TMP_VALUE_LIMIT + "/.config".len()
        );

        let long_cache = "b".repeat(300);
        std::env::set_var("CBM_CACHE_DIR", &long_cache);
        assert_eq!(
            resolve_cache_dir_bytes().unwrap().len(),
            ENV_TMP_VALUE_LIMIT
        );
    }

    #[test]
    fn cgroup_v2_cpu_and_memory_match_c_policy() {
        let root = temp_root("v2");
        fs::write(root.join("cpu.max"), "150000 100000\n").unwrap();
        fs::write(root.join("memory.max"), "2147483648\n").unwrap();

        assert_eq!(detect_cgroup_cpus(&root), 2);
        assert_eq!(detect_cgroup_mem(&root), 2_147_483_648);

        fs::remove_dir_all(root).unwrap();
    }

    #[test]
    fn cgroup_unlimited_values_match_c_policy() {
        let root = temp_root("unlimited");
        fs::write(root.join("cpu.max"), "max 100000\n").unwrap();
        fs::write(root.join("memory.max"), "max\n").unwrap();

        assert_eq!(detect_cgroup_cpus(&root), -1);
        assert_eq!(detect_cgroup_mem(&root), 0);

        fs::remove_dir_all(root).unwrap();
    }

    #[test]
    fn cgroup_v1_cpu_and_memory_match_c_policy() {
        let root = temp_root("v1");
        fs::create_dir(root.join("cpu")).unwrap();
        fs::create_dir(root.join("memory")).unwrap();
        fs::write(root.join("cpu/cpu.cfs_quota_us"), "200000").unwrap();
        fs::write(root.join("cpu/cpu.cfs_period_us"), "100000").unwrap();
        fs::write(root.join("memory/memory.limit_in_bytes"), "1073741824").unwrap();

        assert_eq!(detect_cgroup_cpus(&root), 2);
        assert_eq!(detect_cgroup_mem(&root), 1_073_741_824);

        fs::remove_dir_all(root).unwrap();
    }

    fn temp_root(name: &str) -> std::path::PathBuf {
        let nonce = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos();
        let root = std::env::temp_dir().join(format!("cbm-rs-cgroup-{name}-{nonce}"));
        fs::create_dir(&root).unwrap();
        root
    }

    struct EnvSnapshot {
        values: Vec<(&'static str, Option<OsString>)>,
    }

    impl EnvSnapshot {
        fn new(names: &[&'static str]) -> Self {
            let values = names
                .iter()
                .map(|name| (*name, std::env::var_os(name)))
                .collect();
            Self { values }
        }
    }

    impl Drop for EnvSnapshot {
        fn drop(&mut self) {
            for (name, value) in &self.values {
                match value {
                    Some(value) => std::env::set_var(name, value),
                    None => std::env::remove_var(name),
                }
            }
        }
    }
}
