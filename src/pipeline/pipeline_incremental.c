/*
 * pipeline_incremental.c — Disk-based incremental re-indexing.
 *
 * Operates on the existing SQLite DB directly (not RAM-first graph buffer).
 * Compares file mtime+size against stored hashes to classify changed/unchanged.
 * Deletes changed files' nodes (edges cascade via ON DELETE CASCADE),
 * re-parses only changed files through passes into a temp graph buffer,
 * then merges new nodes/edges into the disk DB. Persists updated hashes.
 *
 * Called from pipeline.c when a DB with stored hashes already exists.
 */
#include "foundation/constants.h"

enum { INCR_RING_BUF = 4, INCR_RING_MASK = 3, INCR_TS_BUF = 24, INCR_WAL_BUF = 1040 };
#include "pipeline/pipeline.h"
#include "pipeline/artifact.h"
#include "pipeline/rust_plan.h"
#include <stdio.h>
#include <time.h>
#include "pipeline/pipeline_internal.h"

#include "pipeline/incremental_edge_type.h"
#include "pipeline/incremental_registry_label.h"
#include "store/store.h"
#include "graph_buffer/graph_buffer.h"
#include "discover/discover.h"
#include "foundation/log.h"
#include "foundation/hash_table.h"
#include "foundation/compat.h"
#include "foundation/compat_fs.h"
#include "foundation/platform.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#endif

/* ── Constants ───────────────────────────────────────────────────── */

#define CBM_MS_PER_SEC 1000.0
#define CBM_NS_PER_MS 1000000.0
#define CBM_NS_PER_SEC 1000000000LL

/* ── Timing helper (same as pipeline.c) ──────────────────────────── */

static double elapsed_ms(struct timespec start) {
    struct timespec now;
    cbm_clock_gettime(CLOCK_MONOTONIC, &now);
    double s = (double)(now.tv_sec - start.tv_sec);
    double ns = (double)(now.tv_nsec - start.tv_nsec);
    return (s * CBM_MS_PER_SEC) + (ns / CBM_NS_PER_MS);
}

/* itoa into static buffer — matches pipeline.c helper */
static const char *itoa_buf(int v) {
    static _Thread_local char buf[INCR_RING_BUF][INCR_TS_BUF];
    static _Thread_local int idx = 0;
    idx = (idx + SKIP_ONE) & INCR_RING_MASK;
    snprintf(buf[idx], sizeof(buf[idx]), "%d", v);
    return buf[idx];
}

/* ── Platform-portable mtime_ns ──────────────────────────────────── */

static int64_t stat_mtime_ns(const struct stat *st) {
#ifdef __APPLE__
    return ((int64_t)st->st_mtimespec.tv_sec * CBM_NS_PER_SEC) + (int64_t)st->st_mtimespec.tv_nsec;
#elif defined(_WIN32)
    return (int64_t)st->st_mtime * CBM_NS_PER_SEC;
#else
    return ((int64_t)st->st_mtim.tv_sec * CBM_NS_PER_SEC) + (int64_t)st->st_mtim.tv_nsec;
#endif
}

/* ── File classification ─────────────────────────────────────────── */

/* Classify discovered files against stored hashes using mtime+size.
 * Returns a boolean array: changed[i] = true if files[i] needs re-parsing.
 * Caller must free the returned array. */
static bool *classify_files(cbm_file_info_t *files, int file_count, cbm_file_hash_t *stored,
                            int stored_count, int *out_changed, int *out_unchanged) {
    bool *changed = calloc((size_t)file_count, sizeof(bool));
    if (!changed) {
        return NULL;
    }

    int n_changed = 0;
    int n_unchanged = 0;

    /* Build lookup: rel_path -> stored hash */
    CBMHashTable *ht =
        cbm_ht_create(stored_count > 0 ? (size_t)stored_count * PAIR_LEN : CBM_SZ_64);
    for (int i = 0; i < stored_count; i++) {
        cbm_ht_set(ht, stored[i].rel_path, &stored[i]);
    }

    for (int i = 0; i < file_count; i++) {
        cbm_file_hash_t *h = cbm_ht_get(ht, files[i].rel_path);
        if (!h) {
            /* New file */
            changed[i] = true;
            n_changed++;
            continue;
        }

        struct stat st;
        if (stat(files[i].path, &st) != 0) {
            changed[i] = true;
            n_changed++;
            continue;
        }

        if (stat_mtime_ns(&st) != h->mtime_ns || st.st_size != h->size) {
            changed[i] = true;
            n_changed++;
        } else {
            n_unchanged++;
        }
    }

    cbm_ht_free(ht);
    *out_changed = n_changed;
    *out_unchanged = n_unchanged;
    return changed;
}

/* Classify stored files that are absent from current discovery. Returns the
 * count of truly-deleted files (output via out_deleted) and ALSO collects
 * mode-skipped files into out_mode_skipped (caller frees both).
 *
 * A stored file is classified as:
 *   - "deleted"      — `stat()` returns ENOENT or ENOTDIR. Its nodes will
 *                       be purged and its hash row dropped.
 *   - "mode-skipped" — `stat()` succeeds. The file exists on disk but the
 *                       current discovery pass didn't visit it (e.g. excluded
 *                       by FAST_SKIP_DIRS in fast/moderate mode). Its nodes
 *                       must be preserved AND its hash row must be carried
 *                       forward into the new DB so subsequent reindexes can
 *                       still see it as "known" rather than treating it as
 *                       new-or-deleted.
 *
 * Without this distinction, a fast-mode reindex after a full-mode index
 * would silently purge every file under `tools/`, `scripts/`, `bin/`,
 * `build/`, `docs/`, `__tests__/`, etc. — see task
 * claude-connectors/codebase-memory-index-repository-is-destructive-...
 * and the 2026-04-13 Skyline incident (packages/mcp/src/tools/ vanished
 * from a live graph mid-session).
 *
 * Mode-skipped hash preservation is the second half of the additive-merge
 * contract: dump_and_persist re-upserts these hash rows so the next reindex
 * can correctly detect a real on-disk deletion of a mode-skipped file (as
 * opposed to seeing it as "never existed" → noop → orphaned graph nodes).
 *
 * Fail-safe rules (preserve nodes on uncertainty):
 *   - repo_path NULL → log error and preserve everything (return 0
 *     deletions, empty mode_skipped). The caller contract is that
 *     repo_path is required; a NULL means a misconfigured pipeline,
 *     not a deletion signal.
 *   - snprintf truncation (combined path ≥ CBM_SZ_4K) → preserve. We can't
 *     reliably stat a truncated path. Treat as mode-skipped.
 *   - stat() errno != ENOENT/ENOTDIR (EACCES, EIO, ELOOP, transient NFS,
 *     etc.) → preserve. The file may exist; we just can't see it right now.
 *     Treat as mode-skipped.
 *
 * Note: we use stat() (not lstat()) on purpose. A symlink whose target was
 * deleted should be classified as deleted from the indexer's perspective
 * because the indexer follows symlinks during discovery — a stale symlink
 * has no source to parse. */
