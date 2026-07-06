/*
 * test_profile.c - Foundation/profile contract tests.
 */
#include "test_framework.h"
#include "../src/foundation/profile.h"
#include "../src/foundation/log.h"
#include "../src/foundation/compat.h"

#include <stdbool.h>
#include <string.h>
#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#else
#include <fcntl.h>
#include <io.h>
#endif

static char profile_log_buf[4096];
static int profile_saved_stderr;
static int profile_pipe_fds[2];

static bool contains_raw(const char *s, const char *sub) {
    return strstr(s, sub) != NULL;
}

static void profile_capture_start(void) {
    fflush(stderr);
    profile_saved_stderr = dup(STDERR_FILENO);
    cbm_pipe(profile_pipe_fds);
#ifndef _WIN32
    fcntl(profile_pipe_fds[0], F_SETFL, O_NONBLOCK);
#endif
    dup2(profile_pipe_fds[1], STDERR_FILENO);
    close(profile_pipe_fds[1]);
}

static const char *profile_capture_end(void) {
    fflush(stderr);
    dup2(profile_saved_stderr, STDERR_FILENO);
    close(profile_saved_stderr);
    ssize_t n = read(profile_pipe_fds[0], profile_log_buf, sizeof(profile_log_buf) - 1);
    close(profile_pipe_fds[0]);
    if (n < 0) {
        n = 0;
    }
    profile_log_buf[n] = '\0';
    return profile_log_buf;
}

TEST(profile_env_gate_contract) {
    cbm_profile_active = false;
    cbm_unsetenv("CBM_PROFILE");
    cbm_profile_init();
    ASSERT_FALSE(cbm_profile_active);

    cbm_setenv("CBM_PROFILE", "", 1);
    cbm_profile_active = false;
    cbm_profile_init();
    ASSERT_FALSE(cbm_profile_active);

    cbm_setenv("CBM_PROFILE", "0", 1);
    cbm_profile_active = false;
    cbm_profile_init();
    ASSERT_FALSE(cbm_profile_active);

    cbm_setenv("CBM_PROFILE", "00", 1);
    cbm_profile_active = false;
    cbm_profile_init();
    ASSERT_FALSE(cbm_profile_active);

    cbm_setenv("CBM_PROFILE", "1", 1);
    cbm_profile_active = false;
    cbm_profile_init();
    ASSERT_TRUE(cbm_profile_active);

    cbm_setenv("CBM_PROFILE", "false", 1);
    cbm_profile_active = false;
    cbm_profile_init();
    ASSERT_TRUE(cbm_profile_active);

    cbm_profile_active = false;
    cbm_profile_enable();
    ASSERT_TRUE(cbm_profile_active);

    cbm_profile_active = false;
    cbm_unsetenv("CBM_PROFILE");
    PASS();
}

TEST(profile_log_elapsed_shape) {
    cbm_log_set_level(CBM_LOG_INFO);
    cbm_log_set_format(CBM_LOG_FORMAT_TEXT);

    struct timespec start;
    cbm_profile_now(&start);
    if (start.tv_nsec >= 2000000L) {
        start.tv_nsec -= 2000000L;
    } else {
        start.tv_sec -= 1;
        start.tv_nsec += 998000000L;
    }

    profile_capture_start();
    cbm_profile_log_elapsed("extract", "defs", &start, 12);
    const char *output = profile_capture_end();

    ASSERT(contains_raw(output, "level=info"));
    ASSERT(contains_raw(output, "msg=prof"));
    ASSERT(contains_raw(output, "phase=extract"));
    ASSERT(contains_raw(output, "sub=defs"));
    ASSERT(contains_raw(output, "ms="));
    ASSERT(contains_raw(output, "us="));
    ASSERT(contains_raw(output, "items=12"));
    ASSERT(contains_raw(output, "rate_per_s="));
    PASS();
}

SUITE(profile) {
    RUN_TEST(profile_env_gate_contract);
    RUN_TEST(profile_log_elapsed_shape);
}
