/*
 * discover.c — Recursive directory walk with filtering.
 *
 * Walks a repository directory tree, applying:
 *   1. Hardcoded directory skip patterns (60+ dirs like .git, node_modules)
 *   2. Hardcoded suffix filters (.pyc, .png, .wasm, etc.)
 *   3. Fast-mode additional filters (docs, examples, lock files, etc.)
 *   4. Gitignore-style pattern matching
 *   5. Language detection for accepted files
 */
#include "discover/discover.h"
#include "discover/discover_string_helpers.h"
#include "discover/path_join.h"
#include "discover/trailing_sep.h"
#include "cbm.h" // CBMLanguage, CBM_LANG_COUNT, CBM_LANG_JSON

#include "foundation/constants.h"
#include "discover/local_rel_path.h"
#include "foundation/compat_fs.h"
#include "foundation/platform.h"
#ifdef _WIN32
#include "foundation/win_utf8.h"
#endif
#include <ctype.h>
#include <stdint.h> // int64_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strdup
#include <sys/stat.h>

int cbm_gitignore_match_result(const cbm_gitignore_t *gi, const char *rel_path, bool is_dir);

/* ── Ignored JSON filenames ──────────────────────── */

static const char *IGNORED_JSON_FILES[] = {
    "package.json",       "package-lock.json", "tsconfig.json",
    "jsconfig.json",      "composer.json",     "composer.lock",
    "yarn.lock",          "openapi.json",      "swagger.json",
    "jest.config.json",   ".eslintrc.json",    ".prettierrc.json",
    ".babelrc.json",      "tslint.json",       "angular.json",
    "firebase.json",      "renovate.json",     "lerna.json",
    "turbo.json",         ".stylelintrc.json", "pnpm-lock.json",
    "deno.json",          "biome.json",        "devcontainer.json",
    ".devcontainer.json", "launch.json",       "settings.json",
    "extensions.json",    "tasks.json",        NULL};

/* ── Git global excludes resolution ───────────────────────────── */

enum { GIT_TILDE_PREFIX_LEN = 2 }; /* "~/". */

static char *trim_ws(char *s) {
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';
    return s;
}

static void strip_inline_comment(char *s) {
    bool in_quote = false;
    char quote = '\0';
    for (char *p = s; *p; p++) {
        if ((*p == '"' || *p == '\'') && (p == s || p[-1] != '\\')) {
            if (!in_quote) {
                in_quote = true;
                quote = *p;
            } else if (*p == quote) {
                in_quote = false;
            }
            continue;
        }
        if (!in_quote && (*p == '#' || *p == ';') && (p == s || isspace((unsigned char)p[-1]))) {
            *p = '\0';
            return;
        }
    }
}

static char *strip_matching_quotes(char *s) {
    size_t len = strlen(s);
    if (len >= CBM_QUOTE_PAIR && ((s[0] == '"' && s[len - SKIP_ONE] == '"') ||
                                  (s[0] == '\'' && s[len - SKIP_ONE] == '\''))) {
        s[len - SKIP_ONE] = '\0';
        return s + SKIP_ONE;
    }
    return s;
}

static bool expand_git_path(const char *path, char *out, size_t out_sz) {
    if (!path || !path[0] || !out || out_sz == 0) {
        return false;
    }
    char normalized[CBM_SZ_4K];
    snprintf(normalized, sizeof(normalized), "%s", path);
    cbm_normalize_path_sep(normalized);

    if (normalized[0] != '~') {
        snprintf(out, out_sz, "%s", normalized);
        cbm_normalize_path_sep(out);
        return out[0] != '\0';
    }

    if (normalized[1] != '\0' && normalized[1] != '/') {
        return false; /* ~user expansion is intentionally not supported. */
    }

    const char *home = cbm_get_home_dir();
    if (!home || home[0] == '\0') {
        return false;
    }
    if (normalized[1] == '\0') {
        snprintf(out, out_sz, "%s", home);
        cbm_normalize_path_sep(out);
    } else {
        cbm_discover_path_join(home, normalized + GIT_TILDE_PREFIX_LEN, out, out_sz);
    }
    return out[0] != '\0';
}

