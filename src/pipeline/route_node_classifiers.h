#ifndef CBM_PIPELINE_ROUTE_NODE_CLASSIFIERS_H
#define CBM_PIPELINE_ROUTE_NODE_CLASSIFIERS_H

#include <stddef.h>

int cbm_pipeline_is_hash_segment(const unsigned char *segment, size_t slen);
int cbm_pipeline_is_broker_route(const char *qn);

#endif
