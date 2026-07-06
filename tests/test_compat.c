/*
 * test_compat.c - Foundation compat_* contract tests.
 */
#include "test_framework.h"
#include "../src/foundation/compat_regex.h"
#include "../src/foundation/compat_thread.h"

#include <stdint.h>
#include <string.h>

typedef struct {
    cbm_mutex_t mutex;
    int value;
} CompatThreadCtx;

static void *compat_thread_increment(void *arg) {
    CompatThreadCtx *ctx = (CompatThreadCtx *)arg;
    cbm_mutex_lock(&ctx->mutex);
    ctx->value += 7;
    cbm_mutex_unlock(&ctx->mutex);
    return NULL;
}

TEST(compat_regex_basic_match_and_capture) {
    cbm_regex_t re;
    ASSERT_EQ(cbm_regcomp(&re, "([A-Za-z]+)-([0-9]+)", CBM_REG_EXTENDED), CBM_REG_OK);

    cbm_regmatch_t matches[3] = {{0}};
    ASSERT_EQ(cbm_regexec(&re, "pkg-123", 3, matches, 0), CBM_REG_OK);
    ASSERT_EQ(matches[0].rm_so, 0);
    ASSERT_EQ(matches[0].rm_eo, 7);
    ASSERT_EQ(matches[1].rm_so, 0);
    ASSERT_EQ(matches[1].rm_eo, 3);
    ASSERT_EQ(matches[2].rm_so, 4);
    ASSERT_EQ(matches[2].rm_eo, 7);

    ASSERT_EQ(cbm_regexec(&re, "pkg-nope", 3, matches, 0), CBM_REG_NOMATCH);
    cbm_regfree(&re);
    PASS();
}

TEST(compat_regex_icase_and_nosub) {
    cbm_regex_t re;
    ASSERT_EQ(cbm_regcomp(&re, "^hello$", CBM_REG_EXTENDED | CBM_REG_ICASE | CBM_REG_NOSUB),
              CBM_REG_OK);
    ASSERT_EQ(cbm_regexec(&re, "HeLLo", 0, NULL, 0), CBM_REG_OK);
    ASSERT_EQ(cbm_regexec(&re, "hello!", 0, NULL, 0), CBM_REG_NOMATCH);
    cbm_regfree(&re);
    PASS();
}

TEST(compat_regex_match_cap_32) {
    cbm_regex_t re;
    ASSERT_EQ(cbm_regcomp(&re, "(a)(b)(c)", CBM_REG_EXTENDED), CBM_REG_OK);
    cbm_regmatch_t matches[40];
    memset(matches, 0x7f, sizeof(matches));
    ASSERT_EQ(cbm_regexec(&re, "abc", 40, matches, 0), CBM_REG_OK);
    ASSERT_EQ(matches[0].rm_so, 0);
    ASSERT_EQ(matches[0].rm_eo, 3);
    ASSERT_EQ(matches[3].rm_so, 2);
    ASSERT_EQ(matches[3].rm_eo, 3);
    ASSERT_EQ(matches[32].rm_so, 0x7f7f7f7f);
    cbm_regfree(&re);
    PASS();
}

TEST(compat_regex_compile_error_nonzero) {
    cbm_regex_t re;
    ASSERT(cbm_regcomp(&re, "([", CBM_REG_EXTENDED) != CBM_REG_OK);
    PASS();
}

TEST(compat_thread_create_join_mutex) {
    CompatThreadCtx ctx;
    ctx.value = 0;
    cbm_mutex_init(&ctx.mutex);

    cbm_thread_t thread;
    ASSERT_EQ(cbm_thread_create(&thread, 0, compat_thread_increment, &ctx), 0);
    ASSERT_EQ(cbm_thread_join(&thread), 0);

    cbm_mutex_lock(&ctx.mutex);
    int value = ctx.value;
    cbm_mutex_unlock(&ctx.mutex);
    cbm_mutex_destroy(&ctx.mutex);

    ASSERT_EQ(value, 7);
    PASS();
}

TEST(compat_aligned_alloc_contract) {
    void *ptr = NULL;
    ASSERT_EQ(cbm_aligned_alloc(&ptr, 64, 256), 0);
    ASSERT_NOT_NULL(ptr);
    ASSERT_EQ(((uintptr_t)ptr) % 64, 0);
    cbm_aligned_free(ptr);
    cbm_aligned_free(NULL);
    PASS();
}

SUITE(compat) {
    RUN_TEST(compat_regex_basic_match_and_capture);
    RUN_TEST(compat_regex_icase_and_nosub);
    RUN_TEST(compat_regex_match_cap_32);
    RUN_TEST(compat_regex_compile_error_nonzero);
    RUN_TEST(compat_thread_create_join_mutex);
    RUN_TEST(compat_aligned_alloc_contract);
}
