#ifndef CBM_PIPELINE_RUST_PLAN_H
#define CBM_PIPELINE_RUST_PLAN_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

enum { CBM_RS_INCR_POST_MAX_STEPS = 10 };
enum { CBM_RS_PLAN_V2_MAX_STEPS = 16 };
enum { CBM_RS_PLAN_TOP_MAX_STEPS = 12 };
enum { CBM_RS_PLAN_STEP_MAX_KIND = 64 };

enum {
    CBM_RS_PLAN_SEQUENTIAL = 0,
    CBM_RS_PLAN_PREDUMP = 1,
    CBM_RS_PLAN_EXTRACTION_CHOICE = 2,
    CBM_RS_PLAN_INCREMENTAL_EXTRACT_RESOLVE = 3,
    CBM_RS_PLAN_INCREMENTAL_POST = 4,
    CBM_RS_PLAN_PARALLEL_EXTRACTION = 5,
    CBM_RS_PLAN_FULL_PIPELINE = 6
};

enum {
    CBM_RS_PLAN_STEP_PREDUMP_DECORATOR_TAGS = 1,
    CBM_RS_PLAN_STEP_PREDUMP_CONFIGLINK = 2,
    CBM_RS_PLAN_STEP_PREDUMP_ROUTE_MATCH = 3,
    CBM_RS_PLAN_STEP_PREDUMP_SIMILARITY = 4,
    CBM_RS_PLAN_STEP_PREDUMP_SEMANTIC_EDGES = 5,
    CBM_RS_PLAN_STEP_PREDUMP_COMPLEXITY = 6,
    CBM_RS_PLAN_STEP_SEQUENTIAL_DEFINITIONS = 16,
    CBM_RS_PLAN_STEP_SEQUENTIAL_K8S = 17,
    CBM_RS_PLAN_STEP_SEQUENTIAL_LSP_CROSS = 18,
    CBM_RS_PLAN_STEP_SEQUENTIAL_CALLS = 19,
    CBM_RS_PLAN_STEP_SEQUENTIAL_USAGES = 20,
    CBM_RS_PLAN_STEP_SEQUENTIAL_SEMANTIC = 21,
    CBM_RS_PLAN_STEP_PARALLEL_EXTRACT = 32,
    CBM_RS_PLAN_STEP_PARALLEL_REGISTRY_BUILD = 33,
    CBM_RS_PLAN_STEP_PARALLEL_LSP_CROSS_PREPARE = 34,
    CBM_RS_PLAN_STEP_PARALLEL_RESOLVE = 35,
    CBM_RS_PLAN_STEP_PARALLEL_INFRA_ROUTES = 36,
    CBM_RS_PLAN_STEP_PARALLEL_INFRA_BINDINGS = 37,
    CBM_RS_PLAN_STEP_PARALLEL_K8S = 38,
    CBM_RS_PLAN_STEP_INCREMENTAL_DEFINITIONS = 48,
    CBM_RS_PLAN_STEP_INCREMENTAL_CALLS = 49,
    CBM_RS_PLAN_STEP_INCREMENTAL_USAGES = 50,
    CBM_RS_PLAN_STEP_INCREMENTAL_SEMANTIC = 51,
    CBM_RS_PLAN_STEP_INCREMENTAL_PARALLEL_EXTRACT = 52,
    CBM_RS_PLAN_STEP_INCREMENTAL_REGISTRY = 53,
    CBM_RS_PLAN_STEP_INCREMENTAL_RESOLVE = 54
};

enum {
    CBM_RS_PLAN_PHASE_PREDUMP = 1,
    CBM_RS_PLAN_PHASE_SEQUENTIAL_EXTRACT = 2,
    CBM_RS_PLAN_PHASE_PARALLEL_EXTRACT = 3,
    CBM_RS_PLAN_PHASE_INCREMENTAL_EXTRACT_RESOLVE = 4
};

enum {
    CBM_RS_INCR_POST_K8S = 1,
    CBM_RS_INCR_POST_TESTS = 2,
    CBM_RS_INCR_POST_DECORATOR_TAGS = 3,
    CBM_RS_INCR_POST_CONFIGLINK = 4,
    CBM_RS_INCR_POST_SIMILARITY = 5,
    CBM_RS_INCR_POST_SEMANTIC_EDGES = 6,
    CBM_RS_INCR_POST_EDGE_RELINK = 7,
    CBM_RS_INCR_POST_INCREMENTAL_DUMP = 8,
    CBM_RS_INCR_POST_PERSIST_HASHES = 9,
    CBM_RS_INCR_POST_ARTIFACT_EXPORT = 10
};

enum {
    CBM_RS_PLAN_POLICY_REQUIRED = 0,
    CBM_RS_PLAN_POLICY_IGNORE_ERR = 1,
    CBM_RS_PLAN_POLICY_BEST_EFFORT = 2,
    CBM_RS_PLAN_POLICY_OPTIONAL_EXISTING_ARTIFACT = 3,
    CBM_RS_PLAN_POLICY_ENV_OPTIONAL = 4
};

