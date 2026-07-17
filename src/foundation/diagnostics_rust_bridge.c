/* Rust diagnostics 的 mimalloc process-info bridge。 */

#include <stddef.h>

#include <mimalloc.h>

void cbm_rs_diag_process_info(size_t *elapsed_ms, size_t *user_ms, size_t *sys_ms,
                              size_t *current_rss, size_t *peak_rss, size_t *current_commit,
                              size_t *peak_commit, size_t *page_faults) {
    mi_process_info(elapsed_ms, user_ms, sys_ms, current_rss, peak_rss, current_commit,
                    peak_commit, page_faults);
}
