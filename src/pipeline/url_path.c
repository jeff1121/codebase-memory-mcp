#include "pipeline/url_path.h"

#include <string.h>

enum {
    URL_PATH_SCHEME_SKIP = 3,
};

#if defined(CBM_USE_RUST_PIPELINE_URL_PATH)
extern const char *cbm_rs_pipeline_url_path_v1(const char *url);
#endif

const char *cbm_pipeline_url_path(const char *url) {
#if defined(CBM_USE_RUST_PIPELINE_URL_PATH)
    return cbm_rs_pipeline_url_path_v1(url);
#else
    if (!url) {
        return NULL;
    }
    const char *scheme_end = strstr(url, "://");
    if (!scheme_end) {
        return url;
    }
    const char *path = strchr(scheme_end + URL_PATH_SCHEME_SKIP, '/');
    return path ? path : "/";
#endif
}
