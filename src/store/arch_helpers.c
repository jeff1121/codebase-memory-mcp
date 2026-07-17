#include "foundation/constants.h"
#include "foundation/compat.h"
#include "store/store.h"

#include <string.h>

enum {
    ARCH_COL_2 = 2,
    ARCH_QN_MAX_DOTS = 5,
    ARCH_QN_MIN_DOTS = 3,
};

#ifdef CBM_USE_RUST_STORE_ARCH_HELPERS
extern int cbm_rs_store_hop_to_risk_v1(int hop);
extern size_t cbm_rs_store_risk_label_v1(char *buf, size_t bufsize, int level);
extern size_t cbm_rs_store_qn_to_package_v1(char *buf, size_t bufsize, const char *qn);
extern size_t cbm_rs_store_qn_to_top_package_v1(char *buf, size_t bufsize, const char *qn);
extern int cbm_rs_store_is_test_file_path_v1(const char *path);
#endif

cbm_risk_level_t cbm_hop_to_risk(int hop) {
#ifdef CBM_USE_RUST_STORE_ARCH_HELPERS
    int level = cbm_rs_store_hop_to_risk_v1(hop);
    if (level >= CBM_RISK_CRITICAL && level <= CBM_RISK_LOW) {
        return (cbm_risk_level_t)level;
    }
    return CBM_RISK_LOW;
#else
    switch (hop) {
    case SKIP_ONE:
        return CBM_RISK_CRITICAL;
    case ARCH_COL_2:
        return CBM_RISK_HIGH;
    case 3:
        return CBM_RISK_MEDIUM;
    default:
        return CBM_RISK_LOW;
    }
#endif
}

const char *cbm_risk_label(cbm_risk_level_t level) {
#ifdef CBM_USE_RUST_STORE_ARCH_HELPERS
    static CBM_TLS char buf[CBM_SZ_16];
    (void)cbm_rs_store_risk_label_v1(buf, sizeof(buf), (int)level);
    return buf;
#else
    switch (level) {
    case CBM_RISK_CRITICAL:
        return "CRITICAL";
    case CBM_RISK_HIGH:
        return "HIGH";
    case CBM_RISK_MEDIUM:
        return "MEDIUM";
    case CBM_RISK_LOW:
    default:
        return "LOW";
    }
#endif
}

/* 從 QN 擷取子 package：project.dir1.dir2.sym -> dir2，否則取第二段。 */
const char *cbm_qn_to_package(const char *qn) {
#ifdef CBM_USE_RUST_STORE_ARCH_HELPERS
    static CBM_TLS char buf[CBM_SZ_256];
    (void)cbm_rs_store_qn_to_package_v1(buf, sizeof(buf), qn);
    return buf;
#else
    if (!qn || !qn[0]) {
        return "";
    }
    static CBM_TLS char buf[CBM_SZ_256];
    const char *dots[ARCH_QN_MAX_DOTS] = {NULL};
    int ndots = 0;
    for (const char *p = qn; *p && ndots < ARCH_QN_MAX_DOTS; p++) {
        if (*p == '.') {
            dots[ndots++] = p;
        }
    }
    if (ndots >= ARCH_QN_MIN_DOTS) {
        const char *start = dots[SKIP_ONE] + SKIP_ONE;
        int len = (int)(dots[ARCH_COL_2] - start);
        if (len > 0 && len < (int)sizeof(buf)) {
            memcpy(buf, start, (size_t)len);
            buf[len] = '\0';
            return buf;
        }
    }
    if (ndots >= SKIP_ONE) {
        const char *start = dots[0] + SKIP_ONE;
        const char *end = (ndots >= ARCH_COL_2) ? dots[SKIP_ONE] : qn + strlen(qn);
        int len = (int)(end - start);
        if (len > 0 && len < (int)sizeof(buf)) {
            memcpy(buf, start, (size_t)len);
            buf[len] = '\0';
            return buf;
        }
    }
    return "";
#endif
}

/* 從 QN 擷取頂層 package：project.dir1.rest -> dir1。 */
const char *cbm_qn_to_top_package(const char *qn) {
#ifdef CBM_USE_RUST_STORE_ARCH_HELPERS
    static CBM_TLS char buf[CBM_SZ_256];
    (void)cbm_rs_store_qn_to_top_package_v1(buf, sizeof(buf), qn);
    return buf;
#else
    if (!qn || !qn[0]) {
        return "";
    }
    static CBM_TLS char buf[CBM_SZ_256];
    const char *first_dot = strchr(qn, '.');
    if (!first_dot) {
        return "";
    }
    const char *start = first_dot + SKIP_ONE;
    const char *second_dot = strchr(start, '.');
    const char *end = second_dot ? second_dot : qn + strlen(qn);
    int len = (int)(end - start);
    if (len > 0 && len < (int)sizeof(buf)) {
        memcpy(buf, start, (size_t)len);
        buf[len] = '\0';
        return buf;
    }
    return "";
#endif
}

bool cbm_is_test_file_path(const char *fp) {
#ifdef CBM_USE_RUST_STORE_ARCH_HELPERS
    return cbm_rs_store_is_test_file_path_v1(fp) != 0;
#else
    if (!fp || fp[0] == '\0') {
        return false;
    }
    return strstr(fp, "test") != NULL;
#endif
}
