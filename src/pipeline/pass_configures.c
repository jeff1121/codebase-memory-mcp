/*
 * pass_configures.c — Config key and env var helpers.
 *
 * Pure helper functions for detecting environment variable names,
 * normalizing config keys, and identifying config file extensions.
 */
#include "pipeline/pipeline.h"
#include "pipeline/pipeline_internal.h"
#include "pipeline/pipeline_is_env_var_name.h"
#include "pipeline/pipeline_normalize_config_key.h"
#include "pipeline/pipeline_has_config_extension.h"

#include <ctype.h>
#include <string.h>

#ifdef CBM_USE_RUST_PIPELINE_CONFIGURES
extern int cbm_rs_pipeline_is_env_var_name_v1(const char *s);
extern int cbm_rs_pipeline_normalize_config_key_v1(const char *key, char *norm_out, size_t norm_sz);
extern int cbm_rs_pipeline_has_config_extension_v1(const char *path);
#endif

/* cbm_is_env_var_name is now implemented in pipeline_is_env_var_name.c/.h */

/* cbm_normalize_config_key is now implemented in pipeline_normalize_config_key.c/.h */

/* cbm_has_config_extension is now implemented in pipeline_has_config_extension.c/.h */