static int find_deleted_files(const char *repo_path, cbm_file_info_t *files, int file_count,
                              cbm_file_hash_t *stored, int stored_count, char ***out_deleted,
                              cbm_file_hash_t **out_mode_skipped, int *out_mode_skipped_count) {
    *out_deleted = NULL;
    *out_mode_skipped = NULL;
    *out_mode_skipped_count = 0;

    if (!repo_path) {
        /* Misconfigured pipeline. Preserve everything rather than risk
         * silently re-introducing the destructive overwrite this function
         * was rewritten to prevent. */
        cbm_log_error("incremental.err", "msg", "find_deleted_files_null_repo_path");
        return 0;
    }

    CBMHashTable *current = cbm_ht_create((size_t)file_count * PAIR_LEN);
    for (int i = 0; i < file_count; i++) {
        cbm_ht_set(current, files[i].rel_path, &files[i]);
    }

    int del_count = 0;
    int del_cap = CBM_SZ_64;
    char **deleted = malloc((size_t)del_cap * sizeof(char *));
    if (!deleted) {
        cbm_log_error("incremental.err", "msg", "find_deleted_files_oom");
        cbm_ht_free(current);
        return 0;
    }

    int ms_count = 0;
    int ms_cap = CBM_SZ_64;
    cbm_file_hash_t *mode_skipped = malloc((size_t)ms_cap * sizeof(cbm_file_hash_t));
    if (!mode_skipped) {
        cbm_log_error("incremental.err", "msg", "find_deleted_files_oom_ms");
        free(deleted);
        cbm_ht_free(current);
        return 0;
    }

    for (int i = 0; i < stored_count; i++) {
        if (cbm_ht_get(current, stored[i].rel_path)) {
            continue; /* still visited by current pass */
        }
        /* Not in current discovery — check if it's truly deleted or just
         * mode-skipped (excluded by FAST_SKIP_DIRS etc.). */
        bool preserve = false;
        char abs_path[CBM_SZ_4K];
        int n = snprintf(abs_path, sizeof(abs_path), "%s/%s", repo_path, stored[i].rel_path);
        if (n < 0 || n >= (int)sizeof(abs_path)) {
            /* Truncation or encoding error — can't reliably stat. Preserve. */
            cbm_log_warn("incremental.path_truncated", "rel_path", stored[i].rel_path);
            preserve = true;
        } else {
            struct stat st;
            if (stat(abs_path, &st) == 0) {
                /* File exists on disk — mode-skipped, not deleted. */
                preserve = true;
            } else if (errno != ENOENT && errno != ENOTDIR) {
                /* Transient or permission error — fail safe by preserving.
                 * EACCES, EIO, ELOOP, ENAMETOOLONG, etc. */
                cbm_log_warn("incremental.stat_uncertain", "rel_path", stored[i].rel_path, "errno",
                             itoa_buf(errno));
                preserve = true;
            }
        }

        if (preserve) {
            /* Carry forward the existing hash row so subsequent reindexes
             * can correctly classify this file. */
            if (ms_count >= ms_cap) {
                ms_cap *= PAIR_LEN;
                cbm_file_hash_t *tmp = realloc(mode_skipped, (size_t)ms_cap * sizeof(*tmp));
                if (!tmp) {
                    cbm_log_error("incremental.err", "msg", "find_deleted_files_realloc_oom_ms");
                    break;
                }
                mode_skipped = tmp;
            }
            char *rp = strdup(stored[i].rel_path);
            char *sh = stored[i].sha256 ? strdup(stored[i].sha256) : NULL;
            if (!rp || (stored[i].sha256 && !sh)) {
                /* OOM mid-record. Drop this entry rather than persist a
                 * row with a NULL rel_path that would silently fail the
                 * NOT NULL constraint in upsert and reintroduce the
                 * orphaned-node bug. */
                cbm_log_error("incremental.err", "msg", "find_deleted_files_strdup_oom", "rel_path",
                              stored[i].rel_path);
                free(rp);
                free(sh);
                break;
            }
            mode_skipped[ms_count].project = NULL; /* unused by upsert API */
            mode_skipped[ms_count].rel_path = rp;
            mode_skipped[ms_count].sha256 = sh;
            mode_skipped[ms_count].mtime_ns = stored[i].mtime_ns;
            mode_skipped[ms_count].size = stored[i].size;
            ms_count++;
            continue;
        }

        /* File is truly gone — record for purge. */
        if (del_count >= del_cap) {
            del_cap *= PAIR_LEN;
            char **tmp = realloc(deleted, (size_t)del_cap * sizeof(char *));
            if (!tmp) {
                cbm_log_error("incremental.err", "msg", "find_deleted_files_realloc_oom");
                break;
            }
            deleted = tmp;
        }
        deleted[del_count++] = strdup(stored[i].rel_path);
    }

    cbm_ht_free(current);
    *out_deleted = deleted;
    *out_mode_skipped = mode_skipped;
    *out_mode_skipped_count = ms_count;
    return del_count;
}

/* Free a mode_skipped array allocated by find_deleted_files. */
static void free_mode_skipped(cbm_file_hash_t *ms, int count) {
    if (!ms) {
        return;
    }
    for (int i = 0; i < count; i++) {
        free((void *)ms[i].rel_path);
        free((void *)ms[i].sha256);
    }
    free(ms);
}

/* ── Inbound cross-file edge preservation (incremental correctness) ──
 *
 * The purge step (cbm_gbuf_delete_by_file) removes a changed file's nodes,
 * and the cascade then drops every edge referencing them — INCLUDING inbound
 * edges whose source lives in an UNCHANGED file (e.g. StudyService.grade ->
 * SM2.review, or a Folder -> File containment edge). Because incremental only
 * re-parses the changed files, the resolution passes never regenerate those
 * inbound edges, so the graph silently loses cross-file CALLS / USAGE /
 * CONTAINS_FILE / INHERITS / ... edges on every edit and diverges from a
 * clean full reindex (which resolves every file).
 *
 * Fix: snapshot the inbound cross-file edges into changed files BEFORE the
 * purge, keyed by endpoint qualified_name (stable across re-parse), then
 * re-link them AFTER re-resolution + post-passes. Notes:
 *   - Only edges whose target is in a changed file and whose source is NOT
 *     are snapshotted; edges out of a changed file are regenerated when that
 *     file is re-resolved.
 *   - Edge types recomputed wholesale by post-passes (SIMILAR_TO,
 *     SEMANTICALLY_RELATED) are skipped — re-linking a stale snapshot could
 *     add edges a full reindex would not produce.
 *   - cbm_gbuf_insert_edge dedups, so re-linking an edge the resolver already
 *     recreated is a harmless no-op.
 *   - A target whose qualified_name no longer exists (symbol deleted or
 *     renamed by the edit) is dropped — matching full-reindex semantics. */

typedef struct {
    char *source_qn;
    char *target_qn;
    char *type;
    char *props;
} cbm_saved_edge_t;

typedef struct {
    cbm_gbuf_t *gbuf;
    CBMHashTable *changed_paths; /* rel_path -> non-NULL sentinel (membership set) */
    cbm_saved_edge_t *items;
    int count;
    int cap;
} cbm_edge_capture_t;

/* Edge types that must NOT be re-linked from the pre-purge snapshot, because a
 * full reindex (re)computes them via a pass whose result can differ from the
 * snapshot — restoring a stale copy could leave wrong properties or even an
 * edge a full reindex would not produce:
 *   - SIMILAR_TO / SEMANTICALLY_RELATED: rebuilt wholesale by the incremental
 *     post-passes (similarity / semantic_edges) over a drifting corpus.
 *   - FILE_CHANGES_WITH (git-history coupling) and DATA_FLOWS (route data flow):
 *     produced only by full-pipeline post-passes (githistory / route_nodes)
 *     that do NOT run during incremental; they remain a known incremental
 *     limitation rather than something to restore stale.
 * Every other edge type IS safe to re-link, by one of two routes that both
 * match a full reindex: edges re-emitted by the per-file resolution passes that
 * run incrementally (CALLS, USAGE, DEFINES, DEFINES_METHOD, INHERITS,
 * IMPLEMENTS) are deduped on re-link, while structural containment edges
 * (CONTAINS_FILE, CONTAINS_FOLDER) — which the full-only structure pass does
 * NOT regenerate incrementally — are preserved precisely by this snapshot. */
/* cbm_gbuf_foreach_edge visitor: snapshot inbound cross-file edges into
 * changed files so they survive the purge and can be re-linked afterward. */
