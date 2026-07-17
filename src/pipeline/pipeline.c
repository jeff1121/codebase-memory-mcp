/*
 * pipeline.c — Indexing pipeline orchestrator.
 *
 * Coordinates multi-pass indexing:
 *   1. Discover files
 *   2. Build structure (Project/Folder/Package/File nodes)
 *   3. Bulk load sources (read + LZ4 HC compress)
 *   4. Extract definitions (fused: extract + write nodes + build registry)
 *   5. Resolve imports, calls, usages, semantic edges
 *   6. Post-passes: tests, communities, HTTP links, git history
 *   7. Dump graph buffer to SQLite
 */
#include "foundation/constants.h"

enum { CBM_DIR_PERMS = 0755, PL_RING = 4, PL_RING_MASK = 3, PL_SEQ_PASSES = 6, PL_WAL_BUF = 1040 };
#define PL_NSEC_PER_SEC 1000000000LL
#define PL_INCREMENTAL_FALLBACK (-2147483647 - 1)
#include "pipeline/pipeline.h"
#include "pipeline/artifact.h"
#include "pipeline/pipeline_internal.h"
#include "pipeline/pass_lsp_cross.h"
#include "pipeline/rust_plan.h"
#include "pipeline/worker_pool.h"
#include "graph_buffer/graph_buffer.h"
#include "git/git_context.h"
#include "store/store.h"
#include "discover/discover.h"
#include "discover/userconfig.h"
#include "foundation/platform.h"
#include "foundation/compat_fs.h"
#include "foundation/log.h"
#include "foundation/str_util.h"
#include "foundation/hash_table.h"
#include "foundation/compat.h"
#include "foundation/compat_thread.h"
#include "foundation/profile.h"
#include "foundation/mem.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <time.h>

static inline void *intptr_to_ptr(intptr_t v) {
    void *p;
    memcpy(&p, &v, sizeof(p));
    return p;
}

/* ── Global index lock ─────────────────────────────────────────── */
/* Prevents concurrent pipeline runs on the same DB file.
 * Atomic spinlock: 0 = free, 1 = locked. */
static atomic_int g_pipeline_busy = 0;

bool cbm_pipeline_try_lock(void) {
    return atomic_exchange(&g_pipeline_busy, 1) == 0;
}

#define LOCK_SPIN_NS 100000000 /* 100ms between lock retries */

void cbm_pipeline_lock(void) {
    while (atomic_exchange(&g_pipeline_busy, 1) != 0) {
        struct timespec ts = {0, LOCK_SPIN_NS};
        cbm_nanosleep(&ts, NULL);
    }
}

void cbm_pipeline_unlock(void) {
    atomic_store(&g_pipeline_busy, 0);
}

/* ── Internal state ──────────────────────────────────────────────── */

struct cbm_pipeline {
    char *repo_path;
    char *db_path;
    char *project_name;
    cbm_git_context_t git_ctx;
    char *branch_qn;
    cbm_index_mode_t mode;
    atomic_int cancelled;
    bool persistence; /* write .codebase-memory/graph.db.zst after indexing */

    /* Indexing state (set during run) */
    cbm_gbuf_t *gbuf;
    cbm_registry_t *registry;

    /* Directory subtrees skipped during discovery (rel paths). Captured from
     * cbm_discover_ex so the MCP layer can report excluded subtrees (#411).
     * Owned by the pipeline; freed in cbm_pipeline_free. */
    char **excluded_dirs;
    int excluded_count;

    /* User-defined extension overrides (loaded once per run) */
    cbm_userconfig_t *userconfig;

    /* Committed graph size at dump time (-1 = dump did not run). #334 gate axis. */
    int committed_nodes;
    int committed_edges;

    /* ADR (project_summaries) captured before a full-reindex DB delete, so it
     * can be restored after the rebuild. NULL when no ADR existed. Issue #516. */
    char *saved_adr;
};

/* ── Global pkgmap (one active pipeline at a time) ─────────────── */

static CBMHashTable *g_pkgmap = NULL;

CBMHashTable *cbm_pipeline_get_pkgmap(void) {
    return g_pkgmap;
}

void cbm_pipeline_set_pkgmap(CBMHashTable *map) {
    g_pkgmap = map;
}

/* ── Timing helper ──────────────────────────────────────────────── */

static double elapsed_ms(struct timespec start) {
    struct timespec now;
    cbm_clock_gettime(CLOCK_MONOTONIC, &now);
    return ((double)(now.tv_sec - start.tv_sec) * CBM_MS_PER_SEC) +
           ((double)(now.tv_nsec - start.tv_nsec) / CBM_US_PER_SEC_F);
}

/* Format int to string for logging. Thread-safe via TLS rotating buffers. */
static const char *itoa_buf(int val) {
    static CBM_TLS char bufs[PL_RING][CBM_SZ_32];
    static CBM_TLS int idx = 0;
    int i = idx;
    idx = (idx + SKIP_ONE) & PL_RING_MASK;
    snprintf(bufs[i], sizeof(bufs[i]), "%d", val);
    return bufs[i];
}

/* Log current + peak RSS at a pipeline phase boundary (memory profiling). */
static void log_phase_mem(const char *phase) {
    enum { PL_BYTES_PER_MB = 1024 * 1024 };
    cbm_log_info("mem.phase", "phase", phase, "rss_mb",
                 itoa_buf((int)(cbm_mem_rss() / PL_BYTES_PER_MB)), "peak_mb",
                 itoa_buf((int)(cbm_mem_peak_rss() / PL_BYTES_PER_MB)));
}

/* ── Lifecycle ──────────────────────────────────────────────────── */

cbm_pipeline_t *cbm_pipeline_new(const char *repo_path, const char *db_path,
                                 cbm_index_mode_t mode) {
    if (!repo_path) {
        return NULL;
    }

    cbm_pipeline_t *p = calloc(CBM_ALLOC_ONE, sizeof(cbm_pipeline_t));
    if (!p) {
        return NULL;
    }

    p->repo_path = strdup(repo_path);
    p->db_path = db_path ? strdup(db_path) : NULL;
    p->project_name = cbm_project_name_from_path(repo_path);
    (void)cbm_git_context_resolve(repo_path, &p->git_ctx);
    p->branch_qn = cbm_git_context_branch_qn(p->project_name, &p->git_ctx);
    p->mode = mode;
    p->persistence = false;
    p->committed_nodes = -1;
    p->committed_edges = -1;
    atomic_init(&p->cancelled, 0);

    return p;
}

void cbm_pipeline_set_persistence(cbm_pipeline_t *p, bool enabled) {
    if (p) {
        p->persistence = enabled;
    }
}

bool cbm_pipeline_set_project_name(cbm_pipeline_t *p, const char *name) {
    if (!p || !name || !name[0]) {
        return false;
    }

    char *normalized = cbm_project_name_from_path(name);
    if (!normalized) {
        return false;
    }
    if (!cbm_validate_project_name(normalized)) {
        free(normalized);
        return false;
    }

    free(p->project_name);
    p->project_name = normalized;
    free(p->branch_qn);
    p->branch_qn = cbm_git_context_branch_qn(p->project_name, &p->git_ctx);
    return true;
}

void cbm_pipeline_free(cbm_pipeline_t *p) {
    if (!p) {
        return;
    }
    free(p->repo_path);
    free(p->db_path);
    free(p->project_name);
    cbm_discover_free_excluded(p->excluded_dirs, p->excluded_count);
    p->excluded_dirs = NULL;
    p->excluded_count = 0;
    free(p->branch_qn);
    free(p->saved_adr); /* freed here too: error paths can exit before the
                         * restore in dump_and_persist_hashes runs. Issue #516. */
    p->saved_adr = NULL;
    cbm_git_context_free(&p->git_ctx);
    /* gbuf, store, registry freed during/after run */
    /* Defensively free userconfig in case run() was never called or panicked */
    if (p->userconfig) {
        cbm_set_user_lang_config(NULL);
        cbm_userconfig_free(p->userconfig);
        p->userconfig = NULL;
    }
    free(p);
}

void cbm_pipeline_cancel(cbm_pipeline_t *p) {
    if (p) {
        atomic_store(&p->cancelled, 1);
    }
}

const char *cbm_pipeline_project_name(const cbm_pipeline_t *p) {
    return p ? p->project_name : NULL;
}

const char *cbm_pipeline_repo_path(const cbm_pipeline_t *p) {
    return p ? p->repo_path : NULL;
}

atomic_int *cbm_pipeline_cancelled_ptr(cbm_pipeline_t *p) {
    return p ? &p->cancelled : NULL;
}

int cbm_pipeline_get_mode(const cbm_pipeline_t *p) {
    return p ? (int)p->mode : 0;
}

bool cbm_pipeline_persistence_enabled(const cbm_pipeline_t *p) {
    return p ? p->persistence : false;
}

void cbm_pipeline_get_excluded(const cbm_pipeline_t *p, char ***out, int *count) {
    if (out) {
        *out = p ? p->excluded_dirs : NULL;
    }
    if (count) {
        *count = p ? p->excluded_count : 0;
    }
}

void cbm_pipeline_get_committed_counts(const cbm_pipeline_t *p, int *nodes, int *edges) {
    if (nodes) {
        *nodes = p ? p->committed_nodes : -1;
    }
    if (edges) {
        *edges = p ? p->committed_edges : -1;
    }
}

void cbm_pipeline_set_committed_counts(cbm_pipeline_t *p, int nodes, int edges) {
    if (p) {
        p->committed_nodes = nodes;
        p->committed_edges = edges;
    }
}

/* Resolve the DB path for this pipeline. Caller must free(). */
static char *resolve_db_path(const cbm_pipeline_t *p) {
    char *path = malloc(CBM_SZ_1K);
    if (!path) {
        return NULL;
    }
    if (p->db_path) {
        snprintf(path, 1024, "%s", p->db_path);
    } else {
        snprintf(path, 1024, "%s/%s.db", cbm_resolve_cache_dir(), p->project_name);
    }
    return path;
}

static int check_cancel(const cbm_pipeline_t *p) {
    return atomic_load(&p->cancelled) ? CBM_NOT_FOUND : 0;
}

/* ── Hash table cleanup callback ─────────────────────────────────── */

static void free_seen_dir_key(const char *key, void *val, void *ud) {
    (void)val;
    (void)ud;
    free((void *)key);
}

/* ── Pass 1: Structure ──────────────────────────────────────────── */

/* Create Project, Folder/Package, and File nodes in the graph buffer. */
/* Walk directory chain upward, creating Folder nodes and CONTAINS_FOLDER edges. */
static void create_folder_chain(cbm_pipeline_t *p, const char *dir, CBMHashTable *seen_dirs) {
    char *walk = strdup(dir);
    while (walk[0] != '\0' && !cbm_ht_get(seen_dirs, walk)) {
        cbm_ht_set(seen_dirs, strdup(walk), intptr_to_ptr(SKIP_ONE));
        char *folder_qn = cbm_pipeline_fqn_folder(p->project_name, walk);
        const char *dir_base = strrchr(walk, '/');
        dir_base = dir_base ? dir_base + SKIP_ONE : walk;
        cbm_gbuf_apply_upsert_node(p->gbuf, "Folder", dir_base, folder_qn, walk, 0, 0, "{}");

        char *pdir = strdup(walk);
        char *ps = strrchr(pdir, '/');
        if (ps) {
            *ps = '\0';
        } else {
            free(pdir);
            pdir = strdup("");
        }
        const char *pqn;
        char *pqn_heap = NULL;
        if (pdir[0] == '\0') {
            pqn = p->branch_qn ? p->branch_qn : p->project_name;
        } else {
            pqn_heap = cbm_pipeline_fqn_folder(p->project_name, pdir);
            pqn = pqn_heap;
        }
        const cbm_gbuf_node_t *fn = cbm_gbuf_find_by_qn(p->gbuf, folder_qn);
        const cbm_gbuf_node_t *pn = cbm_gbuf_find_by_qn(p->gbuf, pqn);
        if (fn && pn) {
            cbm_gbuf_apply_insert_edge(p->gbuf, pn->id, fn->id, "CONTAINS_FOLDER", "{}");
        }
        free(folder_qn);
        free(pqn_heap);
        char *up = strrchr(walk, '/');
        if (up) {
            *up = '\0';
        } else {
            walk[0] = '\0';
        }
        free(pdir);
    }
    free(walk);
}

