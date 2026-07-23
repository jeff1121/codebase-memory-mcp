#include "store/language_map.h"

#include <stddef.h>
#include <string.h>

#ifdef CBM_USE_RUST_STORE_LANGUAGE_MAP
extern const char *cbm_rs_store_ext_to_lang_v1(const char *ext);
#endif

typedef struct {
    const char *ext;
    const char *lang;
} ext_lang_entry_t;

static const ext_lang_entry_t ext_lang_table[] = {
    {".py", "Python"},     {".go", "Go"},          {".js", "JavaScript"}, {".jsx", "JavaScript"},
    {".ts", "TypeScript"}, {".tsx", "TypeScript"}, {".rs", "Rust"},       {".java", "Java"},
    {".cpp", "C++"},       {".cc", "C++"},         {".cxx", "C++"},       {".c", "C"},
    {".h", "C"},           {".cs", "C#"},          {".php", "PHP"},       {".lua", "Lua"},
    {".scala", "Scala"},   {".kt", "Kotlin"},      {".rb", "Ruby"},       {".sh", "Bash"},
    {".bash", "Bash"},     {".zig", "Zig"},        {".ex", "Elixir"},     {".exs", "Elixir"},
    {".hs", "Haskell"},    {".ml", "OCaml"},       {".mli", "OCaml"},     {".html", "HTML"},
    {".css", "CSS"},       {".yaml", "YAML"},      {".yml", "YAML"},      {".toml", "TOML"},
    {".hcl", "HCL"},       {".tf", "HCL"},         {".sql", "SQL"},       {".erl", "Erlang"},
    {".swift", "Swift"},   {".dart", "Dart"},      {".groovy", "Groovy"}, {".pl", "Perl"},
    {".r", "R"},           {".scss", "SCSS"},      {".vue", "Vue"},       {".svelte", "Svelte"},
    {NULL, NULL},
};

static const char *ext_to_lang_c(const char *ext) {
    if (!ext) {
        return NULL;
    }
    for (const ext_lang_entry_t *entry = ext_lang_table; entry->ext; entry++) {
        if (strcmp(ext, entry->ext) == 0) {
            return entry->lang;
        }
    }
    return NULL;
}

const char *cbm_store_ext_to_lang(const char *ext) {
#ifdef CBM_USE_RUST_STORE_LANGUAGE_MAP
    const char *lang = cbm_rs_store_ext_to_lang_v1(ext);
    if (lang) {
        return lang;
    }
#endif
    return ext_to_lang_c(ext);
}
