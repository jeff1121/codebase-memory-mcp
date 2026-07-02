#ifndef CBM_PIPELINE_RUST_PLAN_H
#define CBM_PIPELINE_RUST_PLAN_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifdef CBM_USE_RUST_PIPELINE_PLAN

enum {
    CBM_RS_PLAN_PREDUMP = 1,
    CBM_RS_PLAN_EXTRACTION_CHOICE = 2,
    CBM_RS_PLAN_CHOICE_BUF = 16,
    CBM_RS_PLAN_PREDUMP_BUF = 256
};

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

#endif

#endif
