#![allow(unsafe_code)]

use crate::language_name::LANGUAGE_COUNT;
use core::ffi::{c_char, c_int, c_void};
use std::ffi::CStr;

const LANG_OBJC: c_int = 20;
const LANG_MATLAB: c_int = 59;
const LANG_MAGMA: c_int = 62;
const LANG_BLADE: c_int = 95;
const LANG_DOTENV: c_int = 103;

#[cfg(not(test))]
unsafe extern "C" {
    fn cbm_rs_user_language_lookup(extension: *const c_char) -> c_int;
}

unsafe extern "C" {
    fn fopen(path: *const c_char, mode: *const c_char) -> *mut c_void;
    fn fread(buffer: *mut c_void, size: usize, count: usize, stream: *mut c_void) -> usize;
    fn fclose(stream: *mut c_void) -> c_int;
}

fn user_language(extension: *const c_char) -> c_int {
    #[cfg(test)]
    {
        let _ = extension;
        LANGUAGE_COUNT
    }
    #[cfg(not(test))]
    unsafe {
        cbm_rs_user_language_lookup(extension)
    }
}

unsafe fn c_bytes<'a>(value: *const c_char) -> &'a [u8] {
    if value.is_null() {
        &[]
    } else {
        CStr::from_ptr(value).to_bytes()
    }
}

fn contains(bytes: &[u8], needle: &[u8]) -> bool {
    bytes.windows(needle.len()).any(|window| window == needle)
}

fn has_objc_markers(bytes: &[u8]) -> bool {
    [
        b"@interface".as_slice(),
        b"@implementation".as_slice(),
        b"@protocol".as_slice(),
        b"@property".as_slice(),
        b"#import".as_slice(),
        b"@selector".as_slice(),
        b"@encode".as_slice(),
        b"@synthesize".as_slice(),
        b"@dynamic".as_slice(),
    ]
    .iter()
    .any(|needle| contains(bytes, needle))
}

fn has_magma_end_markers(bytes: &[u8]) -> bool {
    [
        b"end function;".as_slice(),
        b"end procedure;".as_slice(),
        b"end intrinsic;".as_slice(),
        b"end if;".as_slice(),
        b"end for;".as_slice(),
        b"end while;".as_slice(),
    ]
    .iter()
    .any(|needle| contains(bytes, needle))
}

fn has_magma_callable_pattern(bytes: &[u8]) -> bool {
    for marker in [b"intrinsic ".as_slice(), b"procedure ".as_slice()] {
        let Some(start) = bytes
            .windows(marker.len())
            .position(|window| window == marker)
        else {
            continue;
        };
        let mut index = start + marker.len();
        while index < bytes.len() && bytes[index].is_ascii_alphabetic() {
            index += 1;
        }
        if index < bytes.len() && bytes[index] == b'(' {
            return true;
        }
    }
    false
}

fn has_matlab_line_markers(bytes: &[u8]) -> bool {
    let mut line_start = 0;
    while line_start < bytes.len() {
        let mut index = line_start;
        while index < bytes.len() && (bytes[index] == b' ' || bytes[index] == b'\t') {
            index += 1;
        }
        let line = &bytes[index..];
        if line.starts_with(b"function ")
            || line.starts_with(b"function\t")
            || line.starts_with(b"classdef ")
            || line.starts_with(b"classdef\t")
            || line.starts_with(b"%%")
            || (line.first() == Some(&b'%') && line.get(1) != Some(&b'{'))
        {
            return true;
        }
        let Some(newline) = bytes[line_start..].iter().position(|byte| *byte == b'\n') else {
            break;
        };
        line_start += newline + 1;
    }
    false
}