static void incr_capture_inbound_edge(const cbm_gbuf_edge_t *edge, void *userdata) {
    cbm_edge_capture_t *cap = (cbm_edge_capture_t *)userdata;
    if (cbm_pipeline_incremental_edge_type_is_recomputed(edge->type)) {
        return;
    }
    const cbm_gbuf_node_t *src = cbm_gbuf_find_by_id(cap->gbuf, edge->source_id);
    const cbm_gbuf_node_t *tgt = cbm_gbuf_find_by_id(cap->gbuf, edge->target_id);
    if (!src || !tgt || !src->qualified_name || !tgt->qualified_name || !src->file_path ||
        !tgt->file_path) {
        return;
    }
    /* Keep only edges that the purge would orphan permanently: target is in a
     * changed file (its node is deleted + re-created), source is NOT (its file
     * is never re-parsed, so the resolver won't regenerate the edge). */
    if (!cbm_ht_get(cap->changed_paths, tgt->file_path) ||
        cbm_ht_get(cap->changed_paths, src->file_path)) {
        return;
    }
    if (cap->count >= cap->cap) {
        int ncap = (cap->cap > 0) ? cap->cap * PAIR_LEN : CBM_SZ_64;
        cbm_saved_edge_t *tmp = realloc(cap->items, (size_t)ncap * sizeof(*tmp));
        if (!tmp) {
            cbm_log_warn("incremental.edge_snapshot_oom", "captured", itoa_buf(cap->count));
            return; /* best-effort: stop capturing, keep what we have */
        }
        cap->items = tmp;
        cap->cap = ncap;
    }
    cbm_saved_edge_t *s = &cap->items[cap->count];
    s->source_qn = strdup(src->qualified_name);
    s->target_qn = strdup(tgt->qualified_name);
    s->type = strdup(edge->type);
    s->props = strdup(edge->properties_json ? edge->properties_json : "{}");
    if (!s->source_qn || !s->target_qn || !s->type || !s->props) {
        free(s->source_qn);
        free(s->target_qn);
        free(s->type);
        free(s->props);
        return;
    }
    cap->count++;
}

/* Re-link snapshotted inbound edges to the freshly re-created target nodes.
 * Returns the number of edges re-linked. */
static int incr_restore_inbound_edges(cbm_gbuf_t *gbuf, cbm_edge_capture_t *cap) {
    int restored = 0;
    for (int i = 0; i < cap->count; i++) {
        cbm_saved_edge_t *s = &cap->items[i];
        const cbm_gbuf_node_t *src = cbm_gbuf_find_by_qn(gbuf, s->source_qn);
        const cbm_gbuf_node_t *tgt = cbm_gbuf_find_by_qn(gbuf, s->target_qn);
        if (src && tgt) {
            cbm_gbuf_apply_insert_edge(gbuf, src->id, tgt->id, s->type, s->props);
            restored++;
        }
    }
    return restored;
}

static void incr_free_edge_capture(cbm_edge_capture_t *cap) {
    for (int i = 0; i < cap->count; i++) {
        free(cap->items[i].source_qn);
        free(cap->items[i].target_qn);
        free(cap->items[i].type);
        free(cap->items[i].props);
    }
    free(cap->items);
    cap->items = NULL;
    cap->count = 0;
    cap->cap = 0;
}

/* ── Persist file hashes ─────────────────────────────────────────── */

/* Persist file hash rows for the current discovery and any mode-skipped
 * files preserved from the previous DB.
 *
 * Partial-failure policy: an `upsert` failure on any single row is logged
 * as a warning and the loop continues. We deliberately do NOT abort the
 * whole reindex on a single bad row — partial preservation is better than
 * total loss, and a transient failure on one file should not invalidate
 * the entire incremental update. The trade-off is that a silently-failed
 * row produces the same downstream effect as if the file were never
 * indexed at all (forced re-parse on the next run for current-files,
 * potential orphaned-node revival for mode_skipped). The warning surface
 * is the only signal that something went wrong. */
static void persist_hashes(cbm_store_t *store, const char *project, cbm_file_info_t *files,
                           int file_count, const cbm_file_hash_t *mode_skipped,
                           int mode_skipped_count) {
    int current_failed = 0;
    int ms_failed = 0;

    /* Current discovery: re-stat to capture any mtime/size that changed
     * during the run, and write fresh hash rows for visited files. */
    for (int i = 0; i < file_count; i++) {
        struct stat st;
        if (stat(files[i].path, &st) != 0) {
            continue;
        }
        int rc = cbm_store_upsert_file_hash(store, project, files[i].rel_path, "",
                                            stat_mtime_ns(&st), st.st_size);
        if (rc != CBM_STORE_OK) {
            cbm_log_warn("incremental.persist_hash_failed", "scope", "current", "rel_path",
                         files[i].rel_path, "rc", itoa_buf(rc));
            current_failed++;
        }
    }

    /* Mode-skipped (preserved): re-upsert hash rows from the previous DB
     * so the next reindex can still classify these files correctly. Without
     * this, an orphaned-node bug emerges where:
     *   - full mode indexes everything
     *   - fast mode runs and drops mode-skipped hash rows
     *   - file is then deleted on disk
     *   - next reindex's stored hashes don't include the file → noop or
     *     can't detect the deletion → graph nodes for the deleted file
     *     remain forever (or until a destructive rebuild).
     *
     * A failure here is more serious than a current-files failure because
     * it can revive the orphaned-node bug for that specific file. Logged
     * with scope=mode_skipped so the warning is searchable. */
    if (mode_skipped) {
        for (int i = 0; i < mode_skipped_count; i++) {
            int rc =
                cbm_store_upsert_file_hash(store, project, mode_skipped[i].rel_path,
                                           mode_skipped[i].sha256 ? mode_skipped[i].sha256 : "",
                                           mode_skipped[i].mtime_ns, mode_skipped[i].size);
            if (rc != CBM_STORE_OK) {
                cbm_log_warn("incremental.persist_hash_failed", "scope", "mode_skipped", "rel_path",
                             mode_skipped[i].rel_path, "rc", itoa_buf(rc));
                ms_failed++;
            }
        }
    }

    if (current_failed > 0 || ms_failed > 0) {
        cbm_log_warn("incremental.persist_summary", "current_failed", itoa_buf(current_failed),
                     "mode_skipped_failed", itoa_buf(ms_failed));
    }
}

/* ── Registry seed visitor ────────────────────────────────────────── */

/* Labels the full-index definition pass seeds into the registry
 * (pass_definitions.c — KEEP IN SYNC). Incremental re-resolution must see the
 * SAME symbol set, or it diverges from a clean full reindex: seeding extra
 * container nodes (File / Module / Folder / ...) lets a type usage like `Word`
 * resolve to the same-named Module node instead of the Class node. Only
 * callable / declared symbols belong in the registry. */
/* Callback for cbm_gbuf_foreach_node: seed the registry with the existing
 * project's definition symbols so the resolver can match cross-file symbols
 * during incremental. Mirrors the full-index registry contents exactly so an
 * incremental re-resolve picks the same nodes a full reindex would. */
static void registry_visitor(const cbm_gbuf_node_t *node, void *userdata) {
    cbm_registry_t *r = (cbm_registry_t *)userdata;
    if (!cbm_pipeline_incremental_label_is_registry_symbol(node->label)) {
        return;
    }
    cbm_registry_add(r, node->name, node->qualified_name, node->label);
}

#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
typedef struct {
    int kind;
    const char *name;
    uint32_t gate_flags;
} incr_extract_plan_def_t;

static uint64_t rust_plan_step_bit(int kind) {
    if (kind <= 0 || kind > CBM_RS_PLAN_STEP_MAX_KIND) {
        return 0;
    }
    return UINT64_C(1) << ((uint32_t)kind - 1u);
}

static bool append_plan_name(char *buf, size_t bufsize, size_t *pos, const char *name) {
    int written = snprintf(buf + *pos, bufsize - *pos, "%s%s", *pos > 0 ? "," : "", name);
    if (written < 0 || (size_t)written >= bufsize - *pos) {
        return false;
    }
    *pos += (size_t)written;
    return true;
}

