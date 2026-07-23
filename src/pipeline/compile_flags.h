/*
 * compile_flags.h — Helper to extract compiler flags (-I, -D, -isystem).
 */
#ifndef CBM_PIPELINE_COMPILE_FLAGS_H
#define CBM_PIPELINE_COMPILE_FLAGS_H

#include <stdbool.h>
#include "pipeline/pipeline_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

bool cbm_pipeline_try_include_flag(cbm_compile_flags_t *f, const char **args, int argc, int *i,
                                   const char *directory);

bool cbm_pipeline_try_define_flag(cbm_compile_flags_t *f, const char **args, int argc, int *i);

#ifdef __cplusplus
}
#endif

#endif /* CBM_PIPELINE_COMPILE_FLAGS_H */
