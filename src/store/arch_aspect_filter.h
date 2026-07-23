#ifndef CBM_STORE_ARCH_ASPECT_FILTER_H
#define CBM_STORE_ARCH_ASPECT_FILTER_H

#include <stdbool.h>

bool cbm_rs_store_arch_wants_aspect_v1(const char *const *aspects, int aspect_count,
                                       const char *name);
bool cbm_store_arch_wants_aspect(const char *const *aspects, int aspect_count,
                                 const char *name);

#endif