static bool read_core_excludes_file(const char *config_path, char *out, size_t out_sz) {
    FILE *f = fopen(config_path, "r");
    if (!f) {
        return false;
    }

    bool in_core = false;
    bool found = false;
    char line[CBM_SZ_4K];
    while (fgets(line, sizeof(line), f)) {
        char *s = trim_ws(line);
        if (s[0] == '\0' || s[0] == '#' || s[0] == ';') {
            continue;
        }

        if (s[0] == '[') {
            char *end = strchr(s, ']');
            if (!end) {
                in_core = false;
                continue;
            }
            *end = '\0';
            in_core = cbm_discover_ascii_ieq(trim_ws(s + SKIP_ONE), "core");
            continue;
        }

        if (!in_core) {
            continue;
        }

        char *eq = strchr(s, '=');
        if (!eq) {
            continue;
        }
        *eq = '\0';
        char *key = trim_ws(s);
        char *value = trim_ws(eq + SKIP_ONE);
        strip_inline_comment(value);
        value = strip_matching_quotes(trim_ws(value));

        if (cbm_discover_ascii_ieq(key, "excludesfile") && value[0] != '\0' &&
            expand_git_path(value, out, out_sz)) {
            found = true;
        }
    }

    fclose(f);
    return found;
}

static bool resolve_xdg_git_config_dir(char *out, size_t out_sz) {
    char env[CBM_SZ_4K];
    if (cbm_safe_getenv("XDG_CONFIG_HOME", env, sizeof(env), NULL) && env[0] != '\0') {
        snprintf(out, out_sz, "%s", env);
        cbm_normalize_path_sep(out);
        return true;
    }

    const char *home = cbm_get_home_dir();
    if (!home || home[0] == '\0') {
        return false;
    }
    cbm_discover_path_join(home, ".config", out, out_sz);
    return out[0] != '\0';
}

static bool resolve_global_excludes_path(char *out, size_t out_sz) {
    char config_path[CBM_SZ_4K];
    config_path[0] = '\0';

    const char *home = cbm_get_home_dir();
    if (home && home[0] != '\0') {
        cbm_discover_path_join(home, ".gitconfig", config_path, sizeof(config_path));
        if (read_core_excludes_file(config_path, out, out_sz)) {
            return true;
        }
    }

    char xdg_config[CBM_SZ_4K];
    if (resolve_xdg_git_config_dir(xdg_config, sizeof(xdg_config))) {
        cbm_discover_path_join(xdg_config, "git/config", config_path, sizeof(config_path));
        if (read_core_excludes_file(config_path, out, out_sz)) {
            return true;
        }
        cbm_discover_path_join(xdg_config, "git/ignore", out, out_sz);
        return out[0] != '\0';
    }

    return false;
}

/* ── Dynamic file list ────────────────────────── */

typedef struct {
    cbm_file_info_t *files;
    int count;
    int capacity;
    /* Directories skipped during the walk (rel paths), so callers can surface
     * which subtrees were dropped (#411). strdup'd; freed by the caller via
     * cbm_discover_free_excluded or internally when not requested. */
    char **excluded;
    int excluded_count;
    int excluded_cap;
} file_list_t;

static void file_list_add_excluded(file_list_t *fl, const char *rel_path) {
    if (!rel_path || rel_path[0] == '\0') {
        return;
    }
    if (fl->excluded_count >= fl->excluded_cap) {
        int new_cap = fl->excluded_cap ? fl->excluded_cap * PAIR_LEN : CBM_SZ_64;
        char **grown = realloc(fl->excluded, new_cap * sizeof(char *));
        if (!grown) {
            return;
        }
        fl->excluded = grown;
        fl->excluded_cap = new_cap;
    }
    char *copy = strdup(rel_path);
    if (!copy) {
        return;
    }
    fl->excluded[fl->excluded_count++] = copy;
}