static bool decode_rust_incremental_extract_steps(bool use_parallel,
                                                  const cbm_rs_pipeline_plan_step_v2_t *steps,
                                                  int step_count, char *plan_buf,
                                                  size_t plan_bufsize) {
    static const incr_extract_plan_def_t sequential[] = {
        {CBM_RS_PLAN_STEP_INCREMENTAL_DEFINITIONS, "definitions", 0},
        {CBM_RS_PLAN_STEP_INCREMENTAL_CALLS, "calls", 0},
        {CBM_RS_PLAN_STEP_INCREMENTAL_USAGES, "usages", 0},
        {CBM_RS_PLAN_STEP_INCREMENTAL_SEMANTIC, "semantic", 0},
    };
    static const incr_extract_plan_def_t parallel[] = {
        {CBM_RS_PLAN_STEP_INCREMENTAL_PARALLEL_EXTRACT, "incr_extract", 0},
        {CBM_RS_PLAN_STEP_INCREMENTAL_REGISTRY, "incr_registry", 0},
        {CBM_RS_PLAN_STEP_INCREMENTAL_RESOLVE, "incr_resolve",
         CBM_RS_PLAN_GATE_NO_CROSS_LSP_PREBUILD},
    };
    const incr_extract_plan_def_t *expected = use_parallel ? parallel : sequential;
    int expected_count = use_parallel ? (int)(sizeof(parallel) / sizeof(parallel[0]))
                                      : (int)(sizeof(sequential) / sizeof(sequential[0]));
    if (!steps || !plan_buf || plan_bufsize == 0 || step_count != expected_count) {
        return false;
    }

    uint64_t seen = 0;
    size_t pos = 0;
    plan_buf[0] = '\0';
    for (int i = 0; i < expected_count; i++) {
        if (steps[i].kind != expected[i].kind ||
            steps[i].phase != CBM_RS_PLAN_PHASE_INCREMENTAL_EXTRACT_RESOLVE ||
            steps[i].policy != CBM_RS_PLAN_POLICY_IGNORE_ERR ||
            steps[i].gate_flags != expected[i].gate_flags || steps[i].requires_mask != seen ||
            steps[i].effect_flags != CBM_RS_PLAN_EFFECT_MUTATES_GRAPH) {
            return false;
        }
        if (!append_plan_name(plan_buf, plan_bufsize, &pos, expected[i].name)) {
            return false;
        }
        seen |= rust_plan_step_bit(expected[i].kind);
    }
    return true;
}

typedef struct {
    cbm_pipeline_ctx_t *ctx;
    cbm_file_info_t *changed_files;
    int changed_count;
    int worker_count;
    _Atomic int64_t shared_ids;
    CBMFileResult **cache;
} incr_extract_dispatch_t;

static bool incr_extract_prepare_cache(incr_extract_dispatch_t *dispatch) {
    if (dispatch->cache) {
        return true;
    }
    dispatch->cache =
        (CBMFileResult **)calloc((size_t)dispatch->changed_count, sizeof(CBMFileResult *));
    if (!dispatch->cache) {
        return false;
    }
    atomic_init(&dispatch->shared_ids, cbm_gbuf_next_id(dispatch->ctx->gbuf));
    return true;
}

static void incr_extract_free_cache(incr_extract_dispatch_t *dispatch) {
    if (!dispatch->cache) {
        return;
    }
    for (int j = 0; j < dispatch->changed_count; j++) {
        if (dispatch->cache[j]) {
            cbm_free_result(dispatch->cache[j]);
        }
    }
    free(dispatch->cache);
    dispatch->cache = NULL;
}

static bool run_rust_incremental_extract_step(int kind, incr_extract_dispatch_t *dispatch,
                                              int *out_rc) {
    struct timespec t;
    if (out_rc) {
        *out_rc = 0;
    }
    switch (kind) {
    case CBM_RS_PLAN_STEP_INCREMENTAL_DEFINITIONS:
        cbm_pipeline_pass_definitions(dispatch->ctx, dispatch->changed_files,
                                      dispatch->changed_count);
        return true;
    case CBM_RS_PLAN_STEP_INCREMENTAL_CALLS:
        cbm_pipeline_pass_calls(dispatch->ctx, dispatch->changed_files, dispatch->changed_count);
        return true;
    case CBM_RS_PLAN_STEP_INCREMENTAL_USAGES:
        cbm_pipeline_pass_usages(dispatch->ctx, dispatch->changed_files, dispatch->changed_count);
        return true;
    case CBM_RS_PLAN_STEP_INCREMENTAL_SEMANTIC:
        cbm_pipeline_pass_semantic(dispatch->ctx, dispatch->changed_files, dispatch->changed_count);
        return true;
    case CBM_RS_PLAN_STEP_INCREMENTAL_PARALLEL_EXTRACT:
        if (!incr_extract_prepare_cache(dispatch)) {
            return false;
        }
        cbm_clock_gettime(CLOCK_MONOTONIC, &t);
        cbm_parallel_extract(dispatch->ctx, dispatch->changed_files, dispatch->changed_count,
                             dispatch->cache, &dispatch->shared_ids, dispatch->worker_count);
        cbm_gbuf_set_next_id(dispatch->ctx->gbuf, atomic_load(&dispatch->shared_ids));
        cbm_log_info("pass.timing", "pass", "incr_extract", "elapsed_ms",
                     itoa_buf((int)elapsed_ms(t)));
        return true;
    case CBM_RS_PLAN_STEP_INCREMENTAL_REGISTRY:
        if (!dispatch->cache) {
            return false;
        }
        cbm_clock_gettime(CLOCK_MONOTONIC, &t);
        cbm_build_registry_from_cache(dispatch->ctx, dispatch->changed_files,
                                      dispatch->changed_count, dispatch->cache);
        cbm_log_info("pass.timing", "pass", "incr_registry", "elapsed_ms",
                     itoa_buf((int)elapsed_ms(t)));
        return true;
    case CBM_RS_PLAN_STEP_INCREMENTAL_RESOLVE:
        if (!dispatch->cache) {
            if (out_rc) {
                *out_rc = CBM_NOT_FOUND;
            }
            return false;
        }
        cbm_clock_gettime(CLOCK_MONOTONIC, &t);
        cbm_log_info("incremental.lsp_cross", "mode", "skipped", "reason", "no_cross_lsp_prebuild");
        int rc =
            cbm_parallel_resolve(dispatch->ctx, dispatch->changed_files, dispatch->changed_count,
                                 dispatch->cache, &dispatch->shared_ids, dispatch->worker_count,
                                 NULL, 0, NULL, NULL /* module_def_index */,
                                 NULL /* incremental 會跳過 Tier 2 cross registry prebuild */);
        cbm_gbuf_set_next_id(dispatch->ctx->gbuf, atomic_load(&dispatch->shared_ids));
        cbm_log_info("pass.timing", "pass", "incr_resolve", "elapsed_ms",
                     itoa_buf((int)elapsed_ms(t)));
        if (rc != 0 || (dispatch->ctx->cancelled && atomic_load(dispatch->ctx->cancelled) != 0)) {
            if (out_rc) {
                *out_rc = rc != 0 ? rc : CBM_NOT_FOUND;
            }
            return false;
        }
        return true;
    default:
        if (out_rc) {
            *out_rc = CBM_NOT_FOUND;
        }
        return false;
    }
}

static int run_rust_incremental_extract_dispatch(cbm_pipeline_ctx_t *ctx,
                                                 cbm_file_info_t *changed_files, int ci,
                                                 bool use_parallel, int worker_count) {
    cbm_rs_pipeline_plan_step_v2_t rust_steps[CBM_RS_PLAN_V2_MAX_STEPS];
    int rust_step_count = 0;
    char rust_plan[CBM_RS_PLAN_INCREMENTAL_POST_BUF];
    if (!cbm_rust_plan_steps_v2(CBM_RS_PLAN_INCREMENTAL_EXTRACT_RESOLVE, ctx->mode, worker_count,
                                ci, rust_steps, CBM_RS_PLAN_V2_MAX_STEPS, &rust_step_count) ||
        !decode_rust_incremental_extract_steps(use_parallel, rust_steps, rust_step_count, rust_plan,
                                               sizeof(rust_plan))) {
        return CBM_NOT_FOUND;
    }

    cbm_log_info("rust_plan.dispatch", "phase", "incremental_extract_resolve", "passes",
                 itoa_buf(rust_step_count), "plan", rust_plan, "source", "typed_v2");
    if (use_parallel) {
        cbm_log_info("incremental.mode", "mode", "parallel", "workers", itoa_buf(worker_count),
                     "changed", itoa_buf(ci));
    } else {
        cbm_log_info("incremental.mode", "mode", "sequential", "changed", itoa_buf(ci));
    }

    incr_extract_dispatch_t dispatch = {
        .ctx = ctx,
        .changed_files = changed_files,
        .changed_count = ci,
        .worker_count = worker_count,
        .cache = NULL,
    };
    int rc = 0;
    for (int i = 0; i < rust_step_count; i++) {
        if (!run_rust_incremental_extract_step(rust_steps[i].kind, &dispatch, &rc)) {
            incr_extract_free_cache(&dispatch);
            return rc != 0 ? rc : CBM_NOT_FOUND;
        }
    }
    incr_extract_free_cache(&dispatch);
    return 0;
}
#endif