static EXT_TABLE: &[(&str, c_int)] = &[
    (".bash", 15),
    (".sh", 15),
    (".c", 14),
    (".cc", 7),
    (".ccm", 7),
    (".cpp", 7),
    (".cppm", 7),
    (".cxx", 7),
    (".h", 7),
    (".hh", 7),
    (".hpp", 7),
    (".hxx", 7),
    (".ixx", 7),
    (".cs", 8),
    (".clj", 35),
    (".cljc", 35),
    (".cljs", 35),
    (".cmake", 51),
    (".cbl", 44),
    (".cob", 44),
    (".cl", 40),
    (".lisp", 40),
    (".lsp", 40),
    (".css", 28),
    (".cu", 43),
    (".cuh", 43),
    (".dart", 22),
    (".dockerfile", 34),
    (".ex", 17),
    (".exs", 17),
    (".env", 103),
    (".elm", 41),
    (".el", 46),
    (".erl", 25),
    (".fs", 36),
    (".fsi", 36),
    (".fsx", 36),
    (".frm", 61),
    (".prc", 61),
    (".f03", 42),
    (".f08", 42),
    (".f90", 42),
    (".f95", 42),
    (".frag", 57),
    (".glsl", 57),
    (".vert", 57),
    (".go", 0),
    (".gql", 53),
    (".graphql", 53),
    (".gradle", 24),
    (".groovy", 24),
    (".hs", 18),
    (".hcl", 32),
    (".tf", 32),
    (".htm", 27),
    (".html", 27),
    (".cfg", 58),
    (".conf", 58),
    (".ini", 58),
    (".java", 6),
    (".js", 2),
    (".jsx", 2),
    (".mjs", 2),
    (".cjs", 2),
    (".json", 47),
    (".jl", 37),
    (".kt", 12),
    (".kts", 12),
    (".lean", 60),
    (".lua", 10),
    (".mag", 62),
    (".magma", 62),
    (".mk", 50),
    (".md", 49),
    (".mdx", 49),
    (".m", 59),
    (".matlab", 59),
    (".mlx", 59),
    (".meson", 56),
    (".nix", 39),
    (".ml", 19),
    (".mli", 19),
    (".pl", 23),
    (".pm", 23),
    (".php", 9),
    (".proto", 52),
    (".py", 1),
    (".R", 26),
    (".r", 26),
    (".gemspec", 13),
    (".rake", 13),
    (".rb", 13),
    (".rs", 5),
    (".sc", 11),
    (".scala", 11),
    (".scss", 29),
    (".sql", 33),
    (".svelte", 55),
    (".swift", 21),
    (".sv", 45),
    (".v", 45),
    (".toml", 31),
    (".tsx", 4),
    (".ts", 3),
    (".mts", 3),
    (".cts", 3),
    (".vim", 38),
    (".vimrc", 38),
    ("justfile", 96),
    ("Justfile", 96),
    (".justfile", 96),
    (".just", 96),
    ("hyprland.conf", 102),
    ("ssh_config", 113),
    ("sshd_config", 113),
    ("BUILD", 115),
    ("BUILD.bazel", 115),
    ("WORKSPACE", 115),
    ("WORKSPACE.bazel", 115),
    (".inc", 126),
    (".vue", 54),
    (".wl", 63),
    (".wls", 63),
    (".xml", 48),
    (".xsd", 48),
    (".xsl", 48),
    (".svg", 48),
    (".yaml", 30),
    (".yml", 30),
    (".adb", 78),
    (".ads", 78),
    (".agda", 79),
    (".astro", 94),
    (".awk", 75),
    (".bb", 126),
    (".bbappend", 126),
    (".bbclass", 126),
    (".beancount", 137),
    (".bib", 114),
    (".bicep", 116),
    (".bzl", 115),
    (".cairo", 130),
    (".capnp", 111),
    (".cls", 150),
    (".cr", 85),
    (".csv", 117),
    (".d", 70),
    (".diff", 104),
    (".dpr", 69),
    (".dts", 122),
    (".dtsi", 122),
    (".fc", 133),
    (".fish", 74),
    (".fnl", 73),
    (".fx", 119),
    (".gd", 66),
    (".gleam", 67),
    (".gn", 124),
    (".gni", 124),
    (".gotmpl", 97),
    (".tpl", 97),
    (".ha", 87),
    (".hl", 102),
    (".hlsl", 119),
    (".hlsli", 119),
    (".ispc", 129),
    (".j2", 100),
    (".janet", 90),
    (".jinja", 100),
    (".jinja2", 100),
    (".json5", 107),
    (".jsonnet", 108),
    (".kdl", 106),
    (".ld", 123),
    (".lds", 123),
    (".libsonnet", 108),
    (".liquid", 99),
    (".ll", 144),
    (".lpr", 69),
    (".luau", 89),
    (".qml", 156),
    (".cfc", 157),
    (".cfm", 158),
    (".mermaid", 138),
    (".mmd", 138),
    (".move", 131),
    (".nasm", 92),
    (".ncl", 84),
    (".nut", 132),
    (".odin", 81),
    (".overlay", 122),
    (".pas", 69),
    (".patch", 104),
    (".pine", 155),
    (".pkl", 148),
    (".po", 140),
    (".pony", 88),
    (".pot", 140),
    (".pp", 139),
    (".prisma", 101),
    (".properties", 112),
    (".ps1", 68),
    (".psd1", 68),
    (".psm1", 68),
    (".purs", 83),
    (".res", 82),
    (".resi", 82),
    (".re", 134),
    (".rkt", 80),
    (".ron", 109),
    (".rst", 136),
    (".s", 93),
    (".S", 93),
    (".scm", 72),
    (".slang", 143),
    (".smali", 127),
    (".smithy", 145),
    (".sol", 64),
    (".soql", 151),
    (".sosl", 152),
    (".ss", 72),
    (".star", 115),
    (".sw", 91),
    (".tcl", 77),
    (".td", 128),
    (".templ", 98),
    (".thrift", 110),
    (".tl", 86),
    (".tla", 147),
    (".tmpl", 97),
    (".trigger", 150),
    (".typ", 65),
    (".vhd", 120),
    (".vhdl", 120),
    (".wgsl", 105),
    (".wit", 146),
    (".zsh", 76),
    (".zig", 16),
];