static void fl_add(file_list_t *fl, const char *abs_path, const char *rel_path, CBMLanguage lang,
                   int64_t size) {
    if (fl->count >= fl->capacity) {
        int new_cap = fl->capacity ? fl->capacity * PAIR_LEN : CBM_SZ_256;
        cbm_file_info_t *new_files = realloc(fl->files, new_cap * sizeof(cbm_file_info_t));
        if (!new_files) {
            return;
        }
        fl->files = new_files;
        fl->capacity = new_cap;
    }

    cbm_file_info_t *fi = &fl->files[fl->count++];
    fi->path = strdup(abs_path);
    fi->rel_path = strdup(rel_path);
    fi->language = lang;
    fi->size = size;
}

/* ── Recursive walk ─────────────────────────────── */

/* Check if a directory entry should be skipped (hardcoded dirs + gitignore). */
static bool should_skip_directory(const char *entry_name, const char *rel_path,
                                  const cbm_discover_opts_t *opts, const cbm_gitignore_t *gitignore,
                                  const cbm_gitignore_t *global_gi,
                                  const cbm_gitignore_t *cbmignore, const cbm_gitignore_t *local_gi,
                                  const char *local_gi_prefix) {
    if (cbm_should_skip_dir(entry_name, opts ? opts->mode : CBM_MODE_FULL)) {
        return true;
    }
    if (gitignore && cbm_gitignore_matches(gitignore, rel_path, true)) {
        return true;
    }
    bool global_ignored = global_gi && cbm_gitignore_matches(global_gi, rel_path, true);
    if (local_gi) {
        const char *lrel =
            rel_path + cbm_discover_local_rel_path_offset(rel_path, local_gi_prefix);
        if (cbm_gitignore_matches(local_gi, lrel, true)) {
            return true;
        }
    }
    if (cbmignore) {
        int cbm_result = cbm_gitignore_match_result(cbmignore, rel_path, true);
        if (cbm_result > 0) {
            return true;
        }
        if (cbm_result < 0 && global_ignored) {
            return false;
        }
    }
    return global_ignored;
}

/* Check if a regular file should be skipped (filters + gitignore + size). */
static bool should_skip_file(const char *entry_name, const char *rel_path,
                             const cbm_discover_opts_t *opts, const cbm_gitignore_t *gitignore,
                             const cbm_gitignore_t *global_gi, const cbm_gitignore_t *cbmignore,
                             const cbm_gitignore_t *local_gi, const char *local_gi_prefix,
                             off_t file_size) {
    cbm_index_mode_t mode = opts ? opts->mode : CBM_MODE_FULL;
    if (cbm_has_ignored_suffix(entry_name, mode)) {
        return true;
    }
    if (cbm_should_skip_filename(entry_name, mode)) {
        return true;
    }
    if (cbm_matches_fast_pattern(entry_name, mode)) {
        return true;
    }
    if (gitignore && cbm_gitignore_matches(gitignore, rel_path, false)) {
        return true;
    }
    bool global_ignored = global_gi && cbm_gitignore_matches(global_gi, rel_path, false);
    if (local_gi) {
        const char *lrel =
            rel_path + cbm_discover_local_rel_path_offset(rel_path, local_gi_prefix);
        if (cbm_gitignore_matches(local_gi, lrel, false)) {
            return true;
        }
    }
    if (cbmignore) {
        int cbm_result = cbm_gitignore_match_result(cbmignore, rel_path, false);
        if (cbm_result > 0) {
            return true;
        }
        if (cbm_result < 0 && global_ignored) {
            global_ignored = false;
        }
    }
    if (opts && opts->max_file_size > 0 && file_size > opts->max_file_size) {
        return true;
    }
    return global_ignored;
}

/* Detect language for a file, handling .m disambiguation and JSON filtering. */
static CBMLanguage detect_file_language(const char *entry_name, const char *abs_path) {
    CBMLanguage lang = cbm_language_for_filename(entry_name);
    if (lang == CBM_LANG_COUNT) {
        return CBM_LANG_COUNT;
    }
    /* Special: .m files need content-based disambiguation */
    const char *dot = strrchr(entry_name, '.');
    if (dot && strcmp(dot, ".m") == 0) {
        lang = cbm_disambiguate_m(abs_path);
    }
    /* Check ignored JSON files */
    if (lang == CBM_LANG_JSON && cbm_discover_str_in_list(entry_name, IGNORED_JSON_FILES)) {
        return CBM_LANG_COUNT;
    }
    return lang;
}