static void run_sequential_incremental_extract_resolve(cbm_pipeline_ctx_t *ctx,
                                                       cbm_file_info_t *changed_files, int ci) {
    cbm_log_info("incremental.mode", "mode", "sequential", "changed", itoa_buf(ci));
    cbm_pipeline_pass_definitions(ctx, changed_files, ci);
    cbm_pipeline_pass_calls(ctx, changed_files, ci);
    cbm_pipeline_pass_usages(ctx, changed_files, ci);
    cbm_pipeline_pass_semantic(ctx, changed_files, ci);
}

static void run_c_incremental_extract_resolve(cbm_pipeline_ctx_t *ctx,
                                              cbm_file_info_t *changed_files, int ci,
                                              bool use_parallel, int worker_count) {
    struct timespec t;

    if (use_parallel) {
        cbm_log_info("incremental.mode", "mode", "parallel", "workers", itoa_buf(worker_count),
                     "changed", itoa_buf(ci));

        _Atomic int64_t shared_ids;
        atomic_init(&shared_ids, cbm_gbuf_next_id(ctx->gbuf));

        CBMFileResult **cache = (CBMFileResult **)calloc(ci, sizeof(CBMFileResult *));
        if (cache) {
            cbm_clock_gettime(CLOCK_MONOTONIC, &t);
            cbm_parallel_extract(ctx, changed_files, ci, cache, &shared_ids, worker_count);
            cbm_gbuf_set_next_id(ctx->gbuf, atomic_load(&shared_ids));
            cbm_log_info("pass.timing", "pass", "incr_extract", "elapsed_ms",
                         itoa_buf((int)elapsed_ms(t)));

            cbm_clock_gettime(CLOCK_MONOTONIC, &t);
            cbm_build_registry_from_cache(ctx, changed_files, ci, cache);
            cbm_log_info("pass.timing", "pass", "incr_registry", "elapsed_ms",
                         itoa_buf((int)elapsed_ms(t)));

            /* Incremental 不建立 cross-file LSP 前置資料，因為它需要完整專案的
             * all_defs，而不是只有 changed slice。per-file LSP 仍會在
             * cbm_extract_file 內執行；cross-file resolution 延後到下一次 full re-index。
             * 傳入 NULL/0/NULL 讓 resolve_worker 的 fused step 成為 no-op。 */
            cbm_clock_gettime(CLOCK_MONOTONIC, &t);
            cbm_parallel_resolve(ctx, changed_files, ci, cache, &shared_ids, worker_count, NULL, 0,
                                 NULL, NULL /* module_def_index */,
                                 NULL /* incremental 會跳過 Tier 2 cross registry prebuild */);
            cbm_gbuf_set_next_id(ctx->gbuf, atomic_load(&shared_ids));
            cbm_log_info("pass.timing", "pass", "incr_resolve", "elapsed_ms",
                         itoa_buf((int)elapsed_ms(t)));

            for (int j = 0; j < ci; j++) {
                if (cache[j]) {
                    cbm_free_result(cache[j]);
                }
            }
            free(cache);
        } else {
            cbm_log_warn("incremental.parallel_fallback", "reason", "cache_alloc_failed", "path",
                         "sequential");
            run_sequential_incremental_extract_resolve(ctx, changed_files, ci);
        }
    } else {
        run_sequential_incremental_extract_resolve(ctx, changed_files, ci);
    }
}

/* 對 changed files 執行 parallel 或 sequential extract+resolve。 */
static int run_extract_resolve(cbm_pipeline_ctx_t *ctx, cbm_file_info_t *changed_files, int ci) {
    /* per-file LSP 在所有模式都會執行；incremental 一律停用 cross-file LSP，
     * 下方會以 NULL cross_registries 呼叫 cbm_parallel_resolve。 */

#define MIN_FILES_FOR_PARALLEL_INCR 50
    int worker_count = cbm_default_worker_count(true);
    bool c_use_parallel = (worker_count > SKIP_ONE && ci > MIN_FILES_FOR_PARALLEL_INCR);
#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
    bool rust_use_parallel = false;
    if (cbm_rust_plan_extracts_parallel(ctx->mode, worker_count, ci, &rust_use_parallel)) {
        int rc = run_rust_incremental_extract_dispatch(ctx, changed_files, ci, rust_use_parallel,
                                                       worker_count);
        if (rc == 0) {
            return 0;
        }
        if (rc != CBM_NOT_FOUND) {
            return rc;
        }
#if defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
        cbm_log_error("rust_plan.err", "phase", "incremental_extract_resolve", "path",
                      "rust_plan_only");
        return CBM_NOT_FOUND;
#else
        cbm_log_warn("rust_plan.fallback", "phase", "incremental_extract_resolve", "reason",
                     "typed_v2_unavailable", "path", "c_heuristic");
#endif
    } else {
#if defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
        cbm_log_error("rust_plan.err", "phase", "incremental_extract_resolve", "path",
                      "rust_plan_only");
        return CBM_NOT_FOUND;
#else
        cbm_log_warn("rust_plan.fallback", "phase", "incremental_extract_resolve", "reason",
                     "choice_unavailable", "path", "c_heuristic");
#endif
    }
#endif
    run_c_incremental_extract_resolve(ctx, changed_files, ci, c_use_parallel, worker_count);
    return 0;
}

#if !defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
/* Run post-extraction passes (tests, decorator tags, configlink). */
static void run_postpasses(cbm_pipeline_ctx_t *ctx, cbm_file_info_t *changed_files, int ci,
                           const char *project) {
    struct timespec t;

    cbm_clock_gettime(CLOCK_MONOTONIC, &t);
    cbm_pipeline_pass_tests(ctx, changed_files, ci);
    cbm_log_info("pass.timing", "pass", "incr_tests", "elapsed_ms", itoa_buf((int)elapsed_ms(t)));

    cbm_clock_gettime(CLOCK_MONOTONIC, &t);
    cbm_pipeline_pass_decorator_tags(ctx->gbuf, project);
    cbm_log_info("pass.timing", "pass", "incr_decorator_tags", "elapsed_ms",
                 itoa_buf((int)elapsed_ms(t)));

    cbm_clock_gettime(CLOCK_MONOTONIC, &t);
    cbm_pipeline_pass_configlink(ctx);
    cbm_log_info("pass.timing", "pass", "incr_configlink", "elapsed_ms",
                 itoa_buf((int)elapsed_ms(t)));

    /* SIMILAR_TO + SEMANTICALLY_RELATED edges only in moderate/full modes */
    if (ctx->mode <= CBM_MODE_MODERATE) {
        cbm_clock_gettime(CLOCK_MONOTONIC, &t);
        cbm_pipeline_pass_similarity(ctx);
        cbm_log_info("pass.timing", "pass", "incr_similarity", "elapsed_ms",
                     itoa_buf((int)elapsed_ms(t)));

        cbm_clock_gettime(CLOCK_MONOTONIC, &t);
        cbm_pipeline_pass_semantic_edges(ctx);
        cbm_log_info("pass.timing", "pass", "incr_semantic_edges", "elapsed_ms",
                     itoa_buf((int)elapsed_ms(t)));
    }
}
#endif

static void run_incremental_edge_relink(cbm_gbuf_t *gbuf, cbm_edge_capture_t *edge_cap) {
    struct timespec t;
    cbm_clock_gettime(CLOCK_MONOTONIC, &t);
    int relinked = incr_restore_inbound_edges(gbuf, edge_cap);
    cbm_log_info("incremental.edge_relink", "relinked", itoa_buf(relinked), "captured",
                 itoa_buf(edge_cap->count), "elapsed_ms", itoa_buf((int)elapsed_ms(t)));
}

typedef struct {
    cbm_pipeline_ctx_t *ctx;
    cbm_file_info_t *changed_files;
    int changed_count;
    const char *project;
    cbm_pipeline_t *pipeline;
    cbm_gbuf_t *gbuf;
    cbm_edge_capture_t *edge_cap;
    const char *db_path;
    cbm_file_info_t *all_files;
    int all_file_count;
    const cbm_file_hash_t *mode_skipped;
    int mode_skipped_count;
    const char *repo_path;
    int status;
} incr_post_dispatch_t;