enum {
    CBM_RS_PLAN_GATE_SKIP_FAST = 1u << 0,
    CBM_RS_PLAN_GATE_REQUIRES_RESULT_CACHE = 1u << 1,
    CBM_RS_PLAN_GATE_NO_CROSS_LSP_PREBUILD = 1u << 2
};
enum { CBM_RS_PLAN_EFFECT_MUTATES_GRAPH = 1u << 0 };

enum {
    CBM_RS_PLAN_TOP_MACRO_EXTRACTION = 1,
    CBM_RS_PLAN_TOP_USERCONFIG_LOAD = 2,
    CBM_RS_PLAN_TOP_DISCOVER = 3,
    CBM_RS_PLAN_TOP_TRY_INCREMENTAL_OR_DELETE_DB = 4,
    CBM_RS_PLAN_TOP_STRUCTURE = 5,
    CBM_RS_PLAN_TOP_EXTRACTION_DISPATCH = 6,
    CBM_RS_PLAN_TOP_TESTS = 7,
    CBM_RS_PLAN_TOP_GITHISTORY = 8,
    CBM_RS_PLAN_TOP_PREDUMP = 9,
    CBM_RS_PLAN_TOP_DUMP = 10,
    CBM_RS_PLAN_TOP_PERSIST_HASHES = 11,
    CBM_RS_PLAN_TOP_ARTIFACT_EXPORT = 12
};

enum { CBM_RS_PLAN_TOP_PHASE_FULL_PIPELINE = 5 };

enum {
    CBM_RS_PLAN_TOP_POLICY_REQUIRED = 0,
    CBM_RS_PLAN_TOP_POLICY_BEST_EFFORT = 2,
    CBM_RS_PLAN_TOP_POLICY_FULL_MODE_ONLY = 5,
    CBM_RS_PLAN_TOP_POLICY_FAIL_OPEN = 6,
    CBM_RS_PLAN_TOP_POLICY_EXISTING_DB_ONLY = 7,
    CBM_RS_PLAN_TOP_POLICY_OPTIONAL_PERSISTENCE = 8
};

enum { CBM_RS_PLAN_TOP_GATE_SKIP_FAST = 1u << 0, CBM_RS_PLAN_TOP_GATE_MAY_SHORT_CIRCUIT = 1u << 1 };

enum {
    CBM_RS_PLAN_TOP_EFFECT_MUTATES_GRAPH = 1u << 0,
    CBM_RS_PLAN_TOP_EFFECT_WRITES_STORE_OR_ARTIFACT = 1u << 1
};

enum { CBM_RS_PLAN_TOP_NO_NESTED_PLAN = -1 };

typedef struct {
    int kind;
    int policy;
} cbm_rs_pipeline_plan_step_t;

_Static_assert(sizeof(cbm_rs_pipeline_plan_step_t) == 8, "PlanStep ABI size drift");
_Static_assert(offsetof(cbm_rs_pipeline_plan_step_t, kind) == 0, "PlanStep.kind offset drift");
_Static_assert(offsetof(cbm_rs_pipeline_plan_step_t, policy) == 4, "PlanStep.policy offset drift");

typedef struct {
    int kind;
    int phase;
    int policy;
    uint32_t gate_flags;
    uint64_t requires_mask;
    uint32_t effect_flags;
} cbm_rs_pipeline_plan_step_v2_t;

_Static_assert(sizeof(cbm_rs_pipeline_plan_step_v2_t) == 32, "PlanStepV2 ABI size drift");
_Static_assert(offsetof(cbm_rs_pipeline_plan_step_v2_t, kind) == 0, "PlanStepV2.kind offset drift");
_Static_assert(offsetof(cbm_rs_pipeline_plan_step_v2_t, phase) == 4,
               "PlanStepV2.phase offset drift");
_Static_assert(offsetof(cbm_rs_pipeline_plan_step_v2_t, policy) == 8,
               "PlanStepV2.policy offset drift");
_Static_assert(offsetof(cbm_rs_pipeline_plan_step_v2_t, gate_flags) == 12,
               "PlanStepV2.gate_flags offset drift");
_Static_assert(offsetof(cbm_rs_pipeline_plan_step_v2_t, requires_mask) == 16,
               "PlanStepV2.requires_mask offset drift");
_Static_assert(offsetof(cbm_rs_pipeline_plan_step_v2_t, effect_flags) == 24,
               "PlanStepV2.effect_flags offset drift");

typedef struct {
    int kind;
    int phase;
    int policy;
    uint32_t gate_flags;
    uint64_t requires_mask;
    uint32_t effect_flags;
    int nested_plan_kind;
} cbm_rs_pipeline_top_step_v1_t;

_Static_assert(sizeof(cbm_rs_pipeline_top_step_v1_t) == 32, "PipelineTopStepV1 ABI size drift");
_Static_assert(offsetof(cbm_rs_pipeline_top_step_v1_t, kind) == 0,
               "PipelineTopStepV1.kind offset drift");