/* UTF-8-safe stat: wide API on Windows, regular stat on POSIX. */
static int wide_stat(const char *path, struct stat *st) {
#ifdef _WIN32
    wchar_t *wpath = cbm_utf8_to_wide(path);
    if (!wpath) {
        return CBM_NOT_FOUND;
    }
    struct _stat64 wst;
    int ret = _wstat64(wpath, &wst);
    free(wpath);
    if (ret != 0) {
        return CBM_NOT_FOUND;
    }
    st->st_mode = wst.st_mode;
    st->st_size = wst.st_size;
    st->st_mtime = wst.st_mtime;
    return 0;
#else
    return stat(path, st);
#endif
}

/* Stat a path, skipping symlinks. Returns 0 on success, -1 to skip. */
static int safe_stat(const char *abs_path, struct stat *st) {
#ifdef _WIN32
    return wide_stat(abs_path, st);
#else
    if (lstat(abs_path, st) != 0) {
        return CBM_NOT_FOUND;
    }
    if (S_ISLNK(st->st_mode)) {
        return CBM_NOT_FOUND;
    }
    return 0;
#endif
}

/* Process a single regular file entry during directory walk. */
static void walk_dir_process_file(const char *abs_path, const char *rel_path, const char *name,
                                  const cbm_discover_opts_t *opts, const cbm_gitignore_t *gitignore,
                                  const cbm_gitignore_t *global_gi,
                                  const cbm_gitignore_t *cbmignore, const cbm_gitignore_t *local_gi,
                                  const char *local_gi_prefix, off_t size, file_list_t *out) {
    if (should_skip_file(name, rel_path, opts, gitignore, global_gi, cbmignore, local_gi,
                         local_gi_prefix, size)) {
        return;
    }
    CBMLanguage lang = detect_file_language(name, abs_path);
    if (lang == CBM_LANG_COUNT) {
        return;
    }
    fl_add(out, abs_path, rel_path, lang, size);
}

typedef struct {
    char dir[CBM_SZ_4K];
    char prefix[CBM_SZ_4K];
    cbm_gitignore_t *local_gi;       /* nested .gitignore for this subtree */
    char local_gi_prefix[CBM_SZ_4K]; /* rel_prefix when local_gi was loaded */
} walk_frame_t;
#define WALK_STACK_CAP 512
/* Build abs/rel paths and process one directory entry. */
/* Try to load a nested .gitignore from this directory. Returns owned pointer or NULL. */
static cbm_gitignore_t *try_load_nested_gitignore(const walk_frame_t *frame) {
    if (frame->local_gi || frame->prefix[0] == '\0') {
        return NULL;
    }
    char gi_path[CBM_SZ_4K];
    snprintf(gi_path, sizeof(gi_path), "%s/.gitignore", frame->dir);
    struct stat gi_st;
    if (wide_stat(gi_path, &gi_st) == 0 && S_ISREG(gi_st.st_mode)) {
        return cbm_gitignore_load(gi_path);
    }
    return NULL;
}

/* Push a subdirectory onto the walk stack, inheriting local gitignore context. */
static void walk_push_subdir(walk_frame_t *stack, int *top, const char *abs_path,
                             const char *rel_path, const walk_frame_t *parent) {
    if (*top >= WALK_STACK_CAP) {
        return;
    }
    snprintf(stack[*top].dir, CBM_SZ_4K, "%s", abs_path);
    snprintf(stack[*top].prefix, CBM_SZ_4K, "%s", rel_path);
    stack[*top].local_gi = parent->local_gi;
    snprintf(stack[*top].local_gi_prefix, CBM_SZ_4K, "%s", parent->local_gi_prefix);
    (*top)++;
}