static int pass_structure(cbm_pipeline_t *p, const cbm_file_info_t *files, int file_count) {
    cbm_log_info("pass.start", "pass", "structure", "files", itoa_buf(file_count));

    /* Project node */
    cbm_gbuf_apply_upsert_node(p->gbuf, "Project", p->project_name, p->project_name, NULL, 0, 0,
                               "{}");
    const char *branch_qn = p->branch_qn ? p->branch_qn : p->project_name;
    const char *branch_name = p->git_ctx.branch ? p->git_ctx.branch : "working-tree";
    char branch_props[CBM_SZ_2K];
    const char *branch_props_json = "{}";
    if (cbm_git_context_props_json(&p->git_ctx, branch_props, sizeof(branch_props)) > 0) {
        branch_props_json = branch_props;
    }
    if (p->branch_qn) {
        int64_t branch_id = cbm_gbuf_apply_upsert_node(p->gbuf, "Branch", branch_name, branch_qn,
                                                       NULL, 0, 0, branch_props_json);
        const cbm_gbuf_node_t *project_node = cbm_gbuf_find_by_qn(p->gbuf, p->project_name);
        if (project_node && branch_id > 0) {
            cbm_gbuf_apply_insert_edge(p->gbuf, project_node->id, branch_id, "HAS_BRANCH",
                                       branch_props_json);
        }
    }

    /* Collect unique directories and create Folder/Package nodes */
    CBMHashTable *seen_dirs = cbm_ht_create(CBM_SZ_256);

    for (int i = 0; i < file_count; i++) {
        const char *rel = files[i].rel_path;
        if (!rel) {
            continue;
        }

        /* Create File node */
        char *file_qn = cbm_pipeline_fqn_compute(p->project_name, rel, "__file__");
        /* Extract basename */
        const char *slash = strrchr(rel, '/');
        const char *basename = slash ? slash + SKIP_ONE : rel;

        char props[CBM_SZ_256];
        const char *ext = strrchr(basename, '.');
        snprintf(props, sizeof(props), "{\"extension\":\"%s\"}", ext ? ext : "");

        const char *qualified_name = file_qn;
        const char *file_path = rel;
        cbm_gbuf_apply_upsert_node(p->gbuf, "File", basename, qualified_name, file_path, 0, 0,
                                   props);

        /* CONTAINS_FILE edge: parent dir -> file */
        char *dir = strdup(rel);
        char *last_slash = strrchr(dir, '/');
        if (last_slash) {
            {
                *last_slash = '\0';
            }
        } else {
            free(dir);
            dir = strdup("");
        }

        const char *parent_qn;
        char *parent_qn_heap = NULL;
        if (dir[0] == '\0') {
            parent_qn = branch_qn;
        } else {
            parent_qn_heap = cbm_pipeline_fqn_folder(p->project_name, dir);
            parent_qn = parent_qn_heap;
        }

        /* Walk up directory chain, creating Folder nodes */
        create_folder_chain(p, dir, seen_dirs);

        /* Now create the CONTAINS_FILE edge */
        const cbm_gbuf_node_t *fnode = cbm_gbuf_find_by_qn(p->gbuf, file_qn);
        const cbm_gbuf_node_t *pnode = cbm_gbuf_find_by_qn(p->gbuf, parent_qn);
        if (fnode && pnode) {
            cbm_gbuf_apply_insert_edge(p->gbuf, pnode->id, fnode->id, "CONTAINS_FILE", "{}");
        }

        free(file_qn);
        free(dir);
        free(parent_qn_heap);
    }

    /* Free seen_dirs keys */
    cbm_ht_foreach(seen_dirs, free_seen_dir_key, NULL);
    cbm_ht_free(seen_dirs);

    cbm_log_info("pass.done", "pass", "structure", "nodes", itoa_buf(cbm_gbuf_node_count(p->gbuf)),
                 "edges", itoa_buf(cbm_gbuf_edge_count(p->gbuf)));
    return 0;
}

/* ── Pass 2: Definitions ─────────────────────────────────────────── */

/* Implemented in pass_definitions.c via cbm_pipeline_pass_definitions() */

/* ── Githistory compute thread (for fused post-pass parallelism) ─── */

typedef struct {
    const char *repo_path;
    cbm_githistory_result_t *result;
} gh_compute_arg_t;

static void *gh_compute_thread_fn(void *arg) {
    gh_compute_arg_t *a = arg;
    cbm_pipeline_githistory_compute(a->repo_path, a->result);
    return NULL;
}

/* Extract Route nodes from URL strings found in config files (YAML, HCL, TOML).
 * These are infrastructure-defined endpoints (Cloud Scheduler, Terraform). */
/* Process infra bindings: topic→URL pairs from IaC configs.
 * Creates Route nodes for endpoints and HANDLES edges linking
 * topic Routes to endpoint Routes (bridging the gap). */
/* Process one infra binding: create Route node + INFRA_MAPS edge. */
static int process_one_infra_binding(cbm_gbuf_t *gbuf, const CBMInfraBinding *ib,
                                     const char *rel_path) {
    char url_route_qn[CBM_ROUTE_QN_SIZE];
    snprintf(url_route_qn, sizeof(url_route_qn), "__route__infra__%s", ib->target_url);
    int64_t url_route_id = cbm_gbuf_apply_upsert_node(gbuf, "Route", ib->target_url, url_route_qn,
                                                      rel_path, 0, 0, "{\"source\":\"infra\"}");
    char topic_route_qn[CBM_ROUTE_QN_SIZE];
    snprintf(topic_route_qn, sizeof(topic_route_qn), "__route__%s__%s",
             ib->broker ? ib->broker : "async", ib->source_name);
    const cbm_gbuf_node_t *topic_route = cbm_gbuf_find_by_qn(gbuf, topic_route_qn);
    int64_t topic_route_id;
    if (topic_route) {
        topic_route_id = topic_route->id;
    } else {
        /* The config file IS the declaration that the topic/queue/schedule exists;
         * upsert its Route node so the binding maps even when no code-side dispatch
         * call created the node first (e.g. a standalone scheduler/subscription
         * manifest). */
        topic_route_id =
            cbm_gbuf_apply_upsert_node(gbuf, "Route", ib->source_name, topic_route_qn, rel_path, 0,
                                       0, ib->broker ? ib->broker : "async");
        if (topic_route_id <= 0) {
            return 0;
        }
    }
    char props[CBM_SZ_512];
    snprintf(props, sizeof(props), "{\"broker\":\"%s\",\"topic\":\"%s\",\"endpoint\":\"%s\"}",
             ib->broker ? ib->broker : "async", ib->source_name, ib->target_url);
    cbm_gbuf_apply_insert_edge(gbuf, topic_route_id, url_route_id, "INFRA_MAPS", props);
    return SKIP_ONE;
}

static void cbm_pipeline_process_infra_bindings(cbm_gbuf_t *gbuf, const cbm_file_info_t *files,
                                                CBMFileResult **result_cache, int file_count) {
    int bindings = 0;
    for (int i = 0; i < file_count; i++) {
        if (!result_cache[i]) {
            continue;
        }
        for (int bi = 0; bi < result_cache[i]->infra_bindings.count; bi++) {
            const CBMInfraBinding *ib = &result_cache[i]->infra_bindings.items[bi];
            if (ib->source_name && ib->target_url) {
                bindings += process_one_infra_binding(gbuf, ib, files[i].rel_path);
            }
        }
    }
    if (bindings > 0) {
        char buf[CBM_SZ_16];
        snprintf(buf, sizeof(buf), "%d", bindings);
        cbm_log_info("pass.infra_bindings", "linked", buf);
    }
}

static bool is_infra_file(const char *fp) {
    return fp != NULL &&
           (strstr(fp, ".yaml") != NULL || strstr(fp, ".yml") != NULL ||
            strstr(fp, ".tf") != NULL || strstr(fp, ".hcl") != NULL || strstr(fp, ".toml") != NULL);
}

/* True when a YAML key path denotes an UPSTREAM dependency, CONFIG value, or
 * HEALTHCHECK target rather than an endpoint this service exposes. Such URLs
 * (auth JWKS, downstream service base URLs, package-registry URLs, healthcheck
 * curl targets) are NOT routes the service serves and must not mint Route nodes
 * (#521). Exposed-endpoint keys (push_endpoint, post_url, callback, webhook)
 * are intentionally absent here so they still produce infra Route nodes. */
static bool is_upstream_config_key(const char *key_path) {
    if (!key_path) {
        /* No key context (e.g. flat string) — keep prior behaviour and mint. */
        return false;
    }
    static const char *const deny[] = {"jwks",     "registry",     "registries", "healthcheck",
                                       "upstream", "_service_url", "auth",       NULL};
    for (int i = 0; deny[i]; i++) {
        if (strstr(key_path, deny[i]) != NULL) {
            return true;
        }
    }
    return false;
}

/* Try to create an infra Route node from one string_ref. */
static void try_upsert_infra_route(cbm_gbuf_t *gbuf, const CBMStringRef *sr, const char *fp) {
    if (sr->kind != CBM_STRREF_URL || !sr->value || !strstr(sr->value, "://")) {
        return;
    }
    /* Skip upstream/config/healthcheck URLs — they are not exposed routes (#521). */
    if (is_upstream_config_key(sr->key_path)) {
        return;
    }
    char route_qn[CBM_ROUTE_QN_SIZE];
    snprintf(route_qn, sizeof(route_qn), "__route__infra__%s", sr->value);
    char route_props[CBM_SZ_512];
    if (sr->key_path) {
        snprintf(route_props, sizeof(route_props), "{\"source\":\"infra\",\"key_path\":\"%s\"}",
                 sr->key_path);
    } else {
        snprintf(route_props, sizeof(route_props), "{\"source\":\"infra\"}");
    }
    cbm_gbuf_apply_upsert_node(gbuf, "Route", sr->value, route_qn, fp, 0, 0, route_props);
}

/* A URL string_ref that does NOT denote a route the service serves: a value
 * containing whitespace is a command/sentence with an embedded URL (e.g. a
 * Docker healthcheck `curl --fail http://... || exit 1`); a NULL key_path is a
 * context-less/duplicate ref; an upstream/config/healthcheck key is an external
 * dependency, not an exposed route. (#521) */
static bool route_sr_denied(const CBMStringRef *sr) {
    if (!sr->value || strchr(sr->value, ' ')) {
        return true;
    }
    if (!sr->key_path) {
        return true;
    }
    return is_upstream_config_key(sr->key_path);
}

