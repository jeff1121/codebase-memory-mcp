/*
 * test_diagnostics.c — diagnostics 純邏輯的 deterministic 測試。
 */
#include "../src/foundation/diagnostics.h"
#include "test_framework.h"

#include <string.h>

static void reset_global_query_stats(void) {
    atomic_store(&g_query_stats.count, 0);
    atomic_store(&g_query_stats.errors, 0);
    atomic_store(&g_query_stats.time_us, 0);
    atomic_store(&g_query_stats.max_us, 0);
}

static cbm_diag_snapshot_t fixture_snapshot(void) {
    cbm_diag_snapshot_t snapshot = {
        .uptime_s = 17,
        .rss_bytes = 1024,
        .peak_rss_bytes = 2048,
        .heap_committed_bytes = 4096,
        .peak_committed_bytes = 8192,
        .page_faults = 23,
        .fd_count = 9,
        .queries =
            {
                .count = 3,
                .errors = 1,
                .total_us = 425,
                .avg_us = 141,
                .max_us = 250,
            },
        .pid = 4242,
    };
    return snapshot;
}

TEST(diagnostics_env_enabled_values) {
    ASSERT_TRUE(cbm_diag_env_enabled_value("1"));
    ASSERT_TRUE(cbm_diag_env_enabled_value("true"));

    ASSERT_FALSE(cbm_diag_env_enabled_value(NULL));
    ASSERT_FALSE(cbm_diag_env_enabled_value(""));
    ASSERT_FALSE(cbm_diag_env_enabled_value("0"));
    ASSERT_FALSE(cbm_diag_env_enabled_value("false"));
    ASSERT_FALSE(cbm_diag_env_enabled_value("TRUE"));
    ASSERT_FALSE(cbm_diag_env_enabled_value(" true"));
    PASS();
}

TEST(diagnostics_query_stats_snapshot_zero) {
    cbm_query_stats_t stats = {0};
    cbm_diag_query_snapshot_t snapshot = {0};

    cbm_diag_query_stats_snapshot(&stats, &snapshot);

    ASSERT_EQ(snapshot.count, 0);
    ASSERT_EQ(snapshot.errors, 0);
    ASSERT_EQ(snapshot.total_us, 0);
    ASSERT_EQ(snapshot.avg_us, 0);
    ASSERT_EQ(snapshot.max_us, 0);
    PASS();
}

TEST(diagnostics_query_stats_snapshot_reduces_average) {
    cbm_query_stats_t stats = {0};
    cbm_diag_query_snapshot_t snapshot = {0};

    atomic_store(&stats.count, 3);
    atomic_store(&stats.errors, 1);
    atomic_store(&stats.time_us, 425);
    atomic_store(&stats.max_us, 250);

    cbm_diag_query_stats_snapshot(&stats, &snapshot);

    ASSERT_EQ(snapshot.count, 3);
    ASSERT_EQ(snapshot.errors, 1);
    ASSERT_EQ(snapshot.total_us, 425);
    ASSERT_EQ(snapshot.avg_us, 141);
    ASSERT_EQ(snapshot.max_us, 250);
    PASS();
}

TEST(diagnostics_record_query_updates_global_stats) {
    cbm_diag_query_snapshot_t snapshot = {0};
    reset_global_query_stats();

    cbm_diag_record_query(100, false);
    cbm_diag_record_query(250, true);
    cbm_diag_record_query(75, false);
    cbm_diag_query_stats_snapshot(&g_query_stats, &snapshot);

    ASSERT_EQ(snapshot.count, 3);
    ASSERT_EQ(snapshot.errors, 1);
    ASSERT_EQ(snapshot.total_us, 425);
    ASSERT_EQ(snapshot.avg_us, 141);
    ASSERT_EQ(snapshot.max_us, 250);

    reset_global_query_stats();
    PASS();
}

TEST(diagnostics_format_path) {
    char path[128];
    char too_small[8];

    int n = cbm_diag_format_path(path, sizeof(path), "/tmp", 1234, "json");

    ASSERT_EQ(n, (int)strlen("/tmp/cbm-diagnostics-1234.json"));
    ASSERT_STR_EQ(path, "/tmp/cbm-diagnostics-1234.json");
    ASSERT_EQ(cbm_diag_format_path(too_small, sizeof(too_small), "/tmp", 1234, "ndjson"), -1);
    ASSERT_EQ(cbm_diag_format_path(NULL, sizeof(path), "/tmp", 1234, "json"), -1);
    PASS();
}

TEST(diagnostics_format_json_snapshot) {
    cbm_diag_snapshot_t snapshot = fixture_snapshot();
    char json[1024];
    const char *expected = "{\n"
                           "  \"uptime_s\": 17,\n"
                           "  \"rss_bytes\": 1024,\n"
                           "  \"peak_rss_bytes\": 2048,\n"
                           "  \"heap_committed_bytes\": 4096,\n"
                           "  \"peak_committed_bytes\": 8192,\n"
                           "  \"page_faults\": 23,\n"
                           "  \"fd_count\": 9,\n"
                           "  \"query_count\": 3,\n"
                           "  \"query_errors\": 1,\n"
                           "  \"query_total_us\": 425,\n"
                           "  \"query_avg_us\": 141,\n"
                           "  \"query_max_us\": 250,\n"
                           "  \"pid\": 4242\n"
                           "}\n";

    int n = cbm_diag_format_json(json, sizeof(json), &snapshot);

    ASSERT_EQ(n, (int)strlen(expected));
    ASSERT_STR_EQ(json, expected);
    PASS();
}

TEST(diagnostics_format_ndjson_snapshot) {
    cbm_diag_snapshot_t snapshot = fixture_snapshot();
    char line[256];
    const char *expected = "{\"uptime_s\":17,\"rss\":1024,\"peak_rss\":2048,\"committed\":4096,"
                           "\"peak_committed\":8192,\"page_faults\":23,\"fd\":9,\"queries\":3}\n";

    int n = cbm_diag_format_ndjson(line, sizeof(line), &snapshot);

    ASSERT_EQ(n, (int)strlen(expected));
    ASSERT_STR_EQ(line, expected);
    PASS();
}

SUITE(diagnostics) {
    RUN_TEST(diagnostics_env_enabled_values);
    RUN_TEST(diagnostics_query_stats_snapshot_zero);
    RUN_TEST(diagnostics_query_stats_snapshot_reduces_average);
    RUN_TEST(diagnostics_record_query_updates_global_stats);
    RUN_TEST(diagnostics_format_path);
    RUN_TEST(diagnostics_format_json_snapshot);
    RUN_TEST(diagnostics_format_ndjson_snapshot);
}
