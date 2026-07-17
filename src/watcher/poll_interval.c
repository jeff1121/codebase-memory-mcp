#include "watcher/watcher.h"

#include "foundation/constants.h"

#define POLL_BASE_MS 5000
#define POLL_FILE_STEP 500
#define POLL_MAX_MS 60000

#if defined(CBM_USE_RUST_WATCHER_POLL_INTERVAL_ONLY)
extern int cbm_watcher_poll_interval_ms(int file_count);
#elif defined(CBM_USE_RUST_WATCHER_POLL_INTERVAL)
extern int cbm_rs_watcher_poll_interval_ms_v1(int file_count);

int cbm_watcher_poll_interval_ms(int file_count) {
    return cbm_rs_watcher_poll_interval_ms_v1(file_count);
}
#else
int cbm_watcher_poll_interval_ms(int file_count) {
    int ms = POLL_BASE_MS + ((file_count / POLL_FILE_STEP) * CBM_MSEC_PER_SEC);
    if (ms > POLL_MAX_MS) {
        ms = POLL_MAX_MS;
    }
    return ms;
}
#endif
