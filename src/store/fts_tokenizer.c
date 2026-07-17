#include "foundation/constants.h"

#include <sqlite3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    CAMEL_SPLIT_BUF = 2048,
    SQLITE_AUTO_LEN = -1,
    CAMEL_BUF_GUARD = 2,
};

#ifdef CBM_USE_RUST_STORE_FTS_TOKENIZER
extern size_t cbm_rs_store_camel_split_v1(char *buf, size_t bufsize, const char *input);
#endif

/* 將 SQLite 的暫存 destructor sentinel 隔離於單一模組。 */
static sqlite3_destructor_type cbm_sqlite_transient_destructor(void) {
    static const volatile intptr_t raw = -1;
    sqlite3_destructor_type dtor = NULL;
    memcpy(&dtor, (const void *)&raw, sizeof(dtor));
    return dtor;
}
#define CBM_SQLITE_TRANSIENT (cbm_sqlite_transient_destructor())

#ifndef CBM_USE_RUST_STORE_FTS_TOKENIZER
static bool camel_should_split(const char *input, int i) {
    if (i <= 0) {
        return false;
    }
    char curr = input[i];
    char prev = input[i - SKIP_ONE];
    char next = input[i + SKIP_ONE];
    if (curr >= 'A' && curr <= 'Z' && prev >= 'a' && prev <= 'z') {
        return true;
    }
    return curr >= 'A' && curr <= 'Z' && prev >= 'A' && prev <= 'Z' && next >= 'a' && next <= 'z';
}
#endif

void cbm_store_sqlite_camel_split(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    (void)argc;
    const char *input = (const char *)sqlite3_value_text(argv[0]);
#ifdef CBM_USE_RUST_STORE_FTS_TOKENIZER
    char rust_buf[CAMEL_SPLIT_BUF];
    size_t rust_len = cbm_rs_store_camel_split_v1(rust_buf, sizeof(rust_buf), input);
    if (rust_len < sizeof(rust_buf)) {
        sqlite3_result_text(ctx, rust_buf, (int)rust_len, CBM_SQLITE_TRANSIENT);
        return;
    }
    sqlite3_result_text(ctx, input ? input : "", SQLITE_AUTO_LEN, CBM_SQLITE_TRANSIENT);
    return;
#else
    if (!input || !input[0]) {
        sqlite3_result_text(ctx, input ? input : "", SQLITE_AUTO_LEN, CBM_SQLITE_TRANSIENT);
        return;
    }
    char buf[CAMEL_SPLIT_BUF];
    int len = snprintf(buf, sizeof(buf), "%s ", input);
    if (len < 0 || len >= (int)sizeof(buf)) {
        sqlite3_result_text(ctx, input, SQLITE_AUTO_LEN, CBM_SQLITE_TRANSIENT);
        return;
    }
    for (int i = 0; input[i] && len < (int)sizeof(buf) - CAMEL_BUF_GUARD; i++) {
        if (camel_should_split(input, i)) {
            buf[len++] = ' ';
        }
        buf[len++] = input[i];
    }
    buf[len] = '\0';
    sqlite3_result_text(ctx, buf, len, CBM_SQLITE_TRANSIENT);
#endif
}
