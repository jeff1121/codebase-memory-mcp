/*
 * test_progress_sink.c — progress sink 輸出與狀態回歸測試。
 */
#include "test_framework.h"
#include <cli/progress_sink.h>
#include <foundation/log.h>
#include <stdio.h>
#include <string.h>

static const char *read_tmpfile(FILE *f) {
    static char buf[2048];
    fflush(f);
    rewind(f);
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    return buf;
}

TEST(progress_sink_formats_pipeline_events) {
    FILE *out = tmpfile();
    ASSERT_NOT_NULL(out);

    cbm_log_set_level(CBM_LOG_INFO);
    cbm_log_set_format(CBM_LOG_FORMAT_TEXT);
    cbm_progress_sink_init(out);

    cbm_log_info("pipeline.discover", "files", "12");
    cbm_log_info("pipeline.route", "path", "incremental");
    cbm_log_info("pass.start", "pass", "structure");
    cbm_log_info("parallel.extract.progress", "done", "2", "total", "5");
    cbm_log_info("pass.timing", "pass", "parallel_extract");
    cbm_log_info("gbuf.dump", "nodes", "10", "edges", "20");
    cbm_log_info("pipeline.done", "elapsed_ms", "42");
    cbm_progress_sink_fini();

    const char *got = read_tmpfile(out);
    const char *expected =
        "  Discovering files (12 found)\n"
        "  Starting incremental index\n"
        "[1/9] Building file structure\n"
        "\r  Extracting: 2/5 files (40%)\n"
        "[2/9] Extracting definitions\n"
        "Done: 10 nodes, 20 edges (42 ms)\n";

    int ok = strcmp(got, expected) == 0;
    fclose(out);
    cbm_log_set_level(CBM_LOG_INFO);
    cbm_log_set_format(CBM_LOG_FORMAT_TEXT);

    if (!ok) {
        FAIL("progress sink output mismatch");
    }
    PASS();
}

TEST(progress_sink_flushes_pending_newline_on_fini) {
    FILE *out = tmpfile();
    ASSERT_NOT_NULL(out);

    cbm_log_set_level(CBM_LOG_INFO);
    cbm_log_set_format(CBM_LOG_FORMAT_TEXT);
    cbm_progress_sink_init(out);

    cbm_log_info("parallel.extract.progress", "done", "2", "total", "5");
    cbm_progress_sink_fini();

    const char *got = read_tmpfile(out);
    const char *expected = "\r  Extracting: 2/5 files (40%)\n";

    int ok = strcmp(got, expected) == 0;
    fclose(out);
    cbm_log_set_level(CBM_LOG_INFO);
    cbm_log_set_format(CBM_LOG_FORMAT_TEXT);

    if (!ok) {
        FAIL("pending newline was not flushed on fini");
    }
    PASS();
}

void suite_progress_sink(void) {
    RUN_TEST(progress_sink_formats_pipeline_events);
    RUN_TEST(progress_sink_flushes_pending_newline_on_fini);
}