static void walk_dir_process_entry(cbm_dirent_t *entry, const walk_frame_t *frame,
                                   const cbm_discover_opts_t *opts,
                                   const cbm_gitignore_t *gitignore,
                                   const cbm_gitignore_t *global_gi,
                                   const cbm_gitignore_t *cbmignore, walk_frame_t *stack, int *top,
                                   file_list_t *out) {
    char abs_path[CBM_SZ_4K];
    char rel_path[CBM_SZ_4K];
    snprintf(abs_path, sizeof(abs_path), "%s/%s", frame->dir, entry->name);
    if (frame->prefix[0] != '\0') {
        snprintf(rel_path, sizeof(rel_path), "%s/%s", frame->prefix, entry->name);
    } else {
        snprintf(rel_path, sizeof(rel_path), "%s", entry->name);
    }

    struct stat st;
    if (safe_stat(abs_path, &st) != 0) {
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        if (!should_skip_directory(entry->name, rel_path, opts, gitignore, global_gi, cbmignore,
                                   frame->local_gi, frame->local_gi_prefix)) {
            walk_push_subdir(stack, top, abs_path, rel_path, frame);
        } else {
            /* Record the excluded subtree root so callers can report it (#411). */
            file_list_add_excluded(out, rel_path);
        }
    } else if (S_ISREG(st.st_mode)) {
        walk_dir_process_file(abs_path, rel_path, entry->name, opts, gitignore, global_gi,
                              cbmignore, frame->local_gi, frame->local_gi_prefix, st.st_size, out);
    }
}

enum { GI_OWNED_CAP = 64 };

static void walk_dir(const char *dir_path, const char *rel_prefix, const cbm_discover_opts_t *opts,
                     const cbm_gitignore_t *gitignore, const cbm_gitignore_t *global_gi,
                     const cbm_gitignore_t *cbmignore, file_list_t *out) {
    walk_frame_t *stack = calloc(WALK_STACK_CAP, sizeof(walk_frame_t));
    if (!stack) {
        return;
    }
    /* Collect all owned gitignores — freed at the end because child frames
     * on the stack hold borrowed pointers to them. */
    cbm_gitignore_t *owned_gis[GI_OWNED_CAP];
    int owned_count = 0;

    int top = 0;
    snprintf(stack[top].dir, CBM_SZ_4K, "%s", dir_path);
    snprintf(stack[top].prefix, CBM_SZ_4K, "%s", rel_prefix);
    top++;

    while (top > 0) {
        walk_frame_t frame = stack[--top];

        cbm_gitignore_t *loaded = try_load_nested_gitignore(&frame);
        if (loaded) {
            frame.local_gi = loaded;
            snprintf(frame.local_gi_prefix, sizeof(frame.local_gi_prefix), "%s", frame.prefix);
            if (owned_count < GI_OWNED_CAP) {
                owned_gis[owned_count++] = loaded;
            }
        }

        cbm_dir_t *d = cbm_opendir(frame.dir);
        if (!d) {
            continue;
        }

        cbm_dirent_t *entry;
        while ((entry = cbm_readdir(d)) != NULL) {
            walk_dir_process_entry(entry, &frame, opts, gitignore, global_gi, cbmignore, stack,
                                   &top, out);
        }
        cbm_closedir(d);
    }
    for (int i = 0; i < owned_count; i++) {
        cbm_gitignore_free(owned_gis[i]);
    }
    free(stack);
}

/* ── Public API ───────────────────────────────── */

int cbm_discover(const char *repo_path, const cbm_discover_opts_t *opts, cbm_file_info_t **out,
                 int *count) {
    return cbm_discover_ex(repo_path, opts, out, count, NULL, NULL);
}