_Static_assert(offsetof(cbm_rs_pipeline_top_step_v1_t, phase) == 4,
               "PipelineTopStepV1.phase offset drift");
_Static_assert(offsetof(cbm_rs_pipeline_top_step_v1_t, policy) == 8,
               "PipelineTopStepV1.policy offset drift");
_Static_assert(offsetof(cbm_rs_pipeline_top_step_v1_t, gate_flags) == 12,
               "PipelineTopStepV1.gate_flags offset drift");
_Static_assert(offsetof(cbm_rs_pipeline_top_step_v1_t, requires_mask) == 16,
               "PipelineTopStepV1.requires_mask offset drift");
_Static_assert(offsetof(cbm_rs_pipeline_top_step_v1_t, effect_flags) == 24,
               "PipelineTopStepV1.effect_flags offset drift");
_Static_assert(offsetof(cbm_rs_pipeline_top_step_v1_t, nested_plan_kind) == 28,
               "PipelineTopStepV1.nested_plan_kind offset drift");

#ifdef CBM_USE_RUST_PIPELINE_PLAN

enum {
    CBM_RS_PLAN_CHOICE_BUF = 16,
    CBM_RS_PLAN_SEQUENTIAL_BUF = 256,
    CBM_RS_PLAN_PARALLEL_BUF = 256,
    CBM_RS_PLAN_PREDUMP_BUF = 256,
    CBM_RS_PLAN_INCREMENTAL_POST_BUF = 512
};

extern int cbm_rs_pipeline_incremental_post_step_count(int mode);
extern int cbm_rs_pipeline_incremental_post_steps(int mode, cbm_rs_pipeline_plan_step_t *out,
                                                  int cap);
extern int cbm_rs_pipeline_plan_step_count_v2(int kind, int mode, int worker_count, int file_count);
extern int cbm_rs_pipeline_plan_steps_v2(int kind, int mode, int worker_count, int file_count,
                                         cbm_rs_pipeline_plan_step_v2_t *out, int cap);

static inline bool cbm_rust_plan_incremental_post_steps(int mode, cbm_rs_pipeline_plan_step_t *out,
                                                        int cap, int *out_count) {
    if (!out || cap <= 0 || !out_count) {
        return false;
    }
    int count = cbm_rs_pipeline_incremental_post_step_count(mode);
    if (count < 0 || count > cap) {
        return false;
    }
    int rc = cbm_rs_pipeline_incremental_post_steps(mode, out, cap);
    if (rc != count) {
        return false;
    }
    *out_count = count;
    return true;
}

static inline bool cbm_rust_plan_steps_v2(int kind, int mode, int worker_count, int file_count,
                                          cbm_rs_pipeline_plan_step_v2_t *out, int cap,
                                          int *out_count) {
    if (!out || cap <= 0 || !out_count) {
        return false;
    }
    int count = cbm_rs_pipeline_plan_step_count_v2(kind, mode, worker_count, file_count);
    if (count < 0 || count > cap) {
        return false;
    }
    int rc = cbm_rs_pipeline_plan_steps_v2(kind, mode, worker_count, file_count, out, cap);
    if (rc != count) {
        return false;
    }
    *out_count = count;
    return true;
}

extern int cbm_rs_pipeline_plan_describe(int kind, int mode, int worker_count, int file_count,
                                         char *buf, size_t bufsize);

static inline bool cbm_rust_plan_extracts_parallel(int mode, int worker_count, int file_count,
                                                   bool *out) {
    if (!out) {
        return false;
    }
    char choice[CBM_RS_PLAN_CHOICE_BUF];
    int rc = cbm_rs_pipeline_plan_describe(CBM_RS_PLAN_EXTRACTION_CHOICE, mode, worker_count,
                                           file_count, choice, sizeof(choice));
    if (rc < 0) {
        return false;
    }
    if (strcmp(choice, "parallel") == 0) {
        *out = true;
        return true;
    }
    if (strcmp(choice, "sequential") == 0) {
        *out = false;
        return true;
    }
    return false;
}

static inline bool cbm_rust_plan_predump(int mode, char *buf, size_t bufsize) {
    if (!buf || bufsize == 0) {
        return false;
    }
    int rc = cbm_rs_pipeline_plan_describe(CBM_RS_PLAN_PREDUMP, mode, 0, 0, buf, bufsize);
    return rc >= 0 && (size_t)rc < bufsize;
}

static inline bool cbm_rust_plan_incremental_post(int mode, char *buf, size_t bufsize) {
    if (!buf || bufsize == 0) {
        return false;
    }
    int rc = cbm_rs_pipeline_plan_describe(CBM_RS_PLAN_INCREMENTAL_POST, mode, 0, 0, buf, bufsize);
    return rc >= 0 && (size_t)rc < bufsize;
}

#endif

#endif
