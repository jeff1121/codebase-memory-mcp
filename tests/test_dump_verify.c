/*
 * test_dump_verify.c — Post-dump plausibility gate (#334).
 *
 * Pure-function matrix mirrors sast-ai-app checkSilentDegradation cases.
 * I/O-level coverage that drives the gate against a real on-disk SQLite store
 * lives in test_dump_verify_io.c (store-linked, excluded from test-foundation).
 */
#include "../src/foundation/compat.h"
#include "../src/foundation/dump_verify.h"
#include "test_framework.h"

#include <math.h>
#include <stdlib.h>

static char *dump_verify_ratio_snapshot(void) {
    const char *value = getenv("CBM_DUMP_VERIFY_MIN_RATIO");
    return value ? cbm_strdup(value) : NULL;
}

static int dump_verify_ratio_apply(const char *value, double *out_ratio) {
    char *saved = dump_verify_ratio_snapshot();
    int rc = 0;

    if (value) {
        rc = cbm_setenv("CBM_DUMP_VERIFY_MIN_RATIO", value, 1);
    } else {
        rc = cbm_unsetenv("CBM_DUMP_VERIFY_MIN_RATIO");
    }
    if (rc == 0 && out_ratio) {
        *out_ratio = cbm_dump_verify_min_ratio();
    }

    if (saved) {
        if (cbm_setenv("CBM_DUMP_VERIFY_MIN_RATIO", saved, 1) != 0) {
            rc = -1;
        }
    } else if (cbm_unsetenv("CBM_DUMP_VERIFY_MIN_RATIO") != 0) {
        rc = -1;
    }

    free(saved);
    return rc;
}

