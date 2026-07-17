/*
 * mem_rust_bridge.c — Rust mem direct slice 的 mimalloc/OS/log bridge。
 *
 * Budget state 與 public ABI 位於 Rust；本檔只保留 vendor allocator、平台 RSS
 * 與既有 log API 的 C 邊界。
 */
#include "mem.h"
#include "platform.h"
#include "log.h"
#include "foundation/constants.h"

#include <mimalloc.h>
#include <stdio.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <psapi.h>
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#else
#include <unistd.h>
#endif

void cbm_rs_mem_mimalloc_init(void) {
    (void)mi_option_set(mi_option_arena_eager_commit, 0);
    (void)mi_option_set(mi_option_purge_decommits, SKIP_ONE);
    (void)mi_option_set(mi_option_purge_delay, 0);
}

void cbm_rs_mem_process_info(size_t *current, size_t *peak) {
    size_t current_value = 0;
    size_t peak_value = 0;
    mi_process_info(NULL, NULL, NULL, &current_value, &peak_value, NULL, NULL, NULL);
    if (current) {
        *current = current_value;
    }
    if (peak) {
        *peak = peak_value;
    }
}

size_t cbm_rs_mem_os_rss(void) {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return (size_t)pmc.WorkingSetSize;
    }
    return 0;
#elif defined(__APPLE__)
    struct mach_task_basic_info info = {0};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) ==
        KERN_SUCCESS) {
        return (size_t)info.resident_size;
    }
    return 0;
#else
    FILE *file = fopen("/proc/self/statm", "r");
    if (!file) {
        return 0;
    }
    unsigned long pages = 0;
    unsigned long rss_pages = 0;
    if (fscanf(file, "%lu %lu", &pages, &rss_pages) != 2) {
        rss_pages = 0;
    }
    (void)fclose(file);
    long page = sysconf(_SC_PAGESIZE);
    return rss_pages * (page > 0 ? (size_t)page : CBM_SZ_4K);
#endif
}

void cbm_rs_mem_bridge_collect(void) {
    mi_collect(true);
}

void cbm_rs_mem_log_init(size_t budget, size_t total_ram, const char *source) {
    char budget_mb[CBM_SZ_32];
    char ram_mb[CBM_SZ_32];
    snprintf(budget_mb, sizeof(budget_mb), "%zu", budget / ((size_t)CBM_SZ_1K * CBM_SZ_1K));
    snprintf(ram_mb, sizeof(ram_mb), "%zu", total_ram / ((size_t)CBM_SZ_1K * CBM_SZ_1K));
    if (source) {
        cbm_log_info("mem.init", "budget_mb", budget_mb, "source", source);
    } else {
        cbm_log_info("mem.init", "budget_mb", budget_mb, "total_ram_mb", ram_mb);
    }
}

void cbm_rs_mem_log_invalid_env(const char *value) {
    cbm_log_warn("mem.budget.env.invalid", "value", value, "fallback", "ram_fraction");
}

void cbm_rs_mem_log_pressure(int over, size_t rss, size_t budget) {
    char rss_mb[CBM_SZ_32];
    char budget_mb[CBM_SZ_32];
    char pct_str[CBM_SZ_16];
    snprintf(rss_mb, sizeof(rss_mb), "%zu", rss / ((size_t)CBM_SZ_1K * CBM_SZ_1K));
    snprintf(budget_mb, sizeof(budget_mb), "%zu", budget / ((size_t)CBM_SZ_1K * CBM_SZ_1K));
    snprintf(pct_str, sizeof(pct_str), "%zu", budget > 0 ? (rss * CBM_PERCENT) / budget : 0);
    if (over) {
        cbm_log_warn("mem.pressure.warn", "rss_mb", rss_mb, "budget_mb", budget_mb, "pct",
                     pct_str);
    } else {
        cbm_log_info("mem.pressure.ok", "rss_mb", rss_mb, "budget_mb", budget_mb, "pct",
                     pct_str);
    }
}