static int g_test_next_incremental_dump_rc = 0;

void cbm_pipeline_test_fail_next_incremental_dump(int rc) {
    g_test_next_incremental_dump_rc = rc;
}

static int incremental_dump_to_sqlite(cbm_gbuf_t *gbuf, const char *db_path) {
    if (g_test_next_incremental_dump_rc != 0) {
        int rc = g_test_next_incremental_dump_rc;
        g_test_next_incremental_dump_rc = 0;
        return rc;
    }
    return cbm_gbuf_dump_to_sqlite(gbuf, db_path);
}

static bool incr_format_sidecar_path(char *out, size_t out_size, const char *path,
                                     const char *suffix) {
    int n = snprintf(out, out_size, "%s%s", path, suffix);
    return n >= 0 && (size_t)n < out_size;
}

static void incr_unlink_sqlite_sidecars(const char *db_path) {
    char wal[INCR_WAL_BUF];
    char shm[INCR_WAL_BUF];
    if (incr_format_sidecar_path(wal, sizeof(wal), db_path, "-wal")) {
        cbm_unlink(wal);
    }
    if (incr_format_sidecar_path(shm, sizeof(shm), db_path, "-shm")) {
        cbm_unlink(shm);
    }
}

static void incr_unlink_dump_target(const char *path) {
    cbm_unlink(path);
    incr_unlink_sqlite_sidecars(path);
}

