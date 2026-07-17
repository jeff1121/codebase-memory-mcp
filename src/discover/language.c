/*
 * language.c — Language detection from filename and extension.
 *
 * Maps file extensions and special filenames to CBMLanguage enum values.
 * Handles .m disambiguation (Objective-C vs Magma vs MATLAB).
 * Consults the process-global user config (set via cbm_set_user_lang_config)
 * before the built-in lookup table.
 */
#include "discover/discover.h"
#include "discover/language_markers.h"
#include "discover/userconfig.h"
#include "cbm.h" // CBMLanguage, CBM_LANG_*

#include "foundation/constants.h"

#define SLEN(s) (sizeof(s) - 1)
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifndef CBM_USE_RUST_DISCOVER_LANGUAGE_ONLY

/* ── Extension → Language lookup table ───────────────────────────── */

typedef struct {
    const char *ext; /* including dot, e.g. ".go" */
    CBMLanguage language;
} ext_entry_t;

/* Sorted by extension for binary search (but linear scan is fine for ~120 entries) */
static const ext_entry_t EXT_TABLE[] = {
    /* Bash */
    {".bash", CBM_LANG_BASH},
    {".sh", CBM_LANG_BASH},

    /* C */
    {".c", CBM_LANG_C},

    /* C++ */
    {".cc", CBM_LANG_CPP},
    {".ccm", CBM_LANG_CPP},
    {".cpp", CBM_LANG_CPP},
    {".cppm", CBM_LANG_CPP},
    {".cxx", CBM_LANG_CPP},
    {".h", CBM_LANG_CPP},
    {".hh", CBM_LANG_CPP},
    {".hpp", CBM_LANG_CPP},
    {".hxx", CBM_LANG_CPP},
    {".ixx", CBM_LANG_CPP},

    /* C# */
    {".cs", CBM_LANG_CSHARP},

    /* Clojure */
    {".clj", CBM_LANG_CLOJURE},
    {".cljc", CBM_LANG_CLOJURE},
    {".cljs", CBM_LANG_CLOJURE},

    /* CMake */
    {".cmake", CBM_LANG_CMAKE},

    /* COBOL */
    {".cbl", CBM_LANG_COBOL},
    {".cob", CBM_LANG_COBOL},

    /* Common Lisp */
    {".cl", CBM_LANG_COMMONLISP},
    {".lisp", CBM_LANG_COMMONLISP},
    {".lsp", CBM_LANG_COMMONLISP},

    /* CSS */
    {".css", CBM_LANG_CSS},

    /* CUDA */
    {".cu", CBM_LANG_CUDA},
    {".cuh", CBM_LANG_CUDA},

    /* Dart */
    {".dart", CBM_LANG_DART},

    /* Dockerfile */
    {".dockerfile", CBM_LANG_DOCKERFILE},

    /* Elixir */
    {".ex", CBM_LANG_ELIXIR},
    {".exs", CBM_LANG_ELIXIR},

    /* DotEnv */
    {".env", CBM_LANG_DOTENV},

    /* Elm */
    {".elm", CBM_LANG_ELM},

    /* Emacs Lisp */
    {".el", CBM_LANG_EMACSLISP},

    /* Erlang */
    {".erl", CBM_LANG_ERLANG},

    /* F# */
    {".fs", CBM_LANG_FSHARP},
    {".fsi", CBM_LANG_FSHARP},
    {".fsx", CBM_LANG_FSHARP},

    /* FORM */
    {".frm", CBM_LANG_FORM},
    {".prc", CBM_LANG_FORM},

    /* Fortran */
    {".f03", CBM_LANG_FORTRAN},
    {".f08", CBM_LANG_FORTRAN},
    {".f90", CBM_LANG_FORTRAN},
    {".f95", CBM_LANG_FORTRAN},

    /* GLSL */
    {".frag", CBM_LANG_GLSL},
    {".glsl", CBM_LANG_GLSL},
    {".vert", CBM_LANG_GLSL},

    /* Go */
    {".go", CBM_LANG_GO},

    /* GraphQL */
    {".gql", CBM_LANG_GRAPHQL},
    {".graphql", CBM_LANG_GRAPHQL},

    /* Groovy */
    {".gradle", CBM_LANG_GROOVY},
    {".groovy", CBM_LANG_GROOVY},

    /* Haskell */
    {".hs", CBM_LANG_HASKELL},

    /* HCL / Terraform */
    {".hcl", CBM_LANG_HCL},
    {".tf", CBM_LANG_HCL},

    /* HTML */
    {".htm", CBM_LANG_HTML},
    {".html", CBM_LANG_HTML},

    /* INI */
    {".cfg", CBM_LANG_INI},
    {".conf", CBM_LANG_INI},
    {".ini", CBM_LANG_INI},

    /* Java */
    {".java", CBM_LANG_JAVA},

    /* JavaScript */
    {".js", CBM_LANG_JAVASCRIPT},
    {".jsx", CBM_LANG_JAVASCRIPT},
    {".mjs", CBM_LANG_JAVASCRIPT}, /* ES modules (#197) */
    {".cjs", CBM_LANG_JAVASCRIPT}, /* CommonJS modules */

    /* JSON */
    {".json", CBM_LANG_JSON},

    /* Julia */
    {".jl", CBM_LANG_JULIA},

    /* Kotlin */
    {".kt", CBM_LANG_KOTLIN},
    {".kts", CBM_LANG_KOTLIN},

    /* Lean */
    {".lean", CBM_LANG_LEAN},

    /* Lua */
    {".lua", CBM_LANG_LUA},

    /* Magma */
    {".mag", CBM_LANG_MAGMA},
    {".magma", CBM_LANG_MAGMA},

    /* Makefile */
    {".mk", CBM_LANG_MAKEFILE},

    /* Markdown */
    {".md", CBM_LANG_MARKDOWN},
    {".mdx", CBM_LANG_MARKDOWN},

    /* MATLAB */
    {".m", CBM_LANG_MATLAB},
    {".matlab", CBM_LANG_MATLAB},
    {".mlx", CBM_LANG_MATLAB},

    /* Meson */
    {".meson", CBM_LANG_MESON},

    /* Nix */
    {".nix", CBM_LANG_NIX},

    /* OCaml */
    {".ml", CBM_LANG_OCAML},
    {".mli", CBM_LANG_OCAML},

    /* Perl */
    {".pl", CBM_LANG_PERL},
    {".pm", CBM_LANG_PERL},

    /* PHP */
    {".php", CBM_LANG_PHP},

    /* Protobuf */
    {".proto", CBM_LANG_PROTOBUF},

    /* Python */
    {".py", CBM_LANG_PYTHON},

    /* R — case insensitive handled separately */
    {".R", CBM_LANG_R},
    {".r", CBM_LANG_R},

    /* Ruby */
    {".gemspec", CBM_LANG_RUBY},
    {".rake", CBM_LANG_RUBY},
    {".rb", CBM_LANG_RUBY},

    /* Rust */
    {".rs", CBM_LANG_RUST},

    /* Scala */
    {".sc", CBM_LANG_SCALA},
    {".scala", CBM_LANG_SCALA},

    /* SCSS */
    {".scss", CBM_LANG_SCSS},

    /* SQL */
    {".sql", CBM_LANG_SQL},

    /* Svelte */
    {".svelte", CBM_LANG_SVELTE},

    /* Swift */
    {".swift", CBM_LANG_SWIFT},

    /* SystemVerilog + Verilog */
    {".sv", CBM_LANG_VERILOG},
    {".v", CBM_LANG_VERILOG},

    /* TOML */
    {".toml", CBM_LANG_TOML},

    /* TSX */
    {".tsx", CBM_LANG_TSX},

    /* TypeScript */
    {".ts", CBM_LANG_TYPESCRIPT},
    {".mts", CBM_LANG_TYPESCRIPT}, /* TS ES modules */
    {".cts", CBM_LANG_TYPESCRIPT}, /* TS CommonJS modules */

    /* VimScript */
    {".vim", CBM_LANG_VIMSCRIPT},
    {".vimrc", CBM_LANG_VIMSCRIPT},
    {"justfile", CBM_LANG_JUST},
    {"Justfile", CBM_LANG_JUST},
    {".justfile", CBM_LANG_JUST},
    {".just", CBM_LANG_JUST}, /* `import 'common.just'` target files */
    {"hyprland.conf", CBM_LANG_HYPRLANG},
    {"ssh_config", CBM_LANG_SSHCONFIG},
    {"sshd_config", CBM_LANG_SSHCONFIG},
    {"BUILD", CBM_LANG_STARLARK},
    {"BUILD.bazel", CBM_LANG_STARLARK},
    {"WORKSPACE", CBM_LANG_STARLARK},
    {"WORKSPACE.bazel", CBM_LANG_STARLARK},

    /* BitBake include fragments — `require/include foo.inc` target files. */
    {".inc", CBM_LANG_BITBAKE},

    /* Vue */
    {".vue", CBM_LANG_VUE},

    /* Wolfram */
    {".wl", CBM_LANG_WOLFRAM},
    {".wls", CBM_LANG_WOLFRAM},

    /* XML */
    {".xml", CBM_LANG_XML},
    {".xsd", CBM_LANG_XML},
    {".xsl", CBM_LANG_XML},
    {".svg", CBM_LANG_XML},

    /* YAML */
    {".yaml", CBM_LANG_YAML},
    {".yml", CBM_LANG_YAML},

    /* Ada */
    {".adb", CBM_LANG_ADA},

    /* Ada */
    {".ads", CBM_LANG_ADA},

    /* Agda */
    {".agda", CBM_LANG_AGDA},

    /* Astro */
    {".astro", CBM_LANG_ASTRO},

    /* AWK */
    {".awk", CBM_LANG_AWK},

    /* BitBake */
    {".bb", CBM_LANG_BITBAKE},

    /* BitBake */
    {".bbappend", CBM_LANG_BITBAKE},

    /* BitBake */
    {".bbclass", CBM_LANG_BITBAKE},

    /* Beancount */
    {".beancount", CBM_LANG_BEANCOUNT},

    /* BibTeX */
    {".bib", CBM_LANG_BIBTEX},

    /* Bicep */
    {".bicep", CBM_LANG_BICEP},

    /* Blade */
    /* .blade.php handled by userconfig compound extensions, not EXT_TABLE */

    /* Starlark */
    {".bzl", CBM_LANG_STARLARK},

    /* Cairo */
    {".cairo", CBM_LANG_CAIRO},

    /* Cap'n Proto */
    {".capnp", CBM_LANG_CAPNP},

    /* Apex */
    {".cls", CBM_LANG_APEX},

    /* Crystal */
    {".cr", CBM_LANG_CRYSTAL},

    /* CSV */
    {".csv", CBM_LANG_CSV},

    /* D */
    {".d", CBM_LANG_DLANG},

    /* Diff */
    {".diff", CBM_LANG_DIFF},

    /* Pascal */
    {".dpr", CBM_LANG_PASCAL},

    /* DeviceTree */
    {".dts", CBM_LANG_DEVICETREE},

    /* DeviceTree */
    {".dtsi", CBM_LANG_DEVICETREE},

    /* FunC */
    {".fc", CBM_LANG_FUNC},

    /* Fish */
    {".fish", CBM_LANG_FISH},

    /* Fennel */
    {".fnl", CBM_LANG_FENNEL},

    /* HLSL */
    {".fx", CBM_LANG_HLSL},

    /* GDScript */
    {".gd", CBM_LANG_GDSCRIPT},

    /* Gleam */
    {".gleam", CBM_LANG_GLEAM},

    /* GN */
    {".gn", CBM_LANG_GN},

    /* GN */
    {".gni", CBM_LANG_GN},

    /* Go Template */
    {".gotmpl", CBM_LANG_GOTEMPLATE},
    {".tpl", CBM_LANG_GOTEMPLATE}, /* Helm _helpers.tpl named-template definitions */

    /* Hare */
    {".ha", CBM_LANG_HARE},

    /* Hyprlang */
    {".hl", CBM_LANG_HYPRLANG},

    /* HLSL */
    {".hlsl", CBM_LANG_HLSL},

    /* HLSL */
    {".hlsli", CBM_LANG_HLSL},

    /* ISPC */
    {".ispc", CBM_LANG_ISPC},

    /* Jinja2 */
    {".j2", CBM_LANG_JINJA2},

    /* Janet */
    {".janet", CBM_LANG_JANET},

    /* Jinja2 */
    {".jinja", CBM_LANG_JINJA2},

    /* Jinja2 */
    {".jinja2", CBM_LANG_JINJA2},

    /* JSON5 */
    {".json5", CBM_LANG_JSON5},

    /* Jsonnet */
    {".jsonnet", CBM_LANG_JSONNET},

    /* KDL */
    {".kdl", CBM_LANG_KDL},

    /* Linker Script */
    {".ld", CBM_LANG_LINKERSCRIPT},

    /* Linker Script */
    {".lds", CBM_LANG_LINKERSCRIPT},

    /* Jsonnet */
    {".libsonnet", CBM_LANG_JSONNET},

    /* Liquid */
    {".liquid", CBM_LANG_LIQUID},

    /* LLVM IR */
    {".ll", CBM_LANG_LLVM_IR},

    /* Pascal */
    {".lpr", CBM_LANG_PASCAL},

    /* Luau */
    {".luau", CBM_LANG_LUAU},

    /* Qt QML */
    {".qml", CBM_LANG_QML},

    /* CFML / ColdFusion — .cfc components are script-dialect; .cfm are tag templates */
    {".cfc", CBM_LANG_CFSCRIPT},
    {".cfm", CBM_LANG_CFML},

    /* Mermaid */
    {".mermaid", CBM_LANG_MERMAID},

    /* Mermaid */
    {".mmd", CBM_LANG_MERMAID},

    /* Move */
    {".move", CBM_LANG_MOVE},

    /* NASM */
    {".nasm", CBM_LANG_NASM},

    /* Nickel */
    {".ncl", CBM_LANG_NICKEL},

    /* Nim */

    /* Nim */

    /* Squirrel */
    {".nut", CBM_LANG_SQUIRREL},

    /* Odin */
    {".odin", CBM_LANG_ODIN},

    /* DeviceTree */
    {".overlay", CBM_LANG_DEVICETREE},

    /* Pascal */
    {".pas", CBM_LANG_PASCAL},

    /* Diff */
    {".patch", CBM_LANG_DIFF},

    /* Pine Script */
    {".pine", CBM_LANG_PINE},

    /* Pkl */
    {".pkl", CBM_LANG_PKL},

    /* PO */
    {".po", CBM_LANG_PO},

    /* Pony */
    {".pony", CBM_LANG_PONY},

    /* PO */
    {".pot", CBM_LANG_PO},

    /* Puppet */
    {".pp", CBM_LANG_PUPPET},

    /* Prisma */
    {".prisma", CBM_LANG_PRISMA},

    /* Properties */
    {".properties", CBM_LANG_PROPERTIES},

    /* PowerShell */
    {".ps1", CBM_LANG_POWERSHELL},

    /* PowerShell */
    {".psd1", CBM_LANG_POWERSHELL},

    /* PowerShell */
    {".psm1", CBM_LANG_POWERSHELL},

    /* PureScript */
    {".purs", CBM_LANG_PURESCRIPT},

    /* ReScript */
    {".res", CBM_LANG_RESCRIPT},

    /* ReScript */
    {".resi", CBM_LANG_RESCRIPT},

    /* Regex */
    {".re", CBM_LANG_REGEX},

    /* Racket */
    {".rkt", CBM_LANG_RACKET},

    /* RON */
    {".ron", CBM_LANG_RON},

    /* reStructuredText */
    {".rst", CBM_LANG_RST},

    /* Assembly */
    {".s", CBM_LANG_ASSEMBLY},

    /* Assembly */
    {".S", CBM_LANG_ASSEMBLY},

    /* Scheme */
    {".scm", CBM_LANG_SCHEME},

    /* Slang */
    {".slang", CBM_LANG_SLANG},

    /* Smali */
    {".smali", CBM_LANG_SMALI},

    /* Smithy */
    {".smithy", CBM_LANG_SMITHY},

    /* Solidity */
    {".sol", CBM_LANG_SOLIDITY},

    /* SOQL */
    {".soql", CBM_LANG_SOQL},

    /* SOSL */
    {".sosl", CBM_LANG_SOSL},

    /* Scheme */
    {".ss", CBM_LANG_SCHEME},

    /* Starlark */
    {".star", CBM_LANG_STARLARK},

    /* SystemVerilog */

    /* SystemVerilog */

    /* Sway */
    {".sw", CBM_LANG_SWAY},

    /* Tcl */
    {".tcl", CBM_LANG_TCL},

    /* TableGen */
    {".td", CBM_LANG_TABLEGEN},

    /* Templ */
    {".templ", CBM_LANG_TEMPL},

    /* Thrift */
    {".thrift", CBM_LANG_THRIFT},

    /* Teal */
    {".tl", CBM_LANG_TEAL},

    /* TLA+ */
    {".tla", CBM_LANG_TLAPLUS},

    /* Go Template */
    {".tmpl", CBM_LANG_GOTEMPLATE},

    /* Apex */
    {".trigger", CBM_LANG_APEX},

    /* Typst */
    {".typ", CBM_LANG_TYPST},

    /* VHDL */
    {".vhd", CBM_LANG_VHDL},

    /* VHDL */
    {".vhdl", CBM_LANG_VHDL},

    /* WGSL */
    {".wgsl", CBM_LANG_WGSL},

    /* WIT */
    {".wit", CBM_LANG_WIT},

    /* Zsh */
    {".zsh", CBM_LANG_ZSH},

    /* Zig */
    {".zig", CBM_LANG_ZIG},
};