int cbm_discover_ex(const char *repo_path, const cbm_discover_opts_t *opts, cbm_file_info_t **out,
                    int *count, char ***excluded_out, int *excluded_count_out) {
    if (excluded_out) {
        *excluded_out = NULL;
    }
    if (excluded_count_out) {
        *excluded_count_out = 0;
    }
    if (!repo_path || !out || !count) {
        return CBM_NOT_FOUND;
    }

    *out = NULL;
    *count = 0;

    /* Verify directory exists */
    struct stat st;
    if (wide_stat(repo_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        return CBM_NOT_FOUND;
    }

    /* Load gitignore sources when a .git directory is present.
     * Sources merged in order (later patterns win on conflict):
     *   1. <repo>/.gitignore        — committed exclusions
     *   2. <repo>/.git/info/exclude — per-clone exclusions, not committed
     * Both are folded into a single matcher so all downstream call paths
     * remain unchanged.  Fixes issue #489: OOM on repos whose worktrees
     * are excluded only via .git/info/exclude (e.g. Sandcastle). */
    cbm_gitignore_t *gitignore = NULL;
    char gi_path[CBM_SZ_4K];
    snprintf(gi_path, sizeof(gi_path), "%s/.git", repo_path);
    struct stat gi_stat;
    bool is_git_repo = wide_stat(gi_path, &gi_stat) == 0 && S_ISDIR(gi_stat.st_mode);
    bool has_git_config = false;
    /* Always honour the .gitignore at the indexed-directory root, even when the
     * directory is not a git repo root (e.g. indexing a sub-package directly).
     * The .git/info/exclude and global-excludes sources still require .git/.
     * Fixes issue #510: a root .gitignore was silently ignored without .git/. */
    snprintf(gi_path, sizeof(gi_path), "%s/.gitignore", repo_path);
    gitignore = cbm_gitignore_load(gi_path);
    if (is_git_repo) {
        snprintf(gi_path, sizeof(gi_path), "%s/.git/config", repo_path);
        has_git_config = wide_stat(gi_path, &gi_stat) == 0 && S_ISREG(gi_stat.st_mode);

        char exc_path[CBM_SZ_4K];
        snprintf(exc_path, sizeof(exc_path), "%s/.git/info/exclude", repo_path);
        cbm_gitignore_t *git_exclude = cbm_gitignore_load(exc_path);
        if (git_exclude) {
            if (!gitignore) {
                gitignore = git_exclude;
            } else {
                /* On allocation failure the merge is atomic (dst unchanged), so
                 * the .gitignore patterns still apply; the exclude patterns are
                 * simply skipped — same as if .git/info/exclude were absent. */
                (void)cbm_gitignore_merge(gitignore, git_exclude);
                cbm_gitignore_free(git_exclude);
            }
        }
    }

    cbm_gitignore_t *global_gi = NULL;
    if (has_git_config && resolve_global_excludes_path(gi_path, sizeof(gi_path))) {
        global_gi = cbm_gitignore_load(gi_path);
    }

    /* Load cbmignore if specified or exists at repo root */
    cbm_gitignore_t *cbmignore = NULL;
    if (opts && opts->ignore_file) {
        cbmignore = cbm_gitignore_load(opts->ignore_file);
    } else {
        snprintf(gi_path, sizeof(gi_path), "%s/.cbmignore", repo_path);
        cbmignore = cbm_gitignore_load(gi_path);
    }

    /* Walk */
    file_list_t fl = {0};
    walk_dir(repo_path, "", opts, gitignore, global_gi, cbmignore, &fl);

    /* Cleanup */
    cbm_gitignore_free(gitignore);
    cbm_gitignore_free(global_gi);
    cbm_gitignore_free(cbmignore);

    *out = fl.files;
    *count = fl.count;

    /* Hand the excluded-dir list to the caller, or free it if not requested. */
    if (excluded_out) {
        *excluded_out = fl.excluded;
        if (excluded_count_out) {
            *excluded_count_out = fl.excluded_count;
        }
    } else {
        cbm_discover_free_excluded(fl.excluded, fl.excluded_count);
    }
    return 0;
}

void cbm_discover_free(cbm_file_info_t *files, int count) {
    if (!files) {
        return;
    }
    for (int i = 0; i < count; i++) {
        free(files[i].path);
        free(files[i].rel_path);
    }
    free(files);
}

void cbm_discover_free_excluded(char **excluded, int count) {
    if (!excluded) {
        return;
    }
    for (int i = 0; i < count; i++) {
        free(excluded[i]);
    }
    free(excluded);
}