static int incr_replace_db_with_temp(const char *tmp_path, const char *db_path) {
#ifdef _WIN32
    if (!MoveFileExA(tmp_path, db_path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return CBM_STORE_ERR;
    }
    return 0;
#else
    return rename(tmp_path, db_path);
#endif
}

#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
typedef void (*incr_post_pass_fn)(incr_post_dispatch_t *);

typedef struct {
    incr_post_pass_fn fn;
    const char *name;
    int kind;
    int policy;
    bool moderate_only;
} incr_post_pass_def_t;

static void incr_post_k8s(incr_post_dispatch_t *dispatch) {
    cbm_pipeline_pass_k8s(dispatch->ctx, dispatch->changed_files, dispatch->changed_count);
}

static void incr_post_tests(incr_post_dispatch_t *dispatch) {
    cbm_pipeline_pass_tests(dispatch->ctx, dispatch->changed_files, dispatch->changed_count);
}

static void incr_post_decorator_tags(incr_post_dispatch_t *dispatch) {
    cbm_pipeline_pass_decorator_tags(dispatch->ctx->gbuf, dispatch->project);
}

static void incr_post_configlink(incr_post_dispatch_t *dispatch) {
    cbm_pipeline_pass_configlink(dispatch->ctx);
}

static void incr_post_similarity(incr_post_dispatch_t *dispatch) {
    cbm_pipeline_pass_similarity(dispatch->ctx);
}

static void incr_post_semantic_edges(incr_post_dispatch_t *dispatch) {
    cbm_pipeline_pass_semantic_edges(dispatch->ctx);
}
#endif

static void incr_post_edge_relink(incr_post_dispatch_t *dispatch) {
    run_incremental_edge_relink(dispatch->gbuf, dispatch->edge_cap);
}

static void incr_post_incremental_dump(incr_post_dispatch_t *dispatch) {
    struct timespec t;
    cbm_clock_gettime(CLOCK_MONOTONIC, &t);

    cbm_pipeline_set_committed_counts(dispatch->pipeline, cbm_gbuf_node_count(dispatch->gbuf),
                                      cbm_gbuf_edge_count(dispatch->gbuf));

    char tmp_path[INCR_WAL_BUF];
    int dump_rc = CBM_STORE_ERR;
    if (incr_format_sidecar_path(tmp_path, sizeof(tmp_path), dispatch->db_path, ".tmp")) {
        incr_unlink_dump_target(tmp_path);
        dump_rc = incremental_dump_to_sqlite(dispatch->gbuf, tmp_path);
        if (dump_rc == 0) {
            if (incr_replace_db_with_temp(tmp_path, dispatch->db_path) == 0) {
                incr_unlink_sqlite_sidecars(dispatch->db_path);
            } else {
                dump_rc = CBM_STORE_ERR;
                cbm_log_error("pipeline.err", "phase", "incremental_replace", "rc",
                              itoa_buf(dump_rc));
                incr_unlink_dump_target(tmp_path);
            }
        } else {
            incr_unlink_dump_target(tmp_path);
        }
    } else {
        cbm_log_error("pipeline.err", "phase", "incremental_dump", "err", "tmp_path_too_long");
    }
    cbm_log_info("incremental.dump", "rc", itoa_buf(dump_rc), "elapsed_ms",
                 itoa_buf((int)elapsed_ms(t)));
    if (dump_rc != 0 && dispatch->status == 0) {
        cbm_log_error("pipeline.err", "phase", "incremental_dump", "rc", itoa_buf(dump_rc));
        dispatch->status = dump_rc;
    }
}

static void incr_post_persist_hashes(incr_post_dispatch_t *dispatch) {
    cbm_store_t *hash_store = cbm_store_open_path(dispatch->db_path);
    if (!hash_store) {
        cbm_log_warn("incremental.persist_hash_open_failed", "db", dispatch->db_path);
        if (dispatch->status == 0) {
            dispatch->status = CBM_STORE_ERR;
        }
        return;
    }
    persist_hashes(hash_store, dispatch->project, dispatch->all_files, dispatch->all_file_count,
                   dispatch->mode_skipped, dispatch->mode_skipped_count);

    /* FTS5 rebuild after incremental dump.  The btree dump path bypasses
     * any triggers that could have kept nodes_fts synchronized, so we
     * rebuild from the nodes table here.  See the full-dump path in
     * pipeline.c for the matching logic. */
    int fts_delete_rc =
        cbm_store_exec(hash_store, "INSERT INTO nodes_fts(nodes_fts) VALUES('delete-all');");
    int fts_insert_rc = cbm_store_exec(
        hash_store, "INSERT INTO nodes_fts(rowid, name, qualified_name, label, file_path) "
                    "SELECT id, cbm_camel_split(name), qualified_name, label, file_path "
                    "FROM nodes;");
    if (fts_insert_rc != CBM_STORE_OK) {
        fts_insert_rc = cbm_store_exec(
            hash_store, "INSERT INTO nodes_fts(rowid, name, qualified_name, label, file_path) "
                        "SELECT id, name, qualified_name, label, file_path FROM nodes;");
    }
    if ((fts_delete_rc != CBM_STORE_OK || fts_insert_rc != CBM_STORE_OK) && dispatch->status == 0) {
        cbm_log_error("pipeline.err", "phase", "incremental_persist_hashes", "rc",
                      itoa_buf(fts_delete_rc != CBM_STORE_OK ? fts_delete_rc : fts_insert_rc));
        dispatch->status = fts_delete_rc != CBM_STORE_OK ? fts_delete_rc : fts_insert_rc;
    }

    cbm_store_close(hash_store);
}

static void incr_post_artifact_export(incr_post_dispatch_t *dispatch) {
    if (!dispatch->repo_path) {
        return;
    }
    bool persistence = cbm_pipeline_persistence_enabled(dispatch->pipeline);
    bool existing_artifact = cbm_artifact_exists(dispatch->repo_path);
    if (persistence || existing_artifact) {
        int quality = persistence ? CBM_ARTIFACT_BEST : CBM_ARTIFACT_FAST;
        int rc =
            cbm_artifact_export(dispatch->db_path, dispatch->repo_path, dispatch->project, quality);
        if (persistence && rc != 0 && dispatch->status == 0) {
            const char *err = cbm_artifact_export_last_error();
            cbm_log_error("pipeline.err", "phase", "artifact_export", "err", err ? err : "unknown");
            dispatch->status = rc;
        }
    }
}

static bool run_incremental_tail_step(int kind, incr_post_dispatch_t *dispatch) {
    if (dispatch->status != 0 &&
        (kind == CBM_RS_INCR_POST_PERSIST_HASHES || kind == CBM_RS_INCR_POST_ARTIFACT_EXPORT)) {
        cbm_log_warn("incremental.tail_skip", "kind", itoa_buf(kind), "reason", "prior_error");
        return true;
    }
    switch (kind) {
    case CBM_RS_INCR_POST_EDGE_RELINK:
        incr_post_edge_relink(dispatch);
        return true;
    case CBM_RS_INCR_POST_INCREMENTAL_DUMP:
        incr_post_incremental_dump(dispatch);
        return true;
    case CBM_RS_INCR_POST_PERSIST_HASHES:
        incr_post_persist_hashes(dispatch);
        return true;
    case CBM_RS_INCR_POST_ARTIFACT_EXPORT:
        incr_post_artifact_export(dispatch);
        return true;
    default:
        return false;
    }
}

static void run_incremental_tail_fixed(incr_post_dispatch_t *dispatch) {
    run_incremental_tail_step(CBM_RS_INCR_POST_EDGE_RELINK, dispatch);
    run_incremental_tail_step(CBM_RS_INCR_POST_INCREMENTAL_DUMP, dispatch);
    run_incremental_tail_step(CBM_RS_INCR_POST_PERSIST_HASHES, dispatch);
    run_incremental_tail_step(CBM_RS_INCR_POST_ARTIFACT_EXPORT, dispatch);
}

#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
static const incr_post_pass_def_t incr_post_passes[] = {
    {incr_post_k8s, "k8s", CBM_RS_INCR_POST_K8S, CBM_RS_PLAN_POLICY_IGNORE_ERR, false},
    {incr_post_tests, "incr_tests", CBM_RS_INCR_POST_TESTS, CBM_RS_PLAN_POLICY_IGNORE_ERR, false},
    {incr_post_decorator_tags, "incr_decorator_tags", CBM_RS_INCR_POST_DECORATOR_TAGS,
     CBM_RS_PLAN_POLICY_IGNORE_ERR, false},
    {incr_post_configlink, "incr_configlink", CBM_RS_INCR_POST_CONFIGLINK,
     CBM_RS_PLAN_POLICY_IGNORE_ERR, false},
    {incr_post_similarity, "incr_similarity", CBM_RS_INCR_POST_SIMILARITY,
     CBM_RS_PLAN_POLICY_IGNORE_ERR, true},
    {incr_post_semantic_edges, "incr_semantic_edges", CBM_RS_INCR_POST_SEMANTIC_EDGES,
     CBM_RS_PLAN_POLICY_IGNORE_ERR, true},
    {incr_post_edge_relink, "edge_relink", CBM_RS_INCR_POST_EDGE_RELINK,
     CBM_RS_PLAN_POLICY_BEST_EFFORT, false},
    {incr_post_incremental_dump, "incremental_dump", CBM_RS_INCR_POST_INCREMENTAL_DUMP,
     CBM_RS_PLAN_POLICY_BEST_EFFORT, false},
    {incr_post_persist_hashes, "persist_hashes", CBM_RS_INCR_POST_PERSIST_HASHES,
     CBM_RS_PLAN_POLICY_BEST_EFFORT, false},
    {incr_post_artifact_export, "artifact_export", CBM_RS_INCR_POST_ARTIFACT_EXPORT,
     CBM_RS_PLAN_POLICY_OPTIONAL_EXISTING_ARTIFACT, false},
};
enum { INCR_POST_PASS_COUNT = (int)(sizeof(incr_post_passes) / sizeof(incr_post_passes[0])) };

static int find_incr_post_pass(int kind) {
    for (int i = 0; i < INCR_POST_PASS_COUNT; i++) {
        if (incr_post_passes[i].kind == kind) {
            return i;
        }
    }
    return -1;
}

static int expected_incr_post_prefix_count(int mode) {
    int prefix_all = INCR_POST_PASS_COUNT - 4;
    return mode <= CBM_MODE_MODERATE ? prefix_all : prefix_all - PAIR_LEN;
}

static bool incr_post_step_eq(cbm_rs_pipeline_plan_step_t step, int kind, int policy) {
    return step.kind == kind && step.policy == policy;
}

static bool decode_rust_incremental_post_steps(const cbm_rs_pipeline_plan_step_t *steps,
                                               int step_count, int mode,
                                               const incr_post_pass_def_t **ordered, int *out_count,
                                               bool *out_edge_relink) {
    bool seen[INCR_POST_PASS_COUNT] = {false};
    int expected_prefix = expected_incr_post_prefix_count(mode);
    int expected_total = expected_prefix + 4;
    if (!steps || !ordered || !out_count || !out_edge_relink || step_count != expected_total) {
        return false;
    }
    *out_count = 0;
    *out_edge_relink = false;

    for (int i = 0; i < expected_prefix; i++) {
        int pass_idx = find_incr_post_pass(steps[i].kind);
        if (pass_idx != i || seen[pass_idx]) {
            return false;
        }
        const incr_post_pass_def_t *pass = &incr_post_passes[pass_idx];
        if (steps[i].policy != pass->policy || (pass->moderate_only && mode > CBM_MODE_MODERATE)) {
            return false;
        }
        seen[pass_idx] = true;
        ordered[(*out_count)++] = pass;
    }

    if (!incr_post_step_eq(steps[expected_prefix], CBM_RS_INCR_POST_EDGE_RELINK,
                           CBM_RS_PLAN_POLICY_BEST_EFFORT) ||
        !incr_post_step_eq(steps[expected_prefix + 1], CBM_RS_INCR_POST_INCREMENTAL_DUMP,
                           CBM_RS_PLAN_POLICY_BEST_EFFORT) ||
        !incr_post_step_eq(steps[expected_prefix + 2], CBM_RS_INCR_POST_PERSIST_HASHES,
                           CBM_RS_PLAN_POLICY_BEST_EFFORT) ||
        !incr_post_step_eq(steps[expected_prefix + 3], CBM_RS_INCR_POST_ARTIFACT_EXPORT,
                           CBM_RS_PLAN_POLICY_OPTIONAL_EXISTING_ARTIFACT)) {
        return false;
    }

    *out_edge_relink = true;
    return *out_count == expected_prefix;
}

static void run_incremental_post_pass(const incr_post_pass_def_t *pass,
                                      incr_post_dispatch_t *dispatch) {
    struct timespec t;
    cbm_clock_gettime(CLOCK_MONOTONIC, &t);
    pass->fn(dispatch);
    cbm_log_info("pass.timing", "pass", pass->name, "elapsed_ms", itoa_buf((int)elapsed_ms(t)));
}

static bool run_rust_incremental_postpasses(cbm_pipeline_ctx_t *ctx,
                                            incr_post_dispatch_t *dispatch) {
    cbm_rs_pipeline_plan_step_t steps[CBM_RS_INCR_POST_MAX_STEPS];
    const incr_post_pass_def_t *ordered[INCR_POST_PASS_COUNT];
    int step_count = 0;
    int count = 0;
    bool edge_relink = false;
    if (!cbm_rust_plan_incremental_post_steps(ctx->mode, steps, CBM_RS_INCR_POST_MAX_STEPS,
                                              &step_count) ||
        !decode_rust_incremental_post_steps(steps, step_count, ctx->mode, ordered, &count,
                                            &edge_relink)) {
        return false;
    }

    cbm_log_info(
        "rust_plan.dispatch", "phase", "incremental_post", "passes", itoa_buf(step_count), "tail",
        edge_relink ? "edge_relink,incremental_dump,persist_hashes,artifact_export" : "none",
        "source", "typed_steps");
    for (int i = 0; i < count; i++) {
        run_incremental_post_pass(ordered[i], dispatch);
    }
    for (int i = count; i < step_count; i++) {
        if (!run_incremental_tail_step(steps[i].kind, dispatch)) {
            return false;
        }
    }
    return true;
}
#endif

/* ── Incremental pipeline entry point ────────────────────────────── */

int cbm_pipeline_run_incremental(cbm_pipeline_t *p, const char *db_path, cbm_file_info_t *files,
                                 int file_count) {
    struct timespec t0;
    cbm_clock_gettime(CLOCK_MONOTONIC, &t0);

    const char *project = cbm_pipeline_project_name(p);

    /* Open existing disk DB */
    cbm_store_t *store = cbm_store_open_path(db_path);
    if (!store) {
        cbm_log_error("incremental.err", "msg", "open_db_failed", "path", db_path);
        return CBM_NOT_FOUND;
    }

    /* Load stored file hashes */
    cbm_file_hash_t *stored = NULL;
    int stored_count = 0;
    cbm_store_get_file_hashes(store, project, &stored, &stored_count);

    /* Classify files */
    int n_changed = 0;
    int n_unchanged = 0;
    bool *is_changed =
        classify_files(files, file_count, stored, stored_count, &n_changed, &n_unchanged);

    /* Classify stored files absent from current discovery: truly-deleted
     * (purge) vs mode-skipped (preserve nodes AND hash rows). */
    char **deleted = NULL;
    cbm_file_hash_t *mode_skipped = NULL;
    int mode_skipped_count = 0;
    int deleted_count =
        find_deleted_files(cbm_pipeline_repo_path(p), files, file_count, stored, stored_count,
                           &deleted, &mode_skipped, &mode_skipped_count);

    cbm_log_info("incremental.classify", "changed", itoa_buf(n_changed), "unchanged",
                 itoa_buf(n_unchanged), "deleted", itoa_buf(deleted_count), "mode_skipped",
                 itoa_buf(mode_skipped_count));

    /* Fast path: nothing changed → skip. The on-disk DB is left untouched,
     * which means existing hash rows (including for any mode-skipped files
     * that were already preserved by an earlier run) remain intact. */
    if (n_changed == 0 && deleted_count == 0) {
        cbm_log_info("incremental.noop", "reason", "no_changes");
        free(is_changed);
        free(deleted);
        free_mode_skipped(mode_skipped, mode_skipped_count);
        cbm_store_free_file_hashes(stored, stored_count);
        cbm_store_close(store);
        return 0;
    }

    cbm_store_free_file_hashes(stored, stored_count);

    /* Build list of changed files */
    cbm_file_info_t *changed_files =
        (n_changed > 0) ? malloc((size_t)n_changed * sizeof(cbm_file_info_t)) : NULL;
    int ci = 0;
    for (int i = 0; i < file_count; i++) {
        if (is_changed[i]) {
            changed_files[ci++] = files[i];
        }
    }
    free(is_changed);

    cbm_log_info("incremental.reparse", "files", itoa_buf(ci));

    struct timespec t;

    /* Step 1: Load existing graph into RAM */
    cbm_clock_gettime(CLOCK_MONOTONIC, &t);
    cbm_gbuf_t *existing = cbm_gbuf_new(project, cbm_pipeline_repo_path(p));
    int load_rc = cbm_gbuf_load_from_db(existing, db_path, project);
    cbm_log_info("incremental.load_db", "rc", itoa_buf(load_rc), "nodes",
                 itoa_buf(cbm_gbuf_node_count(existing)), "edges",
                 itoa_buf(cbm_gbuf_edge_count(existing)), "elapsed_ms",
                 itoa_buf((int)elapsed_ms(t)));

    if (load_rc != 0) {
        cbm_log_error("incremental.err", "msg", "load_db_failed");
        cbm_gbuf_free(existing);
        free(changed_files);
        for (int i = 0; i < deleted_count; i++) {
            free(deleted[i]);
        }
        free(deleted);
        free_mode_skipped(mode_skipped, mode_skipped_count);
        cbm_store_close(store);
        return CBM_NOT_FOUND;
    }

    cbm_store_close(store);

    /* Snapshot inbound cross-file edges into changed files BEFORE purging, so
     * the cascade delete doesn't permanently drop edges whose source lives in
     * an unchanged (never-re-parsed) file. Re-linked after re-resolution. */
    cbm_edge_capture_t edge_cap = {0};
    edge_cap.gbuf = existing;
    {
        CBMHashTable *changed_paths = cbm_ht_create(ci > 0 ? (size_t)ci * PAIR_LEN : CBM_SZ_64);
        for (int i = 0; i < ci; i++) {
            cbm_ht_set(changed_paths, changed_files[i].rel_path, &changed_files[i]);
        }
        edge_cap.changed_paths = changed_paths;
        cbm_clock_gettime(CLOCK_MONOTONIC, &t);
        cbm_gbuf_foreach_edge(existing, incr_capture_inbound_edge, &edge_cap);
        edge_cap.changed_paths = NULL;
        cbm_ht_free(changed_paths); /* keys borrowed from changed_files; not freed here */
    }
    cbm_log_info("incremental.edge_snapshot", "captured", itoa_buf(edge_cap.count), "elapsed_ms",
                 itoa_buf((int)elapsed_ms(t)));

    /* Step 2: Purge stale nodes */
    cbm_clock_gettime(CLOCK_MONOTONIC, &t);
    for (int i = 0; i < ci; i++) {
        cbm_gbuf_apply_delete_by_file(existing, changed_files[i].rel_path);
    }
    for (int i = 0; i < deleted_count; i++) {
        cbm_gbuf_apply_delete_by_file(existing, deleted[i]);
        free(deleted[i]);
    }
    free(deleted);
    cbm_log_info("incremental.purge", "elapsed_ms", itoa_buf((int)elapsed_ms(t)));

    /* Step 3-5: Registry + extract + resolve */
    cbm_registry_t *registry = cbm_registry_new();
    cbm_clock_gettime(CLOCK_MONOTONIC, &t);
    cbm_gbuf_foreach_node(existing, registry_visitor, registry);
    cbm_log_info("incremental.registry_seed", "symbols", itoa_buf(cbm_registry_size(registry)),
                 "elapsed_ms", itoa_buf((int)elapsed_ms(t)));

    cbm_path_alias_collection_t *path_aliases = cbm_load_path_aliases(cbm_pipeline_repo_path(p));

    cbm_pipeline_ctx_t ctx = {
        .project_name = project,
        .repo_path = cbm_pipeline_repo_path(p),
        .gbuf = existing,
        .registry = registry,
        .cancelled = cbm_pipeline_cancelled_ptr(p),
        .mode = cbm_pipeline_get_mode(p),
        .path_aliases = path_aliases,
    };

    for (int i = 0; i < ci; i++) {
        char *file_qn = cbm_pipeline_fqn_compute(project, changed_files[i].rel_path, "__file__");
        if (file_qn) {
            cbm_gbuf_apply_upsert_node(existing, "File", changed_files[i].rel_path, file_qn,
                                       changed_files[i].rel_path, 0, 0, "{}");
            free(file_qn);
        }
    }

    (void)run_extract_resolve(&ctx, changed_files, ci);
    incr_post_dispatch_t tail_dispatch = {
        .ctx = &ctx,
        .changed_files = changed_files,
        .changed_count = ci,
        .project = project,
        .pipeline = p,
        .gbuf = existing,
        .edge_cap = &edge_cap,
        .db_path = db_path,
        .all_files = files,
        .all_file_count = file_count,
        .mode_skipped = mode_skipped,
        .mode_skipped_count = mode_skipped_count,
        .repo_path = cbm_pipeline_repo_path(p),
        .status = 0,
    };
    bool tail_done = false;
#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
    if (run_rust_incremental_postpasses(&ctx, &tail_dispatch)) {
        tail_done = true;
    } else {
#if defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
        cbm_log_error("rust_plan.err", "phase", "incremental_post", "path", "rust_plan_only");
        tail_dispatch.status = CBM_NOT_FOUND;
        tail_done = true;
#else
        cbm_log_warn("rust_plan.fallback", "phase", "incremental_post", "reason",
                     "typed_steps_unavailable", "path", "c_postpasses");
        cbm_pipeline_pass_k8s(&ctx, changed_files, ci);
        run_postpasses(&ctx, changed_files, ci, project);
#endif
    }
#else
    cbm_pipeline_pass_k8s(&ctx, changed_files, ci);
    run_postpasses(&ctx, changed_files, ci, project);
#endif

    free(changed_files);
    cbm_registry_free(registry);
    cbm_path_alias_collection_free(path_aliases);

    if (!tail_done) {
        run_incremental_tail_fixed(&tail_dispatch);
    }
    incr_free_edge_capture(&edge_cap);

    int tail_status = tail_dispatch.status;
    free_mode_skipped(mode_skipped, mode_skipped_count);
    cbm_gbuf_free(existing);

    if (tail_status != 0) {
        return tail_status;
    }

    cbm_log_info("incremental.done", "elapsed_ms", itoa_buf((int)elapsed_ms(t0)));
    return 0;
}