#define EXT_TABLE_SIZE (sizeof(EXT_TABLE) / sizeof(EXT_TABLE[0]))

/* ── Special filename → Language lookup ──────────────────────────── */

typedef struct {
    const char *filename;
    CBMLanguage language;
} filename_entry_t;

static const filename_entry_t FILENAME_TABLE[] = {
    {"CMakeLists.txt", CBM_LANG_CMAKE},
    {"Dockerfile", CBM_LANG_DOCKERFILE},
    {"GNUmakefile", CBM_LANG_MAKEFILE},
    {"Makefile", CBM_LANG_MAKEFILE},
    {"makefile", CBM_LANG_MAKEFILE},
    {"meson.build", CBM_LANG_MESON},
    {"meson.options", CBM_LANG_MESON},
    {"meson_options.txt", CBM_LANG_MESON},
    {"kustomization.yaml", CBM_LANG_KUSTOMIZE},
    {"kustomization.yml", CBM_LANG_KUSTOMIZE},
    /* Note: FILENAME_TABLE uses case-sensitive strcmp, so mixed-case variants
     * (e.g. "Kustomization.yaml") are not matched here.  They fall through to
     * CBM_LANG_YAML and are re-classified by cbm_is_kustomize_file() in
     * pass_k8s.c, which performs a case-insensitive comparison.  This is the
     * intended behaviour — no additional entries are needed. */
    {".vimrc", CBM_LANG_VIMSCRIPT},
    {".zshrc", CBM_LANG_ZSH},
    {".zshenv", CBM_LANG_ZSH},
    {".zprofile", CBM_LANG_ZSH},
    {"justfile", CBM_LANG_JUST},
    {"Justfile", CBM_LANG_JUST},
    {".justfile", CBM_LANG_JUST},
    {"hyprland.conf", CBM_LANG_HYPRLANG},
    {"ssh_config", CBM_LANG_SSHCONFIG},
    {"sshd_config", CBM_LANG_SSHCONFIG},
    {".ssh/config", CBM_LANG_SSHCONFIG},
    {"BUILD", CBM_LANG_STARLARK},
    {"BUILD.bazel", CBM_LANG_STARLARK},
    {"WORKSPACE", CBM_LANG_STARLARK},
    {"WORKSPACE.bazel", CBM_LANG_STARLARK},
    {"requirements.txt", CBM_LANG_REQUIREMENTS},
    {"requirements-dev.txt", CBM_LANG_REQUIREMENTS},
    {"requirements-test.txt", CBM_LANG_REQUIREMENTS},
    {"Kconfig", CBM_LANG_KCONFIG},
    {"go.mod", CBM_LANG_GOMOD},
    {".env", CBM_LANG_DOTENV},
    {".env.local", CBM_LANG_DOTENV},
    {".gitattributes", CBM_LANG_GITATTRIBUTES},

};