static void cbm_pipeline_extract_infra_routes(cbm_gbuf_t *gbuf, const cbm_file_info_t *files,
                                              CBMFileResult **result_cache, int file_count) {
    /* DENY-WINS-BY-VALUE: the same URL is often extracted as several string_refs
     * at different key_path granularities (full path, leaf key, flat). The Route
     * node is keyed by VALUE, so it would be minted if ANY granularity passed the
     * per-ref guard — e.g. a denied full path `registries.terraform-registry.url`
     * is defeated by a sibling leaf `url`. So pass 1 collects every URL value
     * denied under ANY of its refs; pass 2 mints only values never denied. (#521) */
    CBMHashTable *denied = cbm_ht_create(16);
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < file_count; i++) {
            if (!result_cache[i] || !is_infra_file(files[i].rel_path)) {
                continue;
            }
            for (int si = 0; si < result_cache[i]->string_refs.count; si++) {
                const CBMStringRef *sr = &result_cache[i]->string_refs.items[si];
                if (sr->kind != CBM_STRREF_URL || !sr->value || !strstr(sr->value, "://")) {
                    continue;
                }
                if (pass == 0) {
                    if (denied && route_sr_denied(sr)) {
                        cbm_ht_set(denied, sr->value, (void *)1);
                    }
                } else if (!denied || !cbm_ht_has(denied, sr->value)) {
                    try_upsert_infra_route(gbuf, sr, files[i].rel_path);
                }
            }
        }
    }
    cbm_ht_free(denied);
}

/* Run decorator_tags, configlink, and route matching passes. */
typedef void (*predump_pass_fn)(cbm_pipeline_ctx_t *);
typedef struct {
    predump_pass_fn fn;
    const char *name;
    bool moderate_only; /* true = skip in fast mode */
    int rust_kind;
    uint32_t gate_flags;
    uint32_t effect_flags;
} predump_pass_def_t;

static void predump_deco(cbm_pipeline_ctx_t *ctx) {
    cbm_pipeline_pass_decorator_tags(ctx->gbuf, ctx->project_name);
}
static void predump_route(cbm_pipeline_ctx_t *ctx) {
    cbm_pipeline_create_route_nodes(ctx->gbuf);
}
static void predump_sim(cbm_pipeline_ctx_t *ctx) {
    cbm_pipeline_pass_similarity(ctx);
}
static void predump_sem(cbm_pipeline_ctx_t *ctx) {
    cbm_pipeline_pass_semantic_edges(ctx);
}
static void predump_cfg(cbm_pipeline_ctx_t *ctx) {
    cbm_pipeline_pass_configlink(ctx);
}
static void predump_complexity(cbm_pipeline_ctx_t *ctx) {
    cbm_pipeline_pass_complexity(ctx);
}

