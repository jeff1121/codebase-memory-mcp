#include "foundation/constants.h"
#include "pipeline/pipeline.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

enum {
    PT_TEST_LEN = 4,
    PT_DESCRIBE_LEN = 9,
    PT_CONTEXT_LEN = 7,
};

#define SLEN(s) (sizeof(s) - 1)

#ifdef CBM_USE_RUST_PIPELINE_TEST_DETECT
extern int cbm_rs_pipeline_is_test_path_v1(const char *path);
extern int cbm_rs_pipeline_is_test_func_name_v1(const char *name);
#endif

static bool str_ends_with(const char *s, size_t slen, const char *suffix) {
    size_t sflen = strlen(suffix);
    return slen >= sflen && strcmp(s + slen - sflen, suffix) == 0;
}

bool cbm_is_test_path(const char *path) {
    if (!path) {
        return false;
    }
#ifdef CBM_USE_RUST_PIPELINE_TEST_DETECT
    return cbm_rs_pipeline_is_test_path_v1(path) != 0;
#endif
    const char *base = strrchr(path, '/');
    base = base ? base + SKIP_ONE : path;
    size_t len = strlen(path);

    if (strncmp(base, "test_", SLEN("test_")) == 0) {
        return true;
    }
    if (str_ends_with(path, len, "_test.go") || str_ends_with(path, len, "_test.py") ||
        str_ends_with(path, len, "_test.rs") || str_ends_with(path, len, "_test.cpp") ||
        str_ends_with(path, len, "_test.lua")) {
        return true;
    }
    if (strstr(path, ".test.ts") || strstr(path, ".spec.ts") || strstr(path, ".test.js") ||
        strstr(path, ".spec.js") || strstr(path, ".test.tsx") || strstr(path, ".spec.tsx")) {
        return true;
    }
    if (str_ends_with(path, len, "Test.java") || str_ends_with(path, len, "Test.kt") ||
        str_ends_with(path, len, "Test.cs") || str_ends_with(path, len, "Test.php") ||
        str_ends_with(path, len, "Spec.scala")) {
        return true;
    }
    if (strstr(path, "__tests__/") || strstr(path, "/tests/") || strstr(path, "/test/") ||
        strstr(path, "/spec/")) {
        return true;
    }
    if (strncmp(path, "tests/", SLEN("tests/")) == 0 ||
        strncmp(path, "test/", SLEN("test/")) == 0 || strncmp(path, "spec/", SLEN("spec/")) == 0 ||
        strncmp(path, "__tests__/", SLEN("__tests__/")) == 0) {
        return true;
    }
    return str_ends_with(path, len, "_spec.rb");
}

bool cbm_is_test_func_name(const char *name) {
    if (!name) {
        return false;
    }
#ifdef CBM_USE_RUST_PIPELINE_TEST_DETECT
    return cbm_rs_pipeline_is_test_func_name_v1(name) != 0;
#endif
    if (strncmp(name, "Test", SLEN("Test")) == 0 &&
        (name[PT_TEST_LEN] == '\0' || (name[PT_TEST_LEN] >= 'A' && name[PT_TEST_LEN] <= 'Z'))) {
        return true;
    }
    if (strncmp(name, "Benchmark", SLEN("Benchmark")) == 0 &&
        (name[PT_DESCRIBE_LEN] == '\0' ||
         (name[PT_DESCRIBE_LEN] >= 'A' && name[PT_DESCRIBE_LEN] <= 'Z'))) {
        return true;
    }
    if (strncmp(name, "Example", SLEN("Example")) == 0 &&
        (name[PT_CONTEXT_LEN] == '\0' ||
         (name[PT_CONTEXT_LEN] >= 'A' && name[PT_CONTEXT_LEN] <= 'Z'))) {
        return true;
    }
    if (strncmp(name, "test_", SLEN("test_")) == 0) {
        return true;
    }
    if (strncmp(name, "test", SLEN("test")) == 0 && name[PT_TEST_LEN] >= 'A' &&
        name[PT_TEST_LEN] <= 'Z') {
        return true;
    }
    if (strcmp(name, "test") == 0 || strcmp(name, "it") == 0 || strcmp(name, "describe") == 0 ||
        strcmp(name, "beforeAll") == 0 || strcmp(name, "afterAll") == 0 ||
        strcmp(name, "beforeEach") == 0 || strcmp(name, "afterEach") == 0) {
        return true;
    }
    return strcmp(name, "@testset") == 0 || strcmp(name, "@test") == 0;
}