#define FILENAME_TABLE_SIZE (sizeof(FILENAME_TABLE) / sizeof(FILENAME_TABLE[0]))

/* ── Public API ──────────────────────────────────────────────────── */

CBMLanguage cbm_language_for_extension(const char *ext) {
    if (!ext || !ext[0]) {
        return CBM_LANG_COUNT;
    }

    /* Check user-defined overrides first */
    const cbm_userconfig_t *ucfg = cbm_get_user_lang_config();
    if (ucfg) {
        CBMLanguage ulang = cbm_userconfig_lookup(ucfg, ext);
        if (ulang != CBM_LANG_COUNT) {
            return ulang;
        }
    }

    for (size_t i = 0; i < EXT_TABLE_SIZE; i++) {
        if (strcmp(EXT_TABLE[i].ext, ext) == 0) {
            return EXT_TABLE[i].language;
        }
    }
    return CBM_LANG_COUNT;
}

CBMLanguage cbm_language_for_filename(const char *filename) {
    if (!filename || !filename[0]) {
        return CBM_LANG_COUNT;
    }

    /* Check special filenames first */
    for (size_t i = 0; i < FILENAME_TABLE_SIZE; i++) {
        if (strcmp(FILENAME_TABLE[i].filename, filename) == 0) {
            return FILENAME_TABLE[i].language;
        }
    }

    /* DotEnv variant filenames (".env.local", ".env.production", …): the
     * filename starts with ".env." but its last "extension" (e.g. ".local")
     * is not a real language extension.  Match the dotenv convention used by
     * pass_envscan/pass_infrascan (".env" exact, ".env." prefix, "*.env"
     * suffix) so file-index routing agrees with direct extraction. */
    if (strncmp(filename, ".env.", SLEN(".env.")) == 0) {
        return CBM_LANG_DOTENV;
    }

    /* Fall back to extension-based lookup.
     * For compound extensions (e.g. ".blade.php") defined in the user config,
     * scan from the first dot in the basename toward the last, checking user
     * config at each position.  Built-in extensions use the last dot only. */
    const char *last_dot = strrchr(filename, '.');
    if (!last_dot) {
        return CBM_LANG_COUNT;
    }

    /* Probe compound extensions (e.g. ".blade.php") from the first dot toward
     * the last. Built-in compounds are checked first so e.g. Laravel Blade
     * templates map to Blade rather than the single-extension fallback (PHP);
     * user config can still add more (#258). */
    static const struct {
        const char *ext;
        CBMLanguage lang;
    } COMPOUND_EXT_TABLE[] = {
        {".blade.php", CBM_LANG_BLADE},
    };
    const cbm_userconfig_t *ucfg = cbm_get_user_lang_config();
    const char *p = strchr(filename, '.');
    while (p && p < last_dot) {
        for (size_t i = 0; i < sizeof(COMPOUND_EXT_TABLE) / sizeof(COMPOUND_EXT_TABLE[0]); i++) {
            if (strcmp(p, COMPOUND_EXT_TABLE[i].ext) == 0) {
                return COMPOUND_EXT_TABLE[i].lang;
            }
        }
        if (ucfg) {
            CBMLanguage lang = cbm_userconfig_lookup(ucfg, p);
            if (lang != CBM_LANG_COUNT) {
                return lang;
            }
        }
        p = strchr(p + SKIP_ONE, '.');
    }

    /* Standard single-extension lookup (built-ins + user overrides). */
    return cbm_language_for_extension(last_dot);
}

/* ── .m file disambiguation ──────────────────────────────────────── */

CBMLanguage cbm_disambiguate_m(const char *path) {
    if (!path) {
        return CBM_LANG_MATLAB;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        return CBM_LANG_MATLAB;
    }

    /* Read first 4KB */
    char buf[CBM_SZ_4K + SKIP_ONE];
    size_t n = fread(buf, SKIP_ONE, CBM_SZ_4K, f);
    buf[n] = '\0';
    (void)fclose(f);

    if (cbm_discover_language_marker(buf, 0)) {
        return CBM_LANG_OBJC;
    }
    if (cbm_discover_language_marker(buf, 1)) {
        return CBM_LANG_MAGMA;
    }
    if (cbm_discover_language_marker(buf, 2)) {
        return CBM_LANG_MAGMA;
    }
    if (cbm_discover_language_marker(buf, 3)) {
        return CBM_LANG_MATLAB;
    }

    return CBM_LANG_MATLAB;
}

#endif
