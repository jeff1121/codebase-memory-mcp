/* mem Rust FFI smoke 專用 mimalloc bridge stub。 */

#include <stddef.h>

void cbm_rs_mem_mimalloc_init(void) {}

void cbm_rs_mem_process_info(size_t *current, size_t *peak) {
    if (current) {
        *current = (size_t)8 * 1024 * 1024;
    }
    if (peak) {
        *peak = (size_t)12 * 1024 * 1024;
    }
}

size_t cbm_rs_mem_os_rss(void) {
    return (size_t)8 * 1024 * 1024;
}

void cbm_rs_mem_bridge_collect(void) {}

void cbm_rs_mem_log_init(size_t budget, size_t total_ram, const char *source) {
    (void)budget;
    (void)total_ram;
    (void)source;
}

void cbm_rs_mem_log_invalid_env(const char *value) {
    (void)value;
}

void cbm_rs_mem_log_pressure(int over, size_t rss, size_t budget) {
    (void)over;
    (void)rss;
    (void)budget;
}