static const predump_pass_def_t predump_passes[] = {
    {predump_deco, "decorator_tags", false, CBM_RS_PLAN_STEP_PREDUMP_DECORATOR_TAGS, 0,
     CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {predump_cfg, "configlink", false, CBM_RS_PLAN_STEP_PREDUMP_CONFIGLINK, 0,
     CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {predump_route, "route_match", false, CBM_RS_PLAN_STEP_PREDUMP_ROUTE_MATCH, 0,
     CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {predump_sim, "similarity", true, CBM_RS_PLAN_STEP_PREDUMP_SIMILARITY,
     CBM_RS_PLAN_GATE_SKIP_FAST, CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {predump_sem, "semantic_edges", true, CBM_RS_PLAN_STEP_PREDUMP_SEMANTIC_EDGES,
     CBM_RS_PLAN_GATE_SKIP_FAST, CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {predump_complexity, "complexity", false, CBM_RS_PLAN_STEP_PREDUMP_COMPLEXITY, 0,
     CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
};
enum { PREDUMP_PASS_COUNT = (int)(sizeof(predump_passes) / sizeof(predump_passes[0])) };

static void run_predump_pass(cbm_pipeline_ctx_t *ctx, const predump_pass_def_t *pass) {
    struct timespec t;
    cbm_clock_gettime(CLOCK_MONOTONIC, &t);
    pass->fn(ctx);
    cbm_log_info("pass.timing", "pass", pass->name, "elapsed_ms", itoa_buf((int)elapsed_ms(t)));
}

static void run_c_predump_passes(cbm_pipeline_t *p, cbm_pipeline_ctx_t *ctx) {
    for (int i = 0; i < PREDUMP_PASS_COUNT && !check_cancel(p); i++) {
        /* "moderate_only" passes (similarity/semantic edges) run in FULL,
         * MODERATE and ADVANCED — they are skipped only in FAST. Compare
         * explicitly against FAST rather than `> MODERATE` so ADVANCED
         * (numerically 3) is not mistaken for a lighter mode than FULL. */
        if (predump_passes[i].moderate_only && p->mode == CBM_MODE_FAST) {
            continue;
        }
        run_predump_pass(ctx, &predump_passes[i]);
    }
}

#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
static uint64_t predump_step_bit(int kind) {
    if (kind <= 0 || kind > CBM_RS_PLAN_STEP_MAX_KIND) {
        return 0;
    }
    return UINT64_C(1) << (uint32_t)(kind - 1);
}

static const predump_pass_def_t *find_predump_pass(const char *name, size_t len) {
    if (!name || len == 0) {
        return NULL;
    }
    for (int i = 0; i < PREDUMP_PASS_COUNT; i++) {
        if (strlen(predump_passes[i].name) == len &&
            strncmp(predump_passes[i].name, name, len) == 0) {
            return &predump_passes[i];
        }
    }
    return NULL;
}

static const predump_pass_def_t *find_predump_pass_by_kind(int kind) {
    if (kind <= 0) {
        return NULL;
    }
    for (int i = 0; i < PREDUMP_PASS_COUNT; i++) {
        if (predump_passes[i].rust_kind == kind) {
            return &predump_passes[i];
        }
    }
    return NULL;
}

static bool render_predump_plan(const predump_pass_def_t *const *ordered, int count, char *buf,
                                size_t bufsize) {
    if (!ordered || count < 0 || !buf || bufsize == 0) {
        return false;
    }
    size_t used = 0;
    buf[0] = '\0';
    for (int i = 0; i < count; i++) {
        int n = snprintf(buf + used, bufsize - used, "%s%s", i == 0 ? "" : ",", ordered[i]->name);
        if (n < 0 || (size_t)n >= bufsize - used) {
            buf[bufsize - 1] = '\0';
            return false;
        }
        used += (size_t)n;
    }
    return true;
}

static bool decode_rust_predump_plan(const char *plan, int mode, const predump_pass_def_t **ordered,
                                     int *out_count) {
    if (!plan || !ordered || !out_count) {
        return false;
    }
    bool seen[PREDUMP_PASS_COUNT] = {false};
    int expected_idx = 0;
    *out_count = 0;
    const char *start = plan;
    while (*start) {
        const char *end = strchr(start, ',');
        size_t len = end ? (size_t)(end - start) : strlen(start);
        const predump_pass_def_t *pass = find_predump_pass(start, len);
        int pass_idx = pass ? (int)(pass - predump_passes) : -1;
        while (expected_idx < PREDUMP_PASS_COUNT && predump_passes[expected_idx].moderate_only &&
               mode == CBM_MODE_FAST) {
            expected_idx++;
        }
        if (!pass || (pass->moderate_only && mode == CBM_MODE_FAST) ||
            *out_count >= PREDUMP_PASS_COUNT || pass_idx != expected_idx || seen[pass_idx]) {
            return false;
        }
        seen[pass_idx] = true;
        ordered[(*out_count)++] = pass;
        expected_idx++;
        if (!end) {
            break;
        }
        start = end + 1;
    }
    int expected = mode == CBM_MODE_FAST ? PREDUMP_PASS_COUNT - 2 : PREDUMP_PASS_COUNT;
    return *out_count == expected;
}

static bool decode_rust_predump_steps_v2(const cbm_rs_pipeline_plan_step_v2_t *steps,
                                         int step_count, int mode,
                                         const predump_pass_def_t **ordered, int *out_count) {
    if (!steps || step_count < 0 || !ordered || !out_count) {
        return false;
    }
    int expected = mode == CBM_MODE_FAST ? PREDUMP_PASS_COUNT - 2 : PREDUMP_PASS_COUNT;
    if (step_count != expected) {
        return false;
    }

    bool seen[PREDUMP_PASS_COUNT] = {false};
    uint64_t seen_mask = 0;
    int expected_idx = 0;
    *out_count = 0;
    for (int i = 0; i < step_count; i++) {
        const cbm_rs_pipeline_plan_step_v2_t *step = &steps[i];
        const predump_pass_def_t *pass = find_predump_pass_by_kind(step->kind);
        int pass_idx = pass ? (int)(pass - predump_passes) : -1;
        while (expected_idx < PREDUMP_PASS_COUNT && predump_passes[expected_idx].moderate_only &&
               mode == CBM_MODE_FAST) {
            expected_idx++;
        }
        if (!pass || step->phase != CBM_RS_PLAN_PHASE_PREDUMP ||
            step->policy != CBM_RS_PLAN_POLICY_REQUIRED || step->gate_flags != pass->gate_flags ||
            step->effect_flags != pass->effect_flags || step->requires_mask != seen_mask ||
            predump_step_bit(step->kind) == 0 || (seen_mask & predump_step_bit(step->kind)) != 0 ||
            (pass->moderate_only && mode == CBM_MODE_FAST) || pass_idx != expected_idx ||
            seen[pass_idx]) {
            return false;
        }
        seen[pass_idx] = true;
        ordered[(*out_count)++] = pass;
        seen_mask |= predump_step_bit(step->kind);
        expected_idx++;
    }
    return *out_count == expected;
}

static bool run_rust_predump_passes(cbm_pipeline_t *p, cbm_pipeline_ctx_t *ctx) {
    char plan[CBM_RS_PLAN_SEQUENTIAL_BUF];
    const predump_pass_def_t *ordered[PREDUMP_PASS_COUNT];
    int count = 0;
    const char *source = "typed_v2";
    cbm_rs_pipeline_plan_step_v2_t steps[CBM_RS_PLAN_V2_MAX_STEPS];
    int step_count = 0;
    bool typed_ok = cbm_rust_plan_steps_v2(CBM_RS_PLAN_PREDUMP, p->mode, 0, 0, steps,
                                           CBM_RS_PLAN_V2_MAX_STEPS, &step_count) &&
                    decode_rust_predump_steps_v2(steps, step_count, p->mode, ordered, &count) &&
                    render_predump_plan(ordered, count, plan, sizeof(plan));
    if (typed_ok) {
        /* Typed v2 metadata is the preferred opt-in path. */
    } else if (cbm_rust_plan_predump(p->mode, plan, sizeof(plan)) &&
               decode_rust_predump_plan(plan, p->mode, ordered, &count)) {
        cbm_log_warn("rust_plan.fallback", "phase", "predump", "reason", "typed_v2_unavailable",
                     "path", "string_plan");
        source = "string";
    } else {
        cbm_log_warn("rust_plan.fallback", "phase", "predump", "reason",
                     "typed_v2_and_string_unavailable", "path", "c_predump");
        return false;
    }
    cbm_log_info("rust_plan.dispatch", "phase", "predump", "passes", itoa_buf(count), "plan", plan,
                 "source", source);
    for (int i = 0; i < count && !check_cancel(p); i++) {
        run_predump_pass(ctx, ordered[i]);
    }
    return true;
}
#endif

static void run_predump_passes(cbm_pipeline_t *p, cbm_pipeline_ctx_t *ctx) {
#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
    if (run_rust_predump_passes(p, ctx)) {
        return;
    }
#if defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
    cbm_log_error("rust_plan.err", "phase", "predump", "path", "rust_plan_only");
    cbm_pipeline_cancel(p);
    return;
#endif
#endif
    run_c_predump_passes(p, ctx);
}

/* Adapter that lets cbm_pipeline_pass_lsp_cross slot into the seq_passes
 * dispatch table. The cross-file LSP needs the per-file CBMFileResult cache
 * to read defs/imports without re-extracting; in the sequential path that
 * cache is ctx->result_cache (set up by run_sequential_pipeline before
 * launching the dispatch loop). When the cache is unavailable (e.g. if the
 * pipeline opted out of caching), the pass becomes a no-op since there are
 * no extracted results to feed cross-file resolution. */
static int seq_pass_lsp_cross_dispatch(cbm_pipeline_ctx_t *ctx, const cbm_file_info_t *files,
                                       int file_count) {
    if (!ctx || !ctx->result_cache)
        return 0;
    /* Cross-file LSP runs in every mode. */
    return cbm_pipeline_pass_lsp_cross(ctx, files, file_count, ctx->result_cache);
}

typedef int (*seq_pass_fn)(cbm_pipeline_ctx_t *, const cbm_file_info_t *, int);
typedef struct {
    seq_pass_fn fn;
    const char *name;
    bool ignore_err;
    int rust_kind;
    int rust_policy;
    uint32_t gate_flags;
    uint32_t effect_flags;
} seq_pass_def_t;

static const seq_pass_def_t seq_passes[] = {
    {cbm_pipeline_pass_definitions, "definitions", false, CBM_RS_PLAN_STEP_SEQUENTIAL_DEFINITIONS,
     CBM_RS_PLAN_POLICY_REQUIRED, 0, CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {cbm_pipeline_pass_k8s, "k8s", true, CBM_RS_PLAN_STEP_SEQUENTIAL_K8S,
     CBM_RS_PLAN_POLICY_IGNORE_ERR, 0, CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {seq_pass_lsp_cross_dispatch, "lsp_cross", true, CBM_RS_PLAN_STEP_SEQUENTIAL_LSP_CROSS,
     CBM_RS_PLAN_POLICY_IGNORE_ERR, CBM_RS_PLAN_GATE_REQUIRES_RESULT_CACHE,
     CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {cbm_pipeline_pass_calls, "calls", false, CBM_RS_PLAN_STEP_SEQUENTIAL_CALLS,
     CBM_RS_PLAN_POLICY_REQUIRED, 0, CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {cbm_pipeline_pass_usages, "usages", false, CBM_RS_PLAN_STEP_SEQUENTIAL_USAGES,
     CBM_RS_PLAN_POLICY_REQUIRED, 0, CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {cbm_pipeline_pass_semantic, "semantic", false, CBM_RS_PLAN_STEP_SEQUENTIAL_SEMANTIC,
     CBM_RS_PLAN_POLICY_REQUIRED, 0, CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
};
enum { SEQ_PASS_COUNT = (int)(sizeof(seq_passes) / sizeof(seq_passes[0])) };

static int run_sequential_dispatch(cbm_pipeline_t *p, cbm_pipeline_ctx_t *ctx,
                                   const cbm_file_info_t *files, int file_count, struct timespec *t,
                                   const seq_pass_def_t *const *ordered, int count) {
    int rc = 0;
    for (int si = 0; si < count && rc == 0; si++) {
        const seq_pass_def_t *pass = ordered[si];
        cbm_clock_gettime(CLOCK_MONOTONIC, t);
        int pr = pass->fn(ctx, files, file_count);
        if (pr != 0 && !pass->ignore_err) {
            rc = pr;
        }
        cbm_log_info("pass.timing", "pass", pass->name, "elapsed_ms",
                     itoa_buf((int)elapsed_ms(*t)));
        if (check_cancel(p)) {
            rc = CBM_NOT_FOUND;
        }
    }
    return rc;
}

#if !defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
static int run_c_sequential_dispatch(cbm_pipeline_t *p, cbm_pipeline_ctx_t *ctx,
                                     const cbm_file_info_t *files, int file_count,
                                     struct timespec *t) {
    const seq_pass_def_t *ordered[SEQ_PASS_COUNT];
    for (int i = 0; i < SEQ_PASS_COUNT; i++) {
        ordered[i] = &seq_passes[i];
    }
    return run_sequential_dispatch(p, ctx, files, file_count, t, ordered, SEQ_PASS_COUNT);
}
#endif

#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
static uint64_t seq_step_bit(int kind) {
    if (kind <= 0 || kind > CBM_RS_PLAN_STEP_MAX_KIND) {
        return 0;
    }
    return UINT64_C(1) << (uint32_t)(kind - 1);
}

static const seq_pass_def_t *find_seq_pass_by_kind(int kind) {
    if (kind <= 0) {
        return NULL;
    }
    for (int i = 0; i < SEQ_PASS_COUNT; i++) {
        if (seq_passes[i].rust_kind == kind) {
            return &seq_passes[i];
        }
    }
    return NULL;
}

static bool render_seq_plan(const seq_pass_def_t *const *ordered, int count, char *buf,
                            size_t bufsize) {
    if (!ordered || count < 0 || !buf || bufsize == 0) {
        return false;
    }
    size_t used = 0;
    buf[0] = '\0';
    for (int i = 0; i < count; i++) {
        int n = snprintf(buf + used, bufsize - used, "%s%s", i == 0 ? "" : ",", ordered[i]->name);
        if (n < 0 || (size_t)n >= bufsize - used) {
            buf[bufsize - 1] = '\0';
            return false;
        }
        used += (size_t)n;
    }
    return true;
}

static bool decode_rust_sequential_steps_v2(const cbm_rs_pipeline_plan_step_v2_t *steps,
                                            int step_count, const seq_pass_def_t **ordered,
                                            int *out_count) {
    if (!steps || step_count != SEQ_PASS_COUNT || !ordered || !out_count) {
        return false;
    }
    bool seen[SEQ_PASS_COUNT] = {false};
    uint64_t seen_mask = 0;
    *out_count = 0;
    for (int i = 0; i < step_count; i++) {
        const cbm_rs_pipeline_plan_step_v2_t *step = &steps[i];
        const seq_pass_def_t *pass = find_seq_pass_by_kind(step->kind);
        int pass_idx = pass ? (int)(pass - seq_passes) : -1;
        uint64_t bit = seq_step_bit(step->kind);
        if (!pass || step->phase != CBM_RS_PLAN_PHASE_SEQUENTIAL_EXTRACT ||
            step->policy != pass->rust_policy || step->gate_flags != pass->gate_flags ||
            step->effect_flags != pass->effect_flags || step->requires_mask != seen_mask ||
            bit == 0 || (seen_mask & bit) != 0 || pass_idx != i || seen[pass_idx]) {
            return false;
        }
        seen[pass_idx] = true;
        ordered[(*out_count)++] = pass;
        seen_mask |= bit;
    }
    return *out_count == SEQ_PASS_COUNT;
}

static bool run_rust_sequential_dispatch(cbm_pipeline_t *p, cbm_pipeline_ctx_t *ctx,
                                         const cbm_file_info_t *files, int file_count,
                                         struct timespec *t, int *out_rc) {
    cbm_rs_pipeline_plan_step_v2_t steps[CBM_RS_PLAN_V2_MAX_STEPS];
    const seq_pass_def_t *ordered[SEQ_PASS_COUNT];
    int step_count = 0;
    int count = 0;
    char plan[CBM_RS_PLAN_SEQUENTIAL_BUF];
    if (!out_rc ||
        !cbm_rust_plan_steps_v2(CBM_RS_PLAN_SEQUENTIAL, p->mode, 0, file_count, steps,
                                CBM_RS_PLAN_V2_MAX_STEPS, &step_count) ||
        !decode_rust_sequential_steps_v2(steps, step_count, ordered, &count) ||
        !render_seq_plan(ordered, count, plan, sizeof(plan))) {
        return false;
    }
    cbm_log_info("rust_plan.dispatch", "phase", "sequential_extract", "passes", itoa_buf(count),
                 "plan", plan, "source", "typed_v2");
    *out_rc = run_sequential_dispatch(p, ctx, files, file_count, t, ordered, count);
    return true;
}
#endif

/* Run the sequential pipeline path: definitions, k8s, lsp_cross, calls, usages, semantic. */
static int run_sequential_pipeline(cbm_pipeline_t *p, cbm_pipeline_ctx_t *ctx,
                                   const cbm_file_info_t *files, int file_count,
                                   struct timespec *t) {
    cbm_log_info("pipeline.mode", "mode", "sequential", "files", itoa_buf(file_count));

    /* Build package map from manifest files (sequential: read manifests directly).
     * Use the repo-walking variant so manifests filtered out by the main
     * discoverer (package.json, composer.json) still feed pkgmap and let
     * workspace imports like `@my/pkg` resolve to their target Module. */
    cbm_pipeline_set_pkgmap(
        cbm_pkgmap_build_from_repo(ctx->repo_path, files, file_count, ctx->project_name));

    CBMFileResult **seq_cache = (CBMFileResult **)calloc(file_count, sizeof(CBMFileResult *));
    if (seq_cache) {
        ctx->result_cache = seq_cache;
    }
    int rc = 0;
#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
    if (!run_rust_sequential_dispatch(p, ctx, files, file_count, t, &rc)) {
#if defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
        cbm_log_error("rust_plan.err", "phase", "sequential_extract", "path", "rust_plan_only");
        rc = CBM_NOT_FOUND;
#else
        cbm_log_warn("rust_plan.fallback", "phase", "sequential_extract", "reason",
                     "typed_v2_unavailable", "path", "c_dispatch");
        rc = run_c_sequential_dispatch(p, ctx, files, file_count, t);
#endif
    }
#else
    rc = run_c_sequential_dispatch(p, ctx, files, file_count, t);
#endif
    /* Consume infra bindings (YAML/HCL topic/queue/scheduler → endpoint) so
     * INFRA_MAPS edges also form on the sequential path, not just the parallel
     * one. process_one_infra_binding self-creates the topic Route node when no
     * code-side dispatch created it (e.g. a standalone scheduler manifest). */
    if (seq_cache && rc == 0) {
        cbm_pipeline_extract_infra_routes(p->gbuf, files, seq_cache, file_count);
        cbm_pipeline_process_infra_bindings(p->gbuf, files, seq_cache, file_count);
    }
    if (seq_cache) {
        for (int i = 0; i < file_count; i++) {
            if (seq_cache[i]) {
                cbm_free_result(seq_cache[i]);
            }
        }
        free(seq_cache);
        ctx->result_cache = NULL;
    }
    return rc;
}

#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
typedef struct {
    const char *name;
    int rust_kind;
    int rust_policy;
    uint32_t gate_flags;
    uint32_t effect_flags;
} parallel_pass_def_t;

static const parallel_pass_def_t parallel_passes[] = {
    {"parallel_extract", CBM_RS_PLAN_STEP_PARALLEL_EXTRACT, CBM_RS_PLAN_POLICY_REQUIRED, 0,
     CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {"registry_build", CBM_RS_PLAN_STEP_PARALLEL_REGISTRY_BUILD, CBM_RS_PLAN_POLICY_REQUIRED, 0,
     CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {"lsp_cross_prepare", CBM_RS_PLAN_STEP_PARALLEL_LSP_CROSS_PREPARE,
     CBM_RS_PLAN_POLICY_ENV_OPTIONAL, 0, 0},
    {"parallel_resolve", CBM_RS_PLAN_STEP_PARALLEL_RESOLVE, CBM_RS_PLAN_POLICY_REQUIRED, 0,
     CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {"infra_routes", CBM_RS_PLAN_STEP_PARALLEL_INFRA_ROUTES, CBM_RS_PLAN_POLICY_REQUIRED, 0,
     CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {"infra_bindings", CBM_RS_PLAN_STEP_PARALLEL_INFRA_BINDINGS, CBM_RS_PLAN_POLICY_REQUIRED, 0,
     CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
    {"k8s", CBM_RS_PLAN_STEP_PARALLEL_K8S, CBM_RS_PLAN_POLICY_IGNORE_ERR, 0,
     CBM_RS_PLAN_EFFECT_MUTATES_GRAPH},
};
enum { PARALLEL_PASS_COUNT = (int)(sizeof(parallel_passes) / sizeof(parallel_passes[0])) };

static uint64_t parallel_step_bit(int kind) {
    if (kind <= 0 || kind > CBM_RS_PLAN_STEP_MAX_KIND) {
        return 0;
    }
    return UINT64_C(1) << (uint32_t)(kind - 1);
}

static const parallel_pass_def_t *find_parallel_pass_by_kind(int kind) {
    if (kind <= 0) {
        return NULL;
    }
    for (int i = 0; i < PARALLEL_PASS_COUNT; i++) {
        if (parallel_passes[i].rust_kind == kind) {
            return &parallel_passes[i];
        }
    }
    return NULL;
}

static bool render_parallel_plan(const parallel_pass_def_t *const *ordered, int count, char *buf,
                                 size_t bufsize) {
    if (!ordered || count < 0 || !buf || bufsize == 0) {
        return false;
    }
    size_t used = 0;
    buf[0] = '\0';
    for (int i = 0; i < count; i++) {
        int n = snprintf(buf + used, bufsize - used, "%s%s", i == 0 ? "" : ",", ordered[i]->name);
        if (n < 0 || (size_t)n >= bufsize - used) {
            buf[bufsize - 1] = '\0';
            return false;
        }
        used += (size_t)n;
    }
    return true;
}

static bool decode_rust_parallel_steps_v2(const cbm_rs_pipeline_plan_step_v2_t *steps,
                                          int step_count, const parallel_pass_def_t **ordered,
                                          int *out_count) {
    if (!steps || step_count != PARALLEL_PASS_COUNT || !ordered || !out_count) {
        return false;
    }
    bool seen[PARALLEL_PASS_COUNT] = {false};
    uint64_t seen_mask = 0;
    *out_count = 0;
    for (int i = 0; i < step_count; i++) {
        const cbm_rs_pipeline_plan_step_v2_t *step = &steps[i];
        const parallel_pass_def_t *pass = find_parallel_pass_by_kind(step->kind);
        int pass_idx = pass ? (int)(pass - parallel_passes) : -1;
        uint64_t bit = parallel_step_bit(step->kind);
        if (!pass || step->phase != CBM_RS_PLAN_PHASE_PARALLEL_EXTRACT ||
            step->policy != pass->rust_policy || step->gate_flags != pass->gate_flags ||
            step->effect_flags != pass->effect_flags || step->requires_mask != seen_mask ||
            bit == 0 || (seen_mask & bit) != 0 || pass_idx != i || seen[pass_idx]) {
            return false;
        }
        seen[pass_idx] = true;
        ordered[(*out_count)++] = pass;
        seen_mask |= bit;
    }
    return *out_count == PARALLEL_PASS_COUNT;
}

static bool run_rust_parallel_dispatch(cbm_pipeline_t *p, cbm_pipeline_ctx_t *ctx,
                                       const cbm_file_info_t *files, int file_count,
                                       int worker_count, CBMFileResult **cache,
                                       _Atomic int64_t *shared_ids, struct timespec *t,
                                       int *out_rc) {
    if (!p || !ctx || !files || file_count < 0 || !cache || !shared_ids || !t || !out_rc) {
        return false;
    }
    cbm_rs_pipeline_plan_step_v2_t steps[CBM_RS_PLAN_V2_MAX_STEPS];
    const parallel_pass_def_t *ordered[PARALLEL_PASS_COUNT];
    int step_count = 0;
    int count = 0;
    char plan[CBM_RS_PLAN_PARALLEL_BUF];
    if (!cbm_rust_plan_steps_v2(CBM_RS_PLAN_PARALLEL_EXTRACTION, p->mode, worker_count, file_count,
                                steps, CBM_RS_PLAN_V2_MAX_STEPS, &step_count) ||
        !decode_rust_parallel_steps_v2(steps, step_count, ordered, &count) ||
        !render_parallel_plan(ordered, count, plan, sizeof(plan))) {
        return false;
    }
    cbm_log_info("rust_plan.dispatch", "phase", "parallel_extract", "passes", itoa_buf(count),
                 "plan", plan, "source", "typed_v2");

    char cbm_lsp_cross_env[CBM_SZ_16];
    const bool run_cross_lsp = cbm_safe_getenv("CBM_DISABLE_LSP_CROSS", cbm_lsp_cross_env,
                                               sizeof(cbm_lsp_cross_env), NULL) == NULL;
    if (!run_cross_lsp) {
        cbm_log_info("lsp_cross.skipped", "reason", "CBM_DISABLE_LSP_CROSS env set");
    }

    CBMArena cross_lsp_arena;
    cbm_arena_init(&cross_lsp_arena);
    CBMCrossLspRegistries cross_registries = {0};
    CBMModuleDefIndex *module_def_index = NULL;
    char **def_modules = NULL;
    int def_count = 0;
    CBMLSPDef *all_defs = NULL;
    bool cross_state_ready = false;
    bool cross_state_cleaned = false;
    int rc = 0;
    bool stop = false;

    for (int si = 0; si < count && !stop; si++) {
        const parallel_pass_def_t *pass = ordered[si];
        cbm_clock_gettime(CLOCK_MONOTONIC, t);
        switch (pass->rust_kind) {
        case CBM_RS_PLAN_STEP_PARALLEL_EXTRACT:
            rc = cbm_parallel_extract(ctx, files, file_count, cache, shared_ids, worker_count);
            cbm_log_info("pass.timing", "pass", "parallel_extract", "elapsed_ms",
                         itoa_buf((int)elapsed_ms(*t)));
            if (rc != 0 || check_cancel(p)) {
                rc = rc != 0 ? rc : CBM_NOT_FOUND;
                stop = true;
                break;
            }
            cbm_gbuf_set_next_id(p->gbuf, atomic_load(shared_ids));
            /* extract -> registry handoff: return the extract phase's freed-but-retained
             * allocator pages to the OS before registry_build allocates. On a 2x Linux
             * index the extract peak holds ~13 GB of reclaimable pages (peak_mb 20.7 vs
             * live rss_mb 7); not returning them pushed the process over the system
             * memory-pressure threshold and got it SIGKILLed at registry entry. */
            cbm_mem_collect();
            cbm_log_info("mem.collect", "phase", "post_extract", "rss_mb",
                         itoa_buf((int)(cbm_mem_rss() / (1024 * 1024))));
            break;
        case CBM_RS_PLAN_STEP_PARALLEL_REGISTRY_BUILD:
            rc = cbm_build_registry_from_cache(ctx, files, file_count, cache);
            cbm_log_info("pass.timing", "pass", "registry_build", "elapsed_ms",
                         itoa_buf((int)elapsed_ms(*t)));
            log_phase_mem("registry_build");
            if (rc != 0 || check_cancel(p)) {
                rc = rc != 0 ? rc : CBM_NOT_FOUND;
                stop = true;
            }
            break;
        case CBM_RS_PLAN_STEP_PARALLEL_LSP_CROSS_PREPARE:
            if (run_cross_lsp) {
                def_modules = (char **)calloc((size_t)file_count, sizeof(char *));
                all_defs = def_modules ? cbm_pxc_collect_all_defs(cache, files, file_count,
                                                                  ctx->project_name, def_modules,
                                                                  &def_count)
                                       : NULL;
                module_def_index =
                    all_defs ? cbm_pxc_build_module_def_index(all_defs, def_count) : NULL;
                if (all_defs) {
                    cross_registries.go =
                        cbm_go_build_cross_registry(&cross_lsp_arena, all_defs, def_count);
                    cross_registries.python =
                        cbm_py_build_cross_registry(&cross_lsp_arena, all_defs, def_count);
                    cross_registries.c =
                        cbm_c_build_cross_registry(&cross_lsp_arena, all_defs, def_count);
                    cross_registries.cs =
                        cbm_cs_build_cross_registry(&cross_lsp_arena, all_defs, def_count);
                    cross_registries.ts =
                        cbm_ts_build_cross_registry(&cross_lsp_arena, all_defs, def_count);
                }
                cross_state_ready = true;
            }
            cbm_log_info("pass.timing", "pass", "lsp_cross_prepare", "elapsed_ms",
                         itoa_buf((int)elapsed_ms(*t)));
            log_phase_mem("lsp_cross_prepare");
            break;
        case CBM_RS_PLAN_STEP_PARALLEL_RESOLVE:
            rc = cbm_parallel_resolve(ctx, files, file_count, cache, shared_ids, worker_count,
                                      all_defs, def_count, def_modules, module_def_index,
                                      &cross_registries);
            cbm_log_info("pass.timing", "pass", "parallel_resolve", "elapsed_ms",
                         itoa_buf((int)elapsed_ms(*t)));
            log_phase_mem("parallel_resolve");
            if (rc != 0 || check_cancel(p)) {
                rc = rc != 0 ? rc : CBM_NOT_FOUND;
                stop = true;
            }
            if (cross_state_ready) {
                cbm_pxc_free_module_def_index(module_def_index);
                module_def_index = NULL;
                if (def_modules) {
                    for (int i = 0; i < file_count; i++) {
                        free(def_modules[i]);
                    }
                    free(def_modules);
                    def_modules = NULL;
                }
                free(all_defs);
                all_defs = NULL;
                cbm_arena_destroy(&cross_lsp_arena);
                cross_state_ready = false;
                cross_state_cleaned = true;
            }
            cbm_gbuf_set_next_id(p->gbuf, atomic_load(shared_ids));
            break;
        case CBM_RS_PLAN_STEP_PARALLEL_INFRA_ROUTES:
            cbm_pipeline_extract_infra_routes(p->gbuf, files, cache, file_count);
            cbm_log_info("pass.timing", "pass", "infra_routes", "elapsed_ms",
                         itoa_buf((int)elapsed_ms(*t)));
            break;
        case CBM_RS_PLAN_STEP_PARALLEL_INFRA_BINDINGS:
            cbm_pipeline_process_infra_bindings(p->gbuf, files, cache, file_count);
            cbm_log_info("pass.timing", "pass", "infra_bindings", "elapsed_ms",
                         itoa_buf((int)elapsed_ms(*t)));
            break;
        case CBM_RS_PLAN_STEP_PARALLEL_K8S:
            cbm_pipeline_pass_k8s(ctx, files, file_count);
            cbm_log_info("pass.timing", "pass", "k8s", "elapsed_ms", itoa_buf((int)elapsed_ms(*t)));
            break;
        default:
            rc = CBM_NOT_FOUND;
            stop = true;
            break;
        }
    }

    if (cross_state_ready) {
        cbm_pxc_free_module_def_index(module_def_index);
        if (def_modules) {
            for (int i = 0; i < file_count; i++) {
                free(def_modules[i]);
            }
            free(def_modules);
        }
        free(all_defs);
        cbm_arena_destroy(&cross_lsp_arena);
    } else if (!cross_state_cleaned) {
        cbm_arena_destroy(&cross_lsp_arena);
    }

    if (check_cancel(p) && rc == 0) {
        rc = CBM_NOT_FOUND;
    }
    for (int i = 0; i < file_count; i++) {
        if (cache[i]) {
            cbm_free_result(cache[i]);
        }
    }
    free(cache);
    *out_rc = rc;
    return true;
}
#endif

/* Run the parallel pipeline path: extract, registry, resolve, infra, k8s. */
static int run_parallel_pipeline(cbm_pipeline_t *p, cbm_pipeline_ctx_t *ctx,
                                 const cbm_file_info_t *files, int file_count, int worker_count,
                                 struct timespec *t) {
    cbm_log_info("pipeline.mode", "mode", "parallel", "workers", itoa_buf(worker_count), "files",
                 itoa_buf(file_count));
    _Atomic int64_t shared_ids;
    atomic_init(&shared_ids, cbm_gbuf_next_id(p->gbuf));
    CBMFileResult **cache = (CBMFileResult **)calloc(file_count, sizeof(CBMFileResult *));
    if (!cache) {
        cbm_log_error("pipeline.err", "phase", "cache_alloc");
        return CBM_NOT_FOUND;
    }
#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
    int rc = 0;
    if (run_rust_parallel_dispatch(p, ctx, files, file_count, worker_count, cache, &shared_ids, t,
                                   &rc)) {
        return rc;
    }
#if defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
    cbm_log_error("rust_plan.err", "phase", "parallel_extract", "path", "rust_plan_only");
    free(cache);
    return CBM_NOT_FOUND;
#else
    cbm_log_warn("rust_plan.fallback", "phase", "parallel_extract", "reason",
                 "typed_v2_unavailable", "path", "c_parallel");
#endif
#else
    int rc = 0;
#endif
    cbm_clock_gettime(CLOCK_MONOTONIC, t);
    rc = cbm_parallel_extract(ctx, files, file_count, cache, &shared_ids, worker_count);
    cbm_log_info("pass.timing", "pass", "parallel_extract", "elapsed_ms",
                 itoa_buf((int)elapsed_ms(*t)));
    if (rc != 0 || check_cancel(p)) {
        for (int i = 0; i < file_count; i++) {
            if (cache[i]) {
                cbm_free_result(cache[i]);
            }
        }
        free(cache);
        return rc != 0 ? rc : CBM_NOT_FOUND;
    }
    cbm_gbuf_set_next_id(p->gbuf, atomic_load(&shared_ids));
    /* extract -> registry handoff: return the extract phase's freed-but-retained
     * allocator pages to the OS before registry_build allocates. On a 2x Linux
     * index the extract peak holds ~13 GB of reclaimable pages (peak_mb 20.7 vs
     * live rss_mb 7); not returning them pushed the process over the system
     * memory-pressure threshold and got it SIGKILLed at registry entry. */
    cbm_mem_collect();
    cbm_log_info("mem.collect", "phase", "post_extract", "rss_mb",
                 itoa_buf((int)(cbm_mem_rss() / (1024 * 1024))));
    cbm_clock_gettime(CLOCK_MONOTONIC, t);
    rc = cbm_build_registry_from_cache(ctx, files, file_count, cache);
    cbm_log_info("pass.timing", "pass", "registry_build", "elapsed_ms",
                 itoa_buf((int)elapsed_ms(*t)));
    log_phase_mem("registry_build");
    if (rc != 0 || check_cancel(p)) {
        for (int i = 0; i < file_count; i++) {
            if (cache[i]) {
                cbm_free_result(cache[i]);
            }
        }
        free(cache);
        return rc != 0 ? rc : CBM_NOT_FOUND;
    }
    /* Cross-file LSP precondition: build a project-wide CBMLSPDef[]
     * once. The fused resolve_worker invokes cbm_pxc_run_one(_ts) per
     * file using these defs + the file's IMPORTS map, so cross-file
     * type-resolved CALLS land in result->resolved_calls before the
     * CALLS-edge emission. This replaces the old sequential
     * cbm_pipeline_pass_lsp_cross pass which re-read every source from
     * disk and re-parsed every tree on a single thread (~520s on
     * kubernetes). Soft-failure: NULL all_defs / NULL def_modules just
     * mean cross-file LSP no-ops; per-file LSP already ran during
     * extract. */
    cbm_clock_gettime(CLOCK_MONOTONIC, t);
    /* Cross-file LSP (type-aware call/usage resolution across files) — the
     * most expensive phase. CBM_DISABLE_LSP_CROSS=1 opts out (it can SIGSEGV
     * on large TS projects — see #340/#344); with cross-LSP off, all_defs
     * stays NULL and the fused resolver simply no-ops cross-file resolution
     * (per-file LSP already ran during extract). */
    char cbm_lsp_cross_env[CBM_SZ_16];
    const bool run_cross_lsp = cbm_safe_getenv("CBM_DISABLE_LSP_CROSS", cbm_lsp_cross_env,
                                               sizeof(cbm_lsp_cross_env), NULL) == NULL;
    if (!run_cross_lsp) {
        cbm_log_info("lsp_cross.skipped", "reason", "CBM_DISABLE_LSP_CROSS env set");
    }
    char **def_modules = NULL;
    int def_count = 0;
    CBMLSPDef *all_defs = NULL;
    if (run_cross_lsp) {
        def_modules = (char **)calloc((size_t)file_count, sizeof(char *));
        all_defs = def_modules
                       ? cbm_pxc_collect_all_defs(cache, files, file_count, ctx->project_name,
                                                  def_modules, &def_count)
                       : NULL;
    }
    /* Build inverted index: module_qn → defs. The fused resolve_worker
     * uses this to filter the global all_defs[] down to just the defs
     * each file actually needs (own_module + imported modules) — the
     * gopls "package summary" pattern. Drops per-file registry build
     * cost from O(all_defs) to O(relevant_defs), typically 50-100×
     * smaller per file. */
    CBMModuleDefIndex *module_def_index =
        all_defs ? cbm_pxc_build_module_def_index(all_defs, def_count) : NULL;
    /* Tier 2 full: pre-build per-language cross-LSP registries.
     * Built ONCE here; shared READ-ONLY across all files of that language
     * during resolve. Per-file work is then: parse + AST walk + O(1) lookups
     * — no registry build, no Phase 1b mutations. Languages added so far:
     * Go, Python. Others (C/C++, TS/JS, PHP, C#) fall back to per-file. */
    CBMArena cross_lsp_arena;
    cbm_arena_init(&cross_lsp_arena);
    CBMCrossLspRegistries cross_registries = {0};
    if (all_defs) {
        cross_registries.go = cbm_go_build_cross_registry(&cross_lsp_arena, all_defs, def_count);
        cross_registries.python =
            cbm_py_build_cross_registry(&cross_lsp_arena, all_defs, def_count);
        cross_registries.c = cbm_c_build_cross_registry(&cross_lsp_arena, all_defs, def_count);
        cross_registries.cs = cbm_cs_build_cross_registry(&cross_lsp_arena, all_defs, def_count);
        cross_registries.ts = cbm_ts_build_cross_registry(&cross_lsp_arena, all_defs, def_count);
    }
    cbm_log_info("pass.timing", "pass", "lsp_cross_prepare", "elapsed_ms",
                 itoa_buf((int)elapsed_ms(*t)));
    log_phase_mem("lsp_cross_prepare");
    cbm_clock_gettime(CLOCK_MONOTONIC, t);
    rc = cbm_parallel_resolve(ctx, files, file_count, cache, &shared_ids, worker_count, all_defs,
                              def_count, def_modules, module_def_index, &cross_registries);
    cbm_log_info("pass.timing", "pass", "parallel_resolve", "elapsed_ms",
                 itoa_buf((int)elapsed_ms(*t)));
    log_phase_mem("parallel_resolve");
    cbm_pxc_free_module_def_index(module_def_index);
    cbm_arena_destroy(&cross_lsp_arena); /* releases all per-lang registries */
    free(all_defs);
    if (def_modules) {
        for (int i = 0; i < file_count; i++) {
            free(def_modules[i]);
        }
        free(def_modules);
    }
    cbm_gbuf_set_next_id(p->gbuf, atomic_load(&shared_ids));
    cbm_pipeline_extract_infra_routes(p->gbuf, files, cache, file_count);
    cbm_pipeline_process_infra_bindings(p->gbuf, files, cache, file_count);
    for (int i = 0; i < file_count; i++) {
        if (cache[i]) {
            cbm_free_result(cache[i]);
        }
    }
    free(cache);
    if (rc != 0) {
        return rc;
    }
    cbm_clock_gettime(CLOCK_MONOTONIC, t);
    cbm_pipeline_pass_k8s(ctx, files, file_count);
    cbm_log_info("pass.timing", "pass", "k8s", "elapsed_ms", itoa_buf((int)elapsed_ms(*t)));
    return check_cancel(p) ? CBM_NOT_FOUND : 0;
}

/* Try incremental pipeline or delete old DB for reindex.
 * Returns PL_INCREMENTAL_FALLBACK to proceed with full; otherwise returns
 * the incremental run's actual status, including negative errors. */
static int try_incremental_or_delete_db(cbm_pipeline_t *p, cbm_file_info_t *files, int file_count) {
    char *db_path = resolve_db_path(p);
    if (!db_path) {
        return CBM_NOT_FOUND;
    }
    struct stat db_st;
    if (stat(db_path, &db_st) != 0) {
        free(db_path);
        return PL_INCREMENTAL_FALLBACK;
    }
    cbm_store_t *check_store = cbm_store_open_path(db_path);
    if (check_store && cbm_store_check_integrity(check_store)) {
        cbm_file_hash_t *hashes = NULL;
        int hash_count = 0;
        cbm_store_get_file_hashes(check_store, p->project_name, &hashes, &hash_count);
        cbm_store_free_file_hashes(hashes, hash_count);
        cbm_store_close(check_store);
        if (hash_count > 0 && file_count <= hash_count + (hash_count / PAIR_LEN)) {
            cbm_log_info("pipeline.route", "path", "incremental", "stored_hashes",
                         itoa_buf(hash_count));
            int rc = cbm_pipeline_run_incremental(p, db_path, files, file_count);
            free(db_path);
            return rc;
        }
        if (hash_count > 0) {
            cbm_log_info("pipeline.route", "path", "mode_change_reindex", "stored_hashes",
                         itoa_buf(hash_count), "discovered", itoa_buf(file_count));
        }
    } else if (check_store) {
        cbm_store_close(check_store);
    }
    cbm_log_info("pipeline.route", "path", "reindex", "action", "deleting old db");
    /* Capture any ADR before deleting the DB so the full-reindex rebuild can
     * restore it (project_summaries is otherwise lost). Issue #516. */
    {
        cbm_store_t *adr_store = cbm_store_open_path(db_path);
        if (adr_store) {
            cbm_adr_t existing;
            if (cbm_store_adr_get(adr_store, p->project_name, &existing) == CBM_STORE_OK) {
                if (existing.content) {
                    free(p->saved_adr);
                    p->saved_adr = strdup(existing.content);
                }
                cbm_store_adr_free(&existing);
            }
            cbm_store_close(adr_store);
        }
    }
    cbm_unlink(db_path);
    char wal[PL_WAL_BUF];
    char shm[PL_WAL_BUF];
    snprintf(wal, sizeof(wal), "%s-wal", db_path);
    snprintf(shm, sizeof(shm), "%s-shm", db_path);
    cbm_unlink(wal);
    cbm_unlink(shm);
    free(db_path);
    return PL_INCREMENTAL_FALLBACK;
}

/* Get platform-specific mtime in nanoseconds. */
static int64_t stat_mtime_ns(const struct stat *fst) {
#ifdef __APPLE__
    return ((int64_t)fst->st_mtimespec.tv_sec * PL_NSEC_PER_SEC) +
           (int64_t)fst->st_mtimespec.tv_nsec;
#elif defined(_WIN32)
    return (int64_t)fst->st_mtime * 1000000000LL;
#else
    return ((int64_t)fst->st_mtim.tv_sec * PL_NSEC_PER_SEC) + (int64_t)fst->st_mtim.tv_nsec;
#endif
}

/* Dump graph to SQLite and persist file hashes for incremental indexing. */
static int dump_and_persist_hashes(cbm_pipeline_t *p, const cbm_file_info_t *files, int file_count,
                                   struct timespec *t) {
    cbm_clock_gettime(CLOCK_MONOTONIC, t);
    char db_path[CBM_SZ_1K];
    if (p->db_path) {
        snprintf(db_path, sizeof(db_path), "%s", p->db_path);
    } else {
        const char *cdir = cbm_resolve_cache_dir();
        if (!cdir) {
            cdir = cbm_tmpdir();
        }
        snprintf(db_path, sizeof(db_path), "%s/%s.db", cdir, p->project_name);
    }
    char db_dir[CBM_SZ_1K];
    snprintf(db_dir, sizeof(db_dir), "%s", db_path);
    char *last_slash = strrchr(db_dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        cbm_mkdir_p(db_dir, CBM_DIR_PERMS);
    }
    /* Capture committed counts BEFORE the dump. cbm_gbuf_dump_to_sqlite calls
     * release_gbuf_indexes(), which frees node_by_qn (graph_buffer.c), after
     * which cbm_gbuf_node_count() returns 0. Reading these post-dump left
     * committed_nodes at 0, so the #334 plausibility gate never fired. */
    p->committed_nodes = cbm_gbuf_node_count(p->gbuf);
    p->committed_edges = cbm_gbuf_edge_count(p->gbuf);
    int rc = cbm_gbuf_dump_to_sqlite(p->gbuf, db_path);
    if (rc != 0) {
        cbm_log_error("pipeline.err", "phase", "dump");
        return rc;
    }
    cbm_log_info("pass.timing", "pass", "dump", "elapsed_ms", itoa_buf((int)elapsed_ms(*t)));
    cbm_store_t *hash_store = cbm_store_open_path(db_path);
    if (hash_store) {
        cbm_store_delete_file_hashes(hash_store, p->project_name);

        /* Restore the ADR captured before the dump. Surface a failed restore
         * rather than silently dropping the ADR (the original #516 symptom). */
        if (p->saved_adr) {
            if (cbm_store_adr_store(hash_store, p->project_name, p->saved_adr) != CBM_STORE_OK) {
                cbm_log_error("pipeline.err", "phase", "adr_restore", "project", p->project_name);
            }
        }
        for (int i = 0; i < file_count; i++) {
            struct stat fst;
            if (stat(files[i].path, &fst) == 0) {
                cbm_store_upsert_file_hash(hash_store, p->project_name, files[i].rel_path, "",
                                           stat_mtime_ns(&fst), fst.st_size);
            }
        }

        /* FTS5 backfill: populate nodes_fts with camelCase-split names.
         * Contentless FTS5 requires the special 'delete-all' command instead of
         * DELETE FROM to wipe prior rows (there's no underlying content table).
         * Falls back to plain names if cbm_camel_split is unavailable (which
         * shouldn't happen because we always register it, but we stay defensive). */
        cbm_store_exec(hash_store, "INSERT INTO nodes_fts(nodes_fts) VALUES('delete-all');");
        if (cbm_store_exec(hash_store,
                           "INSERT INTO nodes_fts(rowid, name, qualified_name, label, file_path) "
                           "SELECT id, cbm_camel_split(name), qualified_name, label, file_path "
                           "FROM nodes;") != CBM_STORE_OK) {
            cbm_store_exec(hash_store,
                           "INSERT INTO nodes_fts(rowid, name, qualified_name, label, file_path) "
                           "SELECT id, name, qualified_name, label, file_path FROM nodes;");
        }

        cbm_store_close(hash_store);
        cbm_log_info("pass.timing", "pass", "persist_hashes", "files", itoa_buf(file_count));
    }
    free(p->saved_adr);
    p->saved_adr = NULL;

    /* Export persistent artifact if enabled */
    if (p->persistence) {
        int arc = cbm_artifact_export(db_path, p->repo_path, p->project_name, CBM_ARTIFACT_BEST);
        if (arc != 0) {
            const char *err = cbm_artifact_export_last_error();
            cbm_log_error("pipeline.err", "phase", "artifact_export", "err", err ? err : "unknown");
            /* A failed persistence export intentionally fails the run; this used to be ignored. */
            return arc;
        }
    }

    return 0;
}

/* Run githistory pass. */
static int run_githistory(cbm_pipeline_t *p, cbm_pipeline_ctx_t *ctx) {
    struct timespec t_gh;
    cbm_clock_gettime(CLOCK_MONOTONIC, &t_gh);

    cbm_githistory_result_t gh_result = {0};
    cbm_thread_t gh_thread;
    bool gh_threaded = false;
    gh_compute_arg_t gh_arg = {.repo_path = ctx->repo_path, .result = &gh_result};

    if (p->mode != CBM_MODE_FAST) {
        if (cbm_default_worker_count(true) > SKIP_ONE) {
            if (cbm_thread_create(&gh_thread, 0, gh_compute_thread_fn, &gh_arg) == 0) {
                gh_threaded = true;
            }
        }
        if (!gh_threaded) {
            cbm_pipeline_githistory_compute(ctx->repo_path, &gh_result);
            cbm_log_info("pass.timing", "pass", "githistory_compute", "elapsed_ms",
                         itoa_buf((int)elapsed_ms(t_gh)));
        }
    } else {
        cbm_log_info("pass.skip", "pass", "githistory", "reason", "fast_mode");
    }

    if (gh_threaded) {
        cbm_thread_join(&gh_thread);
        cbm_log_info("pass.timing", "pass", "githistory_compute", "elapsed_ms",
                     itoa_buf((int)elapsed_ms(t_gh)));
    }

    int gh_edges = 0;
    if (gh_result.count > 0 || gh_result.file_temporal_count > 0) {
        gh_edges = cbm_pipeline_githistory_apply(ctx, &gh_result);
    }
    cbm_log_info("pass.done", "pass", "githistory", "commits", itoa_buf(gh_result.commit_count),
                 "edges", itoa_buf(gh_edges));
    free(gh_result.couplings);
    free(gh_result.file_temporal);
    return 0;
}

/* ── Pipeline run ────────────────────────────────────────────────── */

/* Run tests + git history. Returns 0 on success. */
static int run_tests_and_history(cbm_pipeline_t *p, cbm_pipeline_ctx_t *ctx,
                                 const cbm_file_info_t *files, int file_count) {
    struct timespec t;
    cbm_clock_gettime(CLOCK_MONOTONIC, &t);
    CBM_PROF_START(t_tests);
    int rc = cbm_pipeline_pass_tests(ctx, files, file_count);
    CBM_PROF_END_N("pipeline", "pass_tests", t_tests, file_count);
    cbm_log_info("pass.timing", "pass", "tests", "elapsed_ms", itoa_buf((int)elapsed_ms(t)));
    if (rc == 0 && !check_cancel(p)) {
        CBM_PROF_START(t_gh);
        rc = run_githistory(p, ctx);
        CBM_PROF_END("pipeline", "pass_githistory", t_gh);
    }
    if (check_cancel(p)) {
        return CBM_NOT_FOUND;
    }
    return rc;
}

/* Run tests, git history, predump passes, and dump+persist. */
static int run_post_extraction(cbm_pipeline_t *p, cbm_pipeline_ctx_t *ctx,
                               const cbm_file_info_t *files, int file_count) {
    int rc = run_tests_and_history(p, ctx, files, file_count);
    if (rc != 0) {
        return rc;
    }

    CBM_PROF_START(t_predump);
    run_predump_passes(p, ctx);
    CBM_PROF_END("pipeline", "3_predump_passes_total", t_predump);

    if (!check_cancel(p)) {
        struct timespec t;
        CBM_PROF_START(t_dump);
        rc = dump_and_persist_hashes(p, files, file_count, &t);
        CBM_PROF_END("pipeline", "4_dump_and_persist", t_dump);
    }
    return rc;
}

#define MIN_FILES_FOR_PARALLEL 50

#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
typedef struct {
    int kind;
    const char *name;
    bool skip_fast;
    int policy;
    uint32_t gate_flags;
    uint32_t effect_flags;
    int nested_plan_kind;
} top_plan_step_def_t;

typedef struct {
    int step_count;
    int extraction_nested_plan_kind;
    char plan[CBM_RS_PLAN_TOP_BUF];
} top_plan_runtime_t;

enum { TOP_PLAN_DYNAMIC_EXTRACTION_NESTED = -2 };

static const top_plan_step_def_t top_plan_steps[] = {
    {CBM_RS_PLAN_TOP_MACRO_EXTRACTION, "macro_extraction", false,
     CBM_RS_PLAN_TOP_POLICY_FULL_MODE_ONLY, 0, 0, CBM_RS_PLAN_TOP_NO_NESTED_PLAN},
    {CBM_RS_PLAN_TOP_USERCONFIG_LOAD, "userconfig_load", false, CBM_RS_PLAN_TOP_POLICY_FAIL_OPEN, 0,
     0, CBM_RS_PLAN_TOP_NO_NESTED_PLAN},
    {CBM_RS_PLAN_TOP_DISCOVER, "discover", false, CBM_RS_PLAN_TOP_POLICY_REQUIRED, 0, 0,
     CBM_RS_PLAN_TOP_NO_NESTED_PLAN},
    {CBM_RS_PLAN_TOP_TRY_INCREMENTAL_OR_DELETE_DB, "try_incremental_or_delete_db", false,
     CBM_RS_PLAN_TOP_POLICY_EXISTING_DB_ONLY, CBM_RS_PLAN_TOP_GATE_MAY_SHORT_CIRCUIT,
     CBM_RS_PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT, CBM_RS_PLAN_TOP_NO_NESTED_PLAN},
    {CBM_RS_PLAN_TOP_STRUCTURE, "structure", false, CBM_RS_PLAN_TOP_POLICY_REQUIRED, 0,
     CBM_RS_PLAN_TOP_EFFECT_MUTATES_GRAPH, CBM_RS_PLAN_TOP_NO_NESTED_PLAN},
    {CBM_RS_PLAN_TOP_EXTRACTION_DISPATCH, "extraction_dispatch", false,
     CBM_RS_PLAN_TOP_POLICY_REQUIRED, 0, CBM_RS_PLAN_TOP_EFFECT_MUTATES_GRAPH,
     TOP_PLAN_DYNAMIC_EXTRACTION_NESTED},
    {CBM_RS_PLAN_TOP_TESTS, "tests", false, CBM_RS_PLAN_TOP_POLICY_REQUIRED, 0,
     CBM_RS_PLAN_TOP_EFFECT_MUTATES_GRAPH, CBM_RS_PLAN_TOP_NO_NESTED_PLAN},
    {CBM_RS_PLAN_TOP_GITHISTORY, "githistory", true, CBM_RS_PLAN_TOP_POLICY_REQUIRED,
     CBM_RS_PLAN_TOP_GATE_SKIP_FAST, CBM_RS_PLAN_TOP_EFFECT_MUTATES_GRAPH,
     CBM_RS_PLAN_TOP_NO_NESTED_PLAN},
    {CBM_RS_PLAN_TOP_PREDUMP, "predump", false, CBM_RS_PLAN_TOP_POLICY_REQUIRED, 0,
     CBM_RS_PLAN_TOP_EFFECT_MUTATES_GRAPH, CBM_RS_PLAN_PREDUMP},
    {CBM_RS_PLAN_TOP_DUMP, "dump", false, CBM_RS_PLAN_TOP_POLICY_REQUIRED, 0,
     CBM_RS_PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT, CBM_RS_PLAN_TOP_NO_NESTED_PLAN},
    {CBM_RS_PLAN_TOP_PERSIST_HASHES, "persist_hashes", false, CBM_RS_PLAN_TOP_POLICY_BEST_EFFORT, 0,
     CBM_RS_PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT, CBM_RS_PLAN_TOP_NO_NESTED_PLAN},
    {CBM_RS_PLAN_TOP_ARTIFACT_EXPORT, "artifact_export", false,
     CBM_RS_PLAN_TOP_POLICY_OPTIONAL_PERSISTENCE, 0,
     CBM_RS_PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT, CBM_RS_PLAN_TOP_NO_NESTED_PLAN},
};
enum { TOP_PLAN_STEP_COUNT = (int)(sizeof(top_plan_steps) / sizeof(top_plan_steps[0])) };

static uint64_t top_plan_step_bit(int kind) {
    if (kind <= 0 || kind > CBM_RS_PLAN_TOP_ARTIFACT_EXPORT) {
        return 0;
    }
    return UINT64_C(1) << (uint32_t)(kind - 1);
}

static bool append_top_plan_name(char *buf, size_t bufsize, size_t *used, const char *name) {
    int written = snprintf(buf + *used, bufsize - *used, "%s%s", *used == 0 ? "" : ",", name);
    if (written < 0 || (size_t)written >= bufsize - *used) {
        return false;
    }
    *used += (size_t)written;
    return true;
}

static bool top_plan_nested_kind_matches(const top_plan_step_def_t *expected,
                                         const cbm_rs_pipeline_top_step_v1_t *step) {
    if (expected->nested_plan_kind == TOP_PLAN_DYNAMIC_EXTRACTION_NESTED) {
        return step->nested_plan_kind == CBM_RS_PLAN_SEQUENTIAL ||
               step->nested_plan_kind == CBM_RS_PLAN_PARALLEL_EXTRACTION;
    }
    return step->nested_plan_kind == expected->nested_plan_kind;
}

static bool decode_rust_top_plan(const cbm_rs_pipeline_top_step_v1_t *steps, int step_count,
                                 int mode, top_plan_runtime_t *out) {
    if (!steps || step_count <= 0 || !out) {
        return false;
    }
    int expected_total = mode == CBM_MODE_FAST ? TOP_PLAN_STEP_COUNT - 1 : TOP_PLAN_STEP_COUNT;
    if (step_count != expected_total) {
        return false;
    }

    uint64_t seen_mask = 0;
    size_t used = 0;
    int expected_idx = 0;
    memset(out, 0, sizeof(*out));
    out->extraction_nested_plan_kind = CBM_RS_PLAN_TOP_NO_NESTED_PLAN;
    out->plan[0] = '\0';

    for (int i = 0; i < step_count; i++) {
        while (expected_idx < TOP_PLAN_STEP_COUNT && top_plan_steps[expected_idx].skip_fast &&
               mode == CBM_MODE_FAST) {
            expected_idx++;
        }
        if (expected_idx >= TOP_PLAN_STEP_COUNT) {
            return false;
        }

        const top_plan_step_def_t *expected = &top_plan_steps[expected_idx];
        const cbm_rs_pipeline_top_step_v1_t *step = &steps[i];
        uint64_t bit = top_plan_step_bit(step->kind);
        if (step->kind != expected->kind || step->phase != CBM_RS_PLAN_TOP_PHASE_FULL_PIPELINE ||
            step->policy != expected->policy || step->gate_flags != expected->gate_flags ||
            step->effect_flags != expected->effect_flags ||
            !top_plan_nested_kind_matches(expected, step) || bit == 0 || (seen_mask & bit) != 0 ||
            step->requires_mask != seen_mask ||
            !append_top_plan_name(out->plan, sizeof(out->plan), &used, expected->name)) {
            return false;
        }

        if (step->kind == CBM_RS_PLAN_TOP_EXTRACTION_DISPATCH) {
            out->extraction_nested_plan_kind = step->nested_plan_kind;
        }
        seen_mask |= bit;
        expected_idx++;
    }

    out->step_count = step_count;
    return true;
}

static bool load_rust_top_plan(int mode, int worker_count, int file_count,
                               top_plan_runtime_t *out) {
    cbm_rs_pipeline_top_step_v1_t steps[CBM_RS_PLAN_TOP_MAX_STEPS];
    int step_count = 0;
    return cbm_rust_full_plan_steps(mode, worker_count, file_count, steps,
                                    CBM_RS_PLAN_TOP_MAX_STEPS, &step_count) &&
           decode_rust_top_plan(steps, step_count, mode, out);
}
#endif

/* Run structure + extraction passes (parallel or sequential). */
static int run_extraction_phase(cbm_pipeline_t *p, cbm_pipeline_ctx_t *ctx,
                                const cbm_file_info_t *files, int file_count, int worker_count,
                                int rust_top_extraction_plan_kind) {
    struct timespec t;
    cbm_clock_gettime(CLOCK_MONOTONIC, &t);
    CBM_PROF_START(t_struct);
    pass_structure(p, files, file_count);
    CBM_PROF_END_N("pipeline", "pass_structure", t_struct, file_count);
    cbm_log_info("pass.timing", "pass", "structure", "elapsed_ms", itoa_buf((int)elapsed_ms(t)));
    if (check_cancel(p)) {
        return CBM_NOT_FOUND;
    }

    CBM_PROF_START(t_extract_total);
    bool use_parallel = worker_count > SKIP_ONE && file_count > MIN_FILES_FOR_PARALLEL;
#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
    bool rust_use_parallel = false;
    if (rust_top_extraction_plan_kind == CBM_RS_PLAN_PARALLEL_EXTRACTION) {
        use_parallel = true;
        cbm_log_info("rust_plan.dispatch", "phase", "extraction_choice", "decision", "parallel",
                     "source", "top_v1");
    } else if (rust_top_extraction_plan_kind == CBM_RS_PLAN_SEQUENTIAL) {
        use_parallel = false;
        cbm_log_info("rust_plan.dispatch", "phase", "extraction_choice", "decision", "sequential",
                     "source", "top_v1");
    } else if (cbm_rust_plan_extracts_parallel(p->mode, worker_count, file_count,
                                               &rust_use_parallel)) {
        use_parallel = rust_use_parallel;
        cbm_log_info("rust_plan.dispatch", "phase", "extraction_choice", "decision",
                     use_parallel ? "parallel" : "sequential", "source", "choice_v1");
    }
#else
    (void)rust_top_extraction_plan_kind;
#endif
    int rc = use_parallel ? run_parallel_pipeline(p, ctx, files, file_count, worker_count, &t)
                          : run_sequential_pipeline(p, ctx, files, file_count, &t);
    CBM_PROF_END_N("pipeline", "2_extraction_total", t_extract_total, file_count);
    if (check_cancel(p)) {
        return CBM_NOT_FOUND;
    }
    return rc;
}

int cbm_pipeline_run(cbm_pipeline_t *p) {
    if (!p) {
        return CBM_NOT_FOUND;
    }

    CBM_PROF_START(t_pipeline_total);
    struct timespec t0;
    cbm_clock_gettime(CLOCK_MONOTONIC, &t0);
    cbm_path_alias_collection_t *path_aliases = NULL;
    int rust_top_extraction_plan_kind = CBM_RS_PLAN_TOP_NO_NESTED_PLAN;

    /* C/C++ #define Macro nodes (#375) dominate extraction on macro-dense repos
     * (≈49% of nodes on the Linux kernel), so gate them to full mode — moderate
     * and fast skip them entirely. Set before any extraction dispatch. */
    cbm_set_macro_extraction(p->mode == CBM_MODE_FULL);

    /* Load user-defined extension overrides (fail-open: NULL on error) */
    CBM_PROF_START(t_userconfig);
    p->userconfig = cbm_userconfig_load(p->repo_path);
    cbm_set_user_lang_config(p->userconfig);
    CBM_PROF_END("pipeline", "0_userconfig_load", t_userconfig);

    /* Phase 1: Discover files */
    CBM_PROF_START(t_discover);
    cbm_discover_opts_t opts = {
        .mode = p->mode,
        .ignore_file = NULL,
        .max_file_size = 0,
    };
    cbm_file_info_t *files = NULL;
    int file_count = 0;
    /* Capture skipped subtrees on the pipeline so the MCP layer can report
     * which directories were excluded (#411). Replace any prior list (e.g. a
     * re-run on the same pipeline) to avoid leaking the previous one. */
    cbm_discover_free_excluded(p->excluded_dirs, p->excluded_count);
    p->excluded_dirs = NULL;
    p->excluded_count = 0;
    int rc = cbm_discover_ex(p->repo_path, &opts, &files, &file_count, &p->excluded_dirs,
                             &p->excluded_count);
    if (rc != 0) {
        cbm_log_error("pipeline.err", "phase", "discover", "rc", itoa_buf(rc));
    }
    CBM_PROF_END_N("pipeline", "1_discover", t_discover, file_count);
    cbm_log_info("pipeline.discover", "files", itoa_buf(file_count), "elapsed_ms",
                 itoa_buf((int)elapsed_ms(t0)));
    if (rc != 0 || check_cancel(p)) {
        rc = CBM_NOT_FOUND;
        goto cleanup;
    }

#if defined(CBM_USE_RUST_PIPELINE_PLAN) || defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
    int worker_count = cbm_default_worker_count(true);
    top_plan_runtime_t rust_top_plan;
    if (load_rust_top_plan(p->mode, worker_count, file_count, &rust_top_plan)) {
        rust_top_extraction_plan_kind = rust_top_plan.extraction_nested_plan_kind;
        cbm_log_info("rust_plan.dispatch", "phase", "full_pipeline", "passes",
                     itoa_buf(rust_top_plan.step_count), "plan", rust_top_plan.plan, "source",
                     "top_v1");
    } else {
#if defined(CBM_USE_RUST_PIPELINE_PLAN_ONLY)
        cbm_log_error("rust_plan.err", "phase", "full_pipeline", "path", "rust_plan_only");
        rc = CBM_NOT_FOUND;
        goto cleanup;
#else
        cbm_log_warn("rust_plan.fallback", "phase", "full_pipeline", "reason", "top_v1_unavailable",
                     "path", "c_orchestrator");
#endif
    }
#else
    int worker_count = cbm_default_worker_count(true);
#endif

    /* Check for existing DB → try incremental or delete for reindex */
    rc = try_incremental_or_delete_db(p, files, file_count);
    if (rc != PL_INCREMENTAL_FALLBACK) {
        goto cleanup;
    }
    cbm_log_info("pipeline.route", "path", "full");

    /* Phase 2: Create graph buffer and registry */
    p->gbuf = cbm_gbuf_new(p->project_name, p->repo_path);
    p->registry = cbm_registry_new();

    /* Phase 2b: Load build-tool path aliases (tsconfig/jsconfig today). NULL
     * when no usable configs are found — non-TS projects pay nothing. */
    path_aliases = cbm_load_path_aliases(p->repo_path);

    /* Build shared context for pass functions */
    cbm_pipeline_ctx_t ctx = {
        .project_name = p->project_name,
        .repo_path = p->repo_path,
        .gbuf = p->gbuf,
        .registry = p->registry,
        .cancelled = &p->cancelled,
        .mode = (int)p->mode,
        .path_aliases = path_aliases,
    };

    rc = run_extraction_phase(p, &ctx, files, file_count, worker_count,
                              rust_top_extraction_plan_kind);
    if (rc != 0) {
        goto cleanup;
    }

    rc = run_post_extraction(p, &ctx, files, file_count);
    if (rc != 0) {
        goto cleanup;
    }

    cbm_log_info("pipeline.done", "nodes", itoa_buf(cbm_gbuf_node_count(p->gbuf)), "edges",
                 itoa_buf(cbm_gbuf_edge_count(p->gbuf)), "elapsed_ms",
                 itoa_buf((int)elapsed_ms(t0)));
    CBM_PROF_END("pipeline", "TOTAL", t_pipeline_total);

cleanup:
    cbm_pkgmap_free(cbm_pipeline_get_pkgmap());
    cbm_pipeline_set_pkgmap(NULL);
    cbm_discover_free(files, file_count);
    cbm_gbuf_free(p->gbuf);
    p->gbuf = NULL;
    cbm_registry_free(p->registry);
    p->registry = NULL;
    cbm_path_alias_collection_free(path_aliases);
    /* Clear and free user extension config */
    cbm_set_user_lang_config(NULL);
    cbm_userconfig_free(p->userconfig);
    p->userconfig = NULL;
    return rc;
}
