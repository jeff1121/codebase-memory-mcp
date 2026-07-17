#include "pipeline/route_node_classifiers.h"

#include <string.h>

#ifdef CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS
extern int cbm_rs_pipeline_is_hash_segment_v1(const unsigned char *segment, size_t len);
extern int cbm_rs_pipeline_is_broker_route_v1(const char *qn);
#endif

enum {
    RN_MAX_SCHEME = 12,
    RN_MIN_SCHEME = 3,
};

int cbm_pipeline_is_hash_segment(const unsigned char *segment, size_t slen) {
    if (!segment || slen == 0 || slen > RN_MAX_SCHEME) {
        return 0;
    }
#ifdef CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS
    return cbm_rs_pipeline_is_hash_segment_v1(segment, slen);
#else
    int has_letter = 0;
    for (size_t si = 0; si < slen; si++) {
        if (!((segment[si] >= '0' && segment[si] <= '9') ||
              (segment[si] >= 'a' && segment[si] <= 'z'))) {
            return 0;
        }
        if (segment[si] >= 'a' && segment[si] <= 'z') {
            has_letter = 1;
        }
    }
    /* Don't strip meaningful words (>=3 chars with letters like "api", "endpoint") */
    return !(has_letter && slen >= RN_MIN_SCHEME);
#endif
}

int cbm_pipeline_is_broker_route(const char *qn) {
#ifdef CBM_USE_RUST_PIPELINE_ROUTE_NODE_CLASSIFIERS
    return cbm_rs_pipeline_is_broker_route_v1(qn);
#else
    static const char *const prefixes[] = {
        "__route__infra__", "__route__pubsub__",          "__route__cloud_tasks__",
        "__route__async__", "__route__cloud_scheduler__", "__route__kafka__",
        "__route__sqs__"};
    if (!qn) {
        return 0;
    }
    for (int i = 0; i < (int)(sizeof(prefixes) / sizeof(prefixes[0])); i++) {
        if (strstr(qn, prefixes[i]) == qn) {
            return 1;
        }
    }
    return 0;
#endif
}
