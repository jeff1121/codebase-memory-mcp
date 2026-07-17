//! C language enum 到顯示名稱的共用純 mapping。

use core::ffi::{c_char, c_int};
use std::ffi::CStr;

pub const LANGUAGE_COUNT: c_int = 159;

#[must_use]
pub fn language_name(language: c_int) -> &'static CStr {
    if !(0..LANGUAGE_COUNT).contains(&language) {
        return c"Unknown";
    }

    match language {
        0 => c"Go",
        1 => c"Python",
        2 => c"JavaScript",
        3 => c"TypeScript",
        4 => c"TSX",
        5 => c"Rust",
        6 => c"Java",
        7 => c"C++",
        8 => c"C#",
        9 => c"PHP",
        10 => c"Lua",
        11 => c"Scala",
        12 => c"Kotlin",
        13 => c"Ruby",
        14 => c"C",
        15 => c"Bash",
        16 => c"Zig",
        17 => c"Elixir",
        18 => c"Haskell",
        19 => c"OCaml",
        20 => c"Objective-C",
        21 => c"Swift",
        22 => c"Dart",
        23 => c"Perl",
        24 => c"Groovy",
        25 => c"Erlang",
        26 => c"R",
        27 => c"HTML",
        28 => c"CSS",
        29 => c"SCSS",
        30 => c"YAML",
        31 => c"TOML",
        32 => c"HCL",
        33 => c"SQL",
        34 => c"Dockerfile",
        35 => c"Clojure",
        36 => c"F#",
        37 => c"Julia",
        38 => c"VimScript",
        39 => c"Nix",
        40 => c"Common Lisp",
        41 => c"Elm",
        42 => c"Fortran",
        43 => c"CUDA",
        44 => c"COBOL",
        45 => c"Verilog",
        46 => c"Emacs Lisp",
        47 => c"JSON",
        48 => c"XML",
        49 => c"Markdown",
        50 => c"Makefile",
        51 => c"CMake",
        52 => c"Protobuf",
        53 => c"GraphQL",
        54 => c"Vue",
        55 => c"Svelte",
        56 => c"Meson",
        57 => c"GLSL",
        58 => c"INI",
        59 => c"MATLAB",
        60 => c"Lean",
        61 => c"FORM",
        62 => c"Magma",
        63 => c"Wolfram",
        64 => c"Solidity",
        65 => c"Typst",
        66 => c"GDScript",
        67 => c"Gleam",
        68 => c"PowerShell",
        69 => c"Pascal",
        70 => c"D",
        71 => c"Nim",
        72 => c"Scheme",
        73 => c"Fennel",
        74 => c"Fish",
        75 => c"AWK",
        76 => c"Zsh",
        77 => c"Tcl",
        78 => c"Ada",
        79 => c"Agda",
        80 => c"Racket",
        81 => c"Odin",
        82 => c"ReScript",
        83 => c"PureScript",
        84 => c"Nickel",
        85 => c"Crystal",
        86 => c"Teal",
        87 => c"Hare",
        88 => c"Pony",
        89 => c"Luau",
        90 => c"Janet",
        91 => c"Sway",
        92 => c"NASM",
        93 => c"Assembly",
        94 => c"Astro",
        95 => c"Blade",
        96 => c"Just",
        97 => c"Go Template",
        98 => c"Templ",
        99 => c"Liquid",
        100 => c"Jinja2",
        101 => c"Prisma",
        102 => c"Hyprlang",
        103 => c"DotEnv",
        104 => c"Diff",
        105 => c"WGSL",
        106 => c"KDL",
        107 => c"JSON5",
        108 => c"Jsonnet",
        109 => c"RON",
        110 => c"Thrift",
        111 => c"Cap'n Proto",
        112 => c"Properties",
        113 => c"SSH Config",
        114 => c"BibTeX",
        115 => c"Starlark",
        116 => c"Bicep",
        117 => c"CSV",
        118 => c"Requirements",
        119 => c"HLSL",
        120 => c"VHDL",
        121 => c"SystemVerilog",
        122 => c"DeviceTree",
        123 => c"Linker Script",
        124 => c"GN",
        125 => c"Kconfig",
        126 => c"BitBake",
        127 => c"Smali",
        128 => c"TableGen",
        129 => c"ISPC",
        130 => c"Cairo",
        131 => c"Move",
        132 => c"Squirrel",
        133 => c"FunC",
        134 => c"Regex",
        135 => c"JSDoc",
        136 => c"reStructuredText",
        137 => c"Beancount",
        138 => c"Mermaid",
        139 => c"Puppet",
        140 => c"PO",
        141 => c"gitattributes",
        142 => c"gitignore",
        143 => c"Slang",
        144 => c"LLVM IR",
        145 => c"Smithy",
        146 => c"WIT",
        147 => c"TLA+",
        148 => c"Pkl",
        149 => c"Go Mod",
        150 => c"Apex",
        151 => c"SOQL",
        152 => c"SOSL",
        153 => c"Kustomize",
        154 => c"Kubernetes",
        155 => c"PineScript",
        156 => c"QML",
        157 | 158 => c"CFML",
        _ => c"Unknown",
    }
}

#[must_use]
pub fn language_name_ptr(language: c_int) -> *const c_char {
    language_name(language).as_ptr()
}

#[cfg(test)]
mod tests {
    use super::{language_name, LANGUAGE_COUNT};

    #[test]
    fn maps_names_by_content_at_boundaries() {
        assert_eq!(language_name(-1).to_bytes(), b"Unknown");
        assert_eq!(language_name(0).to_bytes(), b"Go");
        assert_eq!(language_name(LANGUAGE_COUNT - 1).to_bytes(), b"CFML");
        assert_eq!(language_name(LANGUAGE_COUNT).to_bytes(), b"Unknown");
    }

    #[test]
    fn maps_every_valid_language_by_content() {
        for language in 0..LANGUAGE_COUNT {
            assert_ne!(
                language_name(language).to_bytes(),
                b"Unknown",
                "language {language}"
            );
        }
    }

    #[test]
    fn preserves_cfml_duplicate_by_content() {
        assert_eq!(language_name(157).to_bytes(), b"CFML");
        assert_eq!(language_name(158).to_bytes(), b"CFML");
    }
}
