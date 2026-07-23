#ifndef CBM_PIPELINE_CROSS_REPO_JSON_H
#define CBM_PIPELINE_CROSS_REPO_JSON_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Extract a JSON string property from properties_json.
 * Writes into buf, returns buf on success, NULL on miss. */
const char *cbm_pipeline_cross_repo_json_str_prop(const char *json, const char *key, char *buf,
                                                  size_t bufsz);

/* Build CROSS_* edge properties JSON. */
void cbm_pipeline_cross_repo_build_props(char *buf, size_t bufsz, const char *target_project,
                                         const char *target_function, const char *target_file,
                                         const char *url_or_channel, const char *extra_key,
                                         const char *extra_val);

#ifdef __cplusplus
}
#endif

#endif /* CBM_PIPELINE_CROSS_REPO_JSON_H */