static FILENAME_TABLE: &[(&str, c_int)] = &[
    ("CMakeLists.txt", 51),
    ("Dockerfile", 34),
    ("GNUmakefile", 50),
    ("Makefile", 50),
    ("makefile", 50),
    ("meson.build", 56),
    ("meson.options", 56),
    ("meson_options.txt", 56),
    ("kustomization.yaml", 153),
    ("kustomization.yml", 153),
    (".vimrc", 38),
    (".zshrc", 76),
    (".zshenv", 76),
    (".zprofile", 76),
    ("justfile", 96),
    ("Justfile", 96),
    (".justfile", 96),
    ("hyprland.conf", 102),
    ("ssh_config", 113),
    ("sshd_config", 113),
    (".ssh/config", 113),
    ("BUILD", 115),
    ("BUILD.bazel", 115),
    ("WORKSPACE", 115),
    ("WORKSPACE.bazel", 115),
    ("requirements.txt", 118),
    ("requirements-dev.txt", 118),
    ("requirements-test.txt", 118),
    ("Kconfig", 125),
    ("go.mod", 149),
    (".env", 103),
    (".env.local", 103),
    (".gitattributes", 141),
];

#[no_mangle]
/// 依副檔名回傳對應的 C 語言 enum 值。
///
/// # Safety
/// `extension` 必須是可讀取且以 NUL 結尾的 C 字串，或為 null。
pub unsafe extern "C" fn cbm_language_for_extension(extension: *const c_char) -> c_int {
    if extension.is_null() {
        return LANGUAGE_COUNT;
    }
    let bytes = unsafe { c_bytes(extension) };
    if bytes.is_empty() {
        return LANGUAGE_COUNT;
    }
    let user_language = user_language(extension);
    if user_language != LANGUAGE_COUNT {
        return user_language;
    }
    for &(name, language) in EXT_TABLE {
        if bytes == name.as_bytes() {
            return language;
        }
    }
    LANGUAGE_COUNT
}

