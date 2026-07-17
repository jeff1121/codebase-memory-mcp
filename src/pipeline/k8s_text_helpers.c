#include "pipeline/k8s_text_helpers.h"

#include "foundation/constants.h"

#include <string.h>

#if defined(CBM_USE_RUST_PIPELINE_K8S_HELPERS)
extern size_t cbm_rs_pipeline_k8s_basename_offset_v1(const char *path);
extern int cbm_rs_pipeline_k8s_indent_v1(const char *line);
extern int cbm_rs_pipeline_k8s_split_kv_v1(const char *text, char *key, size_t key_sz, char *val,
                                           size_t val_sz);
#endif

const char *cbm_pipeline_k8s_basename(const char *path) {
#if defined(CBM_USE_RUST_PIPELINE_K8S_HELPERS)
    return path ? path + cbm_rs_pipeline_k8s_basename_offset_v1(path) : NULL;
#else
    if (!path) {
        return NULL;
    }
    const char *p = strrchr(path, '/');
    return p ? p + SKIP_ONE : path;
#endif
}

int cbm_pipeline_k8s_indent(const char *line) {
#if defined(CBM_USE_RUST_PIPELINE_K8S_HELPERS)
    return cbm_rs_pipeline_k8s_indent_v1(line);
#else
    if (!line) {
        return 0;
    }
    int n = 0;
    while (line[n] == ' ') {
        n++;
    }
    return n;
#endif
}

int cbm_pipeline_k8s_split_kv(const char *text, char *key, size_t key_sz, char *val,
                              size_t val_sz) {
#if defined(CBM_USE_RUST_PIPELINE_K8S_HELPERS)
    return cbm_rs_pipeline_k8s_split_kv_v1(text, key, key_sz, val, val_sz);
#else
    if (!key || !val || key_sz == 0 || val_sz == 0) {
        return 0;
    }
    key[0] = '\0';
    val[0] = '\0';
    if (!text || text[0] == '#' || text[0] == '-' || text[0] == '\0') {
        return 0;
    }
    const char *colon = strchr(text, ':');
    if (!colon) {
        return 0;
    }
    size_t klen = (size_t)(colon - text);
    if (klen == 0 || klen >= key_sz) {
        return 0;
    }
    memcpy(key, text, klen);
    key[klen] = '\0';
    const char *v = colon + 1;
    while (*v == ' ' || *v == '\t') {
        v++;
    }
    size_t vn = 0;
    while (v[vn] && v[vn] != '\r' && v[vn] != '\n' && v[vn] != '#' && vn + 1 < val_sz) {
        val[vn] = v[vn];
        vn++;
    }
    while (vn > 0 && (val[vn - 1] == ' ' || val[vn - 1] == '\t')) {
        vn--;
    }
    val[vn] = '\0';
    if (vn >= 2 && (val[0] == '"' || val[0] == '\'') && val[vn - 1] == val[0]) {
        memmove(val, val + 1, vn - 2);
        val[vn - 2] = '\0';
    }
    return 1;
#endif
}
