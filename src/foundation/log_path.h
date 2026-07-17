#ifndef CBM_FOUNDATION_LOG_PATH_H
#define CBM_FOUNDATION_LOG_PATH_H

#include <stddef.h>

/* path 為 NULL 或 NUL 結尾字串；out 若非 NULL，不得與 path 重疊。 */
void cbm_log_copy_path_without_query(const char *path, char *out, size_t outsz);

#endif