TEST(dump_verify_no_baseline) {
    ASSERT_FALSE(cbm_dump_verify_is_degraded(-1, 500, 0.5, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_sparse_at_floor) {
    ASSERT_FALSE(cbm_dump_verify_is_degraded(50, 10, 0.5, CBM_DUMP_VERIFY_MIN_FLOOR));
    ASSERT_FALSE(cbm_dump_verify_is_degraded(12, 5, 0.5, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_floor_plus_one_boundary) {
    ASSERT_FALSE(cbm_dump_verify_is_degraded(51, 26, 0.5, CBM_DUMP_VERIFY_MIN_FLOOR));
    ASSERT_TRUE(cbm_dump_verify_is_degraded(51, 25, 0.5, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_custom_floor) {
    ASSERT_FALSE(cbm_dump_verify_is_degraded(25, 12, 0.5, 25));
    ASSERT_TRUE(cbm_dump_verify_is_degraded(26, 12, 0.5, 25));
    PASS();
}

TEST(dump_verify_shortfall_below_ratio) {
    ASSERT_TRUE(cbm_dump_verify_is_degraded(1000, 400, 0.5, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_just_above_ratio) {
    ASSERT_FALSE(cbm_dump_verify_is_degraded(1000, 500, 0.5, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_just_below_ratio) {
    ASSERT_TRUE(cbm_dump_verify_is_degraded(1000, 499, 0.5, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_zero_persisted) {
    ASSERT_TRUE(cbm_dump_verify_is_degraded(1000, 0, 0.5, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_growth) {
    ASSERT_FALSE(cbm_dump_verify_is_degraded(500, 750, 0.5, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_count_error) {
    ASSERT_TRUE(cbm_dump_verify_is_degraded(1000, -1, 0.5, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_ratio_zero_disables) {
    ASSERT_FALSE(cbm_dump_verify_is_degraded(1000, 10, 0.0, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_invalid_ratio_disables) {
    ASSERT_FALSE(cbm_dump_verify_is_degraded(1000, 10, -0.1, CBM_DUMP_VERIFY_MIN_FLOOR));
    ASSERT_FALSE(cbm_dump_verify_is_degraded(1000, 10, -INFINITY, CBM_DUMP_VERIFY_MIN_FLOOR));
    ASSERT_FALSE(cbm_dump_verify_is_degraded(1000, 10, NAN, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_loosened_ratio) {
    ASSERT_FALSE(cbm_dump_verify_is_degraded(1000, 600, 0.5, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_tightened_ratio) {
    ASSERT_TRUE(cbm_dump_verify_is_degraded(1000, 900, 0.95, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_edges_shrank_nodes_ok) {
    /* Edges are not gated; this documents nodes-only semantics for integrators. */
    ASSERT_FALSE(cbm_dump_verify_is_degraded(200, 200, 0.5, CBM_DUMP_VERIFY_MIN_FLOOR));
    PASS();
}

TEST(dump_verify_min_ratio_default_when_unset) {
    double ratio = -1.0;
    ASSERT_EQ(dump_verify_ratio_apply(NULL, &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, CBM_DUMP_VERIFY_DEFAULT_RATIO, 0.0);
    PASS();
}

TEST(dump_verify_min_ratio_valid_values) {
    double ratio = -1.0;
    ASSERT_EQ(dump_verify_ratio_apply("0", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, 0.0, 0.0);
    ASSERT_EQ(dump_verify_ratio_apply("0.25", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, 0.25, 0.0);
    ASSERT_EQ(dump_verify_ratio_apply(".25", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, 0.25, 0.0);
    ASSERT_EQ(dump_verify_ratio_apply(" +0.25", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, 0.25, 0.0);
    ASSERT_EQ(dump_verify_ratio_apply("0.5x", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, 0.5, 0.0);
    ASSERT_EQ(dump_verify_ratio_apply("0.5e", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, 0.5, 0.0);
    ASSERT_EQ(dump_verify_ratio_apply("5e-1", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, 0.5, 0.0);
    ASSERT_EQ(dump_verify_ratio_apply("0x1p-1", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, 0.5, 0.0);
    ASSERT_EQ(dump_verify_ratio_apply("1", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, 1.0, 0.0);
    PASS();
}

TEST(dump_verify_min_ratio_invalid_values) {
    double ratio = -1.0;
    ASSERT_EQ(dump_verify_ratio_apply("", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, CBM_DUMP_VERIFY_DEFAULT_RATIO, 0.0);
    ASSERT_EQ(dump_verify_ratio_apply("abc", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, CBM_DUMP_VERIFY_DEFAULT_RATIO, 0.0);
    ASSERT_EQ(dump_verify_ratio_apply("-0.1", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, CBM_DUMP_VERIFY_DEFAULT_RATIO, 0.0);
    ASSERT_EQ(dump_verify_ratio_apply("1.1", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, CBM_DUMP_VERIFY_DEFAULT_RATIO, 0.0);
    ASSERT_EQ(dump_verify_ratio_apply("inf", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, CBM_DUMP_VERIFY_DEFAULT_RATIO, 0.0);
    ASSERT_EQ(dump_verify_ratio_apply("nan", &ratio), 0);
    ASSERT_FLOAT_EQ(ratio, CBM_DUMP_VERIFY_DEFAULT_RATIO, 0.0);
    PASS();
}

SUITE(dump_verify) {
    RUN_TEST(dump_verify_no_baseline);
    RUN_TEST(dump_verify_sparse_at_floor);
    RUN_TEST(dump_verify_floor_plus_one_boundary);
    RUN_TEST(dump_verify_custom_floor);
    RUN_TEST(dump_verify_shortfall_below_ratio);
    RUN_TEST(dump_verify_just_above_ratio);
    RUN_TEST(dump_verify_just_below_ratio);
    RUN_TEST(dump_verify_zero_persisted);
    RUN_TEST(dump_verify_growth);
    RUN_TEST(dump_verify_count_error);
    RUN_TEST(dump_verify_ratio_zero_disables);
    RUN_TEST(dump_verify_invalid_ratio_disables);
    RUN_TEST(dump_verify_loosened_ratio);
    RUN_TEST(dump_verify_tightened_ratio);
    RUN_TEST(dump_verify_edges_shrank_nodes_ok);
    RUN_TEST(dump_verify_min_ratio_default_when_unset);
    RUN_TEST(dump_verify_min_ratio_valid_values);
    RUN_TEST(dump_verify_min_ratio_invalid_values);
}
