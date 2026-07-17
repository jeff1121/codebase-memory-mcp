/* Rust diagnostics FFI smoke 專用 bridge stub。 */

#include <stddef.h>

void cbm_rs_diag_process_info(size_t *elapsed_ms, size_t *user_ms, size_t *sys_ms,
                              size_t *current_rss, size_t *peak_rss, size_t *current_commit,
                              size_t *peak_commit, size_t *page_faults) {
    if (elapsed_ms) {
        *elapsed_ms = 0;
    }
    if (user_ms) {
        *user_ms = 0;
    }
    if (sys_ms) {
        *sys_ms = 0;
    }
    if (current_rss) {
        *current_rss = (size_t)8 * 1024 * 1024;
    }
    if (peak_rss) {
        *peak_rss = (size_t)12 * 1024 * 1024;
    }
    if (current_commit) {
        *current_commit = 0;
    }
    if (peak_commit) {
        *peak_commit = 0;
    }
    if (page_faults) {
        *page_faults = 0;
    }
}

#if !defined(CBM_USE_RUST_MEM_ONLY)
size_t cbm_mem_rss(void) {
    return (size_t)8 * 1024 * 1024;
}

size_t cbm_mem_peak_rss(void) {
    return (size_t)12 * 1024 * 1024;
}
#endif
