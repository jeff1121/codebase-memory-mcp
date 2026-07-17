/* mem Rust FFI smoke 專用 system-info stub。 */

#include "platform.h"

cbm_system_info_t cbm_system_info(void) {
    cbm_system_info_t info = {0};
    info.total_cores = 1;
    info.perf_cores = 1;
    info.total_ram = (size_t)8 * 1024 * 1024 * 1024;
    return info;
}
