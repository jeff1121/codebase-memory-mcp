/*
 * pass_enrichment.c — Decorator tokenization, camelCase splitting,
 * and decorator_tags pass (post-flush enrichment).
 *
 * Pure helper functions + a store-level pass that classifies decorators
 * into semantic tags via auto-discovery (words on 2+ nodes become tags).
 */
#include "foundation/constants.h"

#include "pipeline/pipeline.h"
#include <stdint.h>
#include "pipeline/pipeline_internal.h"
#include "graph_buffer/graph_buffer.h"
#include "foundation/log.h"
#include "foundation/hash_table.h"
#include "foundation/platform.h"
#include "foundation/compat.h"
#include "yyjson/yyjson.h"

#include <stdlib.h>
#include <string.h>

/* Convert intptr_t to void* without triggering performance-no-int-to-ptr. */
static inline void *intptr_to_ptr(intptr_t v) {
    void *p;
    memcpy(&p, &v, sizeof(p));
    return p;
}

/* ══════════════════════════════════════════════════════════════════
 *  Decorator Tags Pass (post-flush, operates on store)
 *
 *  Algorithm:
 *  1. Load all Function/Method/Class nodes from the store
 *  2. For each node with "decorators" property, tokenize decorators
 *  3. Count word frequency across all nodes
 *  4. Words on 2+ distinct nodes become candidates
 *  5. Update each node's properties_json with "decorator_tags" array
 * ══════════════════════════════════════════════════════════════════ */

/* Per-node tokenization state */
typedef struct {
    int64_t node_id;
    char *qualified_name;
    char **words;
    int word_count;
} tagged_node_t;

/* Extract the "decorators" array from a properties_json string.
 * Returns a NULL-terminated array of strings. Caller must free array and strings. */