#[no_mangle]
/// 依檔名回傳對應的 C 語言 enum 值。
///
/// # Safety
/// `filename` 必須是可讀取且以 NUL 結尾的 C 字串，或為 null。
pub unsafe extern "C" fn cbm_language_for_filename(filename: *const c_char) -> c_int {
    if filename.is_null() {
        return LANGUAGE_COUNT;
    }
    let bytes = unsafe { c_bytes(filename) };
    if bytes.is_empty() {
        return LANGUAGE_COUNT;
    }
    for &(name, language) in FILENAME_TABLE {
        if bytes == name.as_bytes() {
            return language;
        }
    }
    if bytes.starts_with(b".env.") {
        return LANG_DOTENV;
    }
    let Some(last_dot) = bytes.iter().rposition(|byte| *byte == b'.') else {
        return LANGUAGE_COUNT;
    };
    let Some(mut dot) = bytes.iter().position(|byte| *byte == b'.') else {
        return LANGUAGE_COUNT;
    };
    while dot < last_dot {
        if bytes[dot..] == *b".blade.php" {
            return LANG_BLADE;
        }
        let user_language = user_language(unsafe { filename.add(dot) });
        if user_language != LANGUAGE_COUNT {
            return user_language;
        }
        dot += 1;
        while dot < last_dot && bytes[dot] != b'.' {
            dot += 1;
        }
    }
    cbm_language_for_extension(unsafe { filename.add(last_dot) })
}

#[no_mangle]
/// 讀取 `.m` 檔案前 4 KiB 並判斷 Objective-C、Magma 或 MATLAB。
///
/// # Safety
/// `path` 必須是可讀取且以 NUL 結尾的 C 字串，或為 null。
pub unsafe extern "C" fn cbm_disambiguate_m(path: *const c_char) -> c_int {
    if path.is_null() {
        return LANG_MATLAB;
    }
    let file = unsafe { fopen(path, c"r".as_ptr()) };
    if file.is_null() {
        return LANG_MATLAB;
    }
    let mut buffer = [0_u8; 4097];
    let length = unsafe { fread(buffer.as_mut_ptr().cast::<c_void>(), 1, 4096, file) };
    unsafe {
        fclose(file);
    }
    let length = buffer[..length]
        .iter()
        .position(|byte| *byte == 0)
        .unwrap_or(length);
    let bytes = &buffer[..length];
    if has_objc_markers(bytes) {
        return LANG_OBJC;
    }
    if has_magma_end_markers(bytes) {
        return LANG_MAGMA;
    }
    if (contains(bytes, b"intrinsic ") || contains(bytes, b"procedure "))
        && has_magma_callable_pattern(bytes)
    {
        return LANG_MAGMA;
    }
    if has_matlab_line_markers(bytes) {
        return LANG_MATLAB;
    }
    LANG_MATLAB
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn maps_extensions_without_user_config() {
        unsafe {
            assert_eq!(cbm_language_for_extension(c".rs".as_ptr()), 5);
            assert_eq!(cbm_language_for_extension(c".tsx".as_ptr()), 4);
            assert_eq!(cbm_language_for_extension(c".txt".as_ptr()), LANGUAGE_COUNT);
            assert_eq!(
                cbm_language_for_extension(core::ptr::null()),
                LANGUAGE_COUNT
            );
        }
    }

    #[test]
    fn maps_special_filenames() {
        unsafe {
            assert_eq!(cbm_language_for_filename(c"Makefile".as_ptr()), 50);
            assert_eq!(
                cbm_language_for_filename(c".env.local".as_ptr()),
                LANG_DOTENV
            );
        }
    }
}
