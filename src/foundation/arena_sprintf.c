/*
 * arena_sprintf.c — Rust arena 的 C variadic compatibility shim。
 *
 * arena 的配置、回收與字串複製由 Rust FFI 實作；C 只保留公開 C ABI
 * 的 forwarding layer，以及 stable Rust 無法直接實作的 variadic formatting。
 */
#include "arena.h"

#include <stdarg.h>
#include <stdio.h>

extern void* cbm_rs_arena_alloc(CBMArena *arena, size_t n);
extern void cbm_rs_arena_init(CBMArena *arena);
extern void cbm_rs_arena_init_sized(CBMArena *arena, size_t block_size);
extern void* cbm_rs_arena_calloc(CBMArena *arena, size_t n);
extern char* cbm_rs_arena_strdup(CBMArena *arena, const char *value);
extern char* cbm_rs_arena_strndup(CBMArena *arena, const unsigned char *value, size_t len);
extern void cbm_rs_arena_reset(CBMArena *arena);
extern void cbm_rs_arena_destroy(CBMArena *arena);
extern size_t cbm_rs_arena_total(const CBMArena *arena);

void cbm_arena_init(CBMArena *a) {
    cbm_rs_arena_init(a);
}

void cbm_arena_init_sized(CBMArena *a, size_t block_size) {
    cbm_rs_arena_init_sized(a, block_size);
}

void *cbm_arena_alloc(CBMArena *a, size_t n) {
    return cbm_rs_arena_alloc(a, n);
}

void *cbm_arena_calloc(CBMArena *a, size_t n) {
    return cbm_rs_arena_calloc(a, n);
}

char *cbm_arena_strdup(CBMArena *a, const char *s) {
    return cbm_rs_arena_strdup(a, s);
}

char *cbm_arena_strndup(CBMArena *a, const char *s, size_t len) {
    return cbm_rs_arena_strndup(a, (const unsigned char *)s, len);
}

char *cbm_arena_sprintf(CBMArena *a, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (needed < 0) {
        return NULL;
    }

    char *dst = (char *)cbm_rs_arena_alloc(a, (size_t)needed + 1);
    if (!dst) {
        return NULL;
    }

    va_start(args, fmt);
    vsnprintf(dst, (size_t)needed + 1, fmt, args);
    va_end(args);
    return dst;
}

void cbm_arena_reset(CBMArena *a) {
    cbm_rs_arena_reset(a);
}

void cbm_arena_destroy(CBMArena *a) {
    cbm_rs_arena_destroy(a);
}

size_t cbm_arena_total(const CBMArena *a) {
    return cbm_rs_arena_total(a);
}