static char **extract_decorators_from_json(const char *json) {
    if (!json) {
        return NULL;
    }

    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    if (!doc) {
        return NULL;
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *decs = yyjson_obj_get(root, "decorators");
    if (!decs || !yyjson_is_arr(decs)) {
        yyjson_doc_free(doc);
        return NULL;
    }

    size_t cnt = yyjson_arr_size(decs);
    if (cnt == 0) {
        yyjson_doc_free(doc);
        return NULL;
    }

    char **out = calloc(cnt + SKIP_ONE, sizeof(char *));
    size_t idx = 0;
    yyjson_val *item;
    yyjson_arr_iter iter;
    yyjson_arr_iter_init(decs, &iter);
    while ((item = yyjson_arr_iter_next(&iter))) {
        if (yyjson_is_str(item)) {
            out[idx++] = strdup(yyjson_get_str(item));
        }
    }
    out[idx] = NULL;

    yyjson_doc_free(doc);
    if (idx > 0) {
        return out;
    }
    free(out);
    return NULL;
}

/* Tokenize all decorators on a node into unique words.
 * Returns heap-allocated array, caller frees each word and the array. */
static int extract_decorator_words(const char *json, char ***out_words) {
    char **decorators = extract_decorators_from_json(json);
    if (!decorators) {
        *out_words = NULL;
        return 0;
    }

    /* Collect unique words from all decorators */
    char *all_words[CBM_SZ_256];
    int total = 0;
    CBMHashTable *seen = cbm_ht_create(CBM_SZ_32);

    for (int i = 0; decorators[i]; i++) {
        char *tokens[CBM_SZ_32];
        int tc = cbm_tokenize_decorator(decorators[i], tokens, CBM_SZ_32);
        for (int j = 0; j < tc; j++) {
            if (!cbm_ht_get(seen, tokens[j]) && total < CBM_SZ_256) {
                cbm_ht_set(seen, tokens[j], intptr_to_ptr(SKIP_ONE));
                all_words[total++] = tokens[j];
            } else {
                free(tokens[j]);
            }
        }
        free(decorators[i]);
    }
    free(decorators);
    cbm_ht_free(seen);

    if (total == 0) {
        *out_words = NULL;
        return 0;
    }

    *out_words = malloc(sizeof(char *) * total);
    memcpy(*out_words, all_words, sizeof(char *) * total);
    return total;
}

/* Insert "decorator_tags" into a properties_json string.
 * Returns a newly allocated JSON string. Caller must free(). */
static char *inject_decorator_tags(const char *json, char **tags, int tag_count) {
    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    yyjson_val *root = doc ? yyjson_doc_get_root(doc) : NULL;

    yyjson_mut_doc *mdoc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *mroot;

    if (root && yyjson_is_obj(root)) {
        mroot = yyjson_val_mut_copy(mdoc, root);
    } else {
        mroot = yyjson_mut_obj(mdoc);
    }
    yyjson_mut_doc_set_root(mdoc, mroot);

    /* Remove existing decorator_tags if any */
    yyjson_mut_obj_remove_key(mroot, "decorator_tags");

    /* Add sorted tag array */
    yyjson_mut_val *arr = yyjson_mut_arr(mdoc);
    for (int i = 0; i < tag_count; i++) {
        yyjson_mut_arr_add_str(mdoc, arr, tags[i]);
    }
    yyjson_mut_obj_add_val(mdoc, mroot, "decorator_tags", arr);

    char *result = yyjson_mut_write(mdoc, 0, NULL);
    yyjson_mut_doc_free(mdoc);
    yyjson_doc_free(doc);
    return result;
}

/* Simple string comparison for qsort */
static int cmp_str(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

/* Free tagged_node_t array. */
static void free_tagged_nodes(tagged_node_t *nodes, int count) {
    for (int n = 0; n < count; n++) {
        free(nodes[n].qualified_name);
        for (int w = 0; w < nodes[n].word_count; w++) {
            free(nodes[n].words[w]);
        }
        free(nodes[n].words);
    }
    free(nodes);
}

/* Phase 1: Collect decorated nodes and count word frequency. */
static int collect_decorated_nodes(cbm_gbuf_t *gbuf, tagged_node_t **out_nodes,
                                   CBMHashTable *word_counts) {
    /* "Struct" alongside "Class" so Go/Rust/Swift/D struct names keep
     * contributing to / receiving auto-tags as they did when structs were
     * labelled "Class". */
    static const char *labels[] = {"Function", "Method", "Class", "Struct"};
    static const int nlabels = 4;
    tagged_node_t *nodes = NULL;
    int node_count = 0;
    int node_cap = 0;

    for (int l = 0; l < nlabels; l++) {
        const cbm_gbuf_node_t **found = NULL;
        int fc = 0;
        cbm_gbuf_find_by_label(gbuf, labels[l], &found, &fc);
        if (!found || fc <= 0) {
            continue;
        }

        for (int i = 0; i < fc; i++) {
            char **words = NULL;
            int wc = extract_decorator_words(found[i]->properties_json, &words);
            if (wc <= 0) {
                continue;
            }
            if (node_count >= node_cap) {
                node_cap = node_cap ? node_cap * PAIR_LEN : CBM_SZ_64;
                nodes = safe_realloc(nodes, sizeof(tagged_node_t) * node_cap);
            }
            tagged_node_t *tn = &nodes[node_count++];
            tn->node_id = found[i]->id;
            tn->qualified_name = strdup(found[i]->qualified_name);
            tn->words = words;
            tn->word_count = wc;
            for (int w = 0; w < wc; w++) {
                intptr_t cnt = (intptr_t)cbm_ht_get(word_counts, words[w]);
                cbm_ht_set(word_counts, words[w], intptr_to_ptr(cnt + SKIP_ONE));
            }
        }
    }

    *out_nodes = nodes;
    return node_count;
}

/* Phase 3: Apply candidate tags to nodes in the gbuf. */
static int apply_decorator_tags(cbm_gbuf_t *gbuf, tagged_node_t *nodes, int node_count,
                                CBMHashTable *candidates) {
    int tagged = 0;
    for (int n = 0; n < node_count; n++) {
        char *tag_words[CBM_SZ_256];
        int tag_count = 0;
        for (int w = 0; w < nodes[n].word_count; w++) {
            if (cbm_ht_get(candidates, nodes[n].words[w]) && tag_count < CBM_SZ_256) {
                tag_words[tag_count++] = nodes[n].words[w];
            }
        }
        if (tag_count == 0) {
            continue;
        }
        qsort(tag_words, tag_count, sizeof(char *), cmp_str);

        const cbm_gbuf_node_t *gn = cbm_gbuf_find_by_qn(gbuf, nodes[n].qualified_name);
        if (!gn) {
            continue;
        }

        const char *props = gn->properties_json ? gn->properties_json : "{}";
        char *new_props = inject_decorator_tags(props, tag_words, tag_count);
        if (new_props) {
            cbm_gbuf_apply_upsert_node(gbuf, gn->label, gn->name, gn->qualified_name, gn->file_path,
                                       gn->start_line, gn->end_line, new_props);
            free(new_props);
            tagged++;
        }
    }
    return tagged;
}

int cbm_pipeline_pass_decorator_tags(cbm_gbuf_t *gbuf, const char *project) {
    if (!gbuf || !project) {
        return 0;
    }

    CBMHashTable *word_counts = cbm_ht_create(CBM_SZ_128);
    tagged_node_t *nodes = NULL;
    int node_count = collect_decorated_nodes(gbuf, &nodes, word_counts);
    if (node_count == 0) {
        cbm_ht_free(word_counts);
        free(nodes);
        return 0;
    }

    /* Phase 2: Determine candidates (words on 2+ nodes) */
    CBMHashTable *candidates = cbm_ht_create(CBM_SZ_64);
    int candidate_count = 0;
    for (int n = 0; n < node_count; n++) {
        for (int w = 0; w < nodes[n].word_count; w++) {
            const char *word = nodes[n].words[w];
            intptr_t cnt = (intptr_t)cbm_ht_get(word_counts, word);
            if (cnt >= PAIR_LEN && !cbm_ht_get(candidates, word)) {
                cbm_ht_set(candidates, word, intptr_to_ptr(SKIP_ONE));
                candidate_count++;
            }
        }
    }

    int tagged = 0;
    if (candidate_count > 0) {
        tagged = apply_decorator_tags(gbuf, nodes, node_count, candidates);
    }

    free_tagged_nodes(nodes, node_count);
    cbm_ht_free(word_counts);
    cbm_ht_free(candidates);

    cbm_log_info("pass.decorator_tags", "candidates", candidate_count > 0 ? "yes" : "0", "tagged",
                 tagged > 0 ? "yes" : "0");
    return tagged;
}
