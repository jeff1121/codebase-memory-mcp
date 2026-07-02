/*
 * test_foundation_main.c — Foundation-only test runner.
 *
 * Kept separate from test_main.c so the fast foundation target does not link
 * every suite in the repository.
 */
int tf_pass_count = 0;
int tf_fail_count = 0;
int tf_skip_count = 0;

#include "test_framework.h"

extern void suite_arena(void);
extern void suite_hash_table(void);
extern void suite_dyn_array(void);
extern void suite_str_intern(void);
extern void suite_log(void);
extern void suite_diagnostics(void);
extern void suite_str_util(void);
extern void suite_platform(void);
extern void suite_dump_verify(void);

int main(void) {
    printf("\n  codebase-memory-mcp  foundation test suite\n");

    RUN_SUITE(arena);
    RUN_SUITE(hash_table);
    RUN_SUITE(dyn_array);
    RUN_SUITE(str_intern);
    RUN_SUITE(log);
    RUN_SUITE(diagnostics);
    RUN_SUITE(str_util);
    RUN_SUITE(platform);
    RUN_SUITE(dump_verify);

    TEST_SUMMARY();
}
