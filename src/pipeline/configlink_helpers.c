#include "pipeline/configlink_helpers.h"

#include "foundation/compat.h"
#include "foundation/constants.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define CONF_DEP_EXACT 0.95
#define CONF_DEP_QN_SUBSTR 0.80

#if defined(CBM_USE_RUST_PIPELINE_CONFIGLINK_HELPERS)
extern int cbm_rs_pipeline_configlink_is_manifest_file_v1(const char *basename);
extern int cbm_rs_pipeline_configlink_is_dep_section_v1(const char *value);
extern int cbm_rs_pipeline_configlink_is_cargo_dep_section_v1(const char *value);
extern double cbm_rs_pipeline_configlink_match_dep_to_import_v1(const char *target_name,
                                                                const char *target_qn,
                                                                const char *dep_lower);
extern void cbm_rs_pipeline_configlink_lowercase_into_v1(const char *value, char *buf,
                                                         size_t bufsize);
#endif

int cbm_pipeline_configlink_is_manifest_file(const char *basename) {
#if defined(CBM_USE_RUST_PIPELINE_CONFIGLINK_HELPERS)
    return cbm_rs_pipeline_configlink_is_manifest_file_v1(basename);
#else
    if (!basename) {
        return 0;
    }
    static const char *names[] = {"Cargo.toml",       "package.json",  "go.mod",
                                  "requirements.txt", "Gemfile",       "build.gradle",
                                  "pom.xml",          "composer.json", NULL};
    for (int i = 0; names[i]; i++) {
        if (strcmp(basename, names[i]) == 0) {
            return 1;
        }
    }
    return 0;
#endif
}

int cbm_pipeline_configlink_is_dep_section(const char *value) {
#if defined(CBM_USE_RUST_PIPELINE_CONFIGLINK_HELPERS)
    return cbm_rs_pipeline_configlink_is_dep_section_v1(value);
#else
    if (!value) {
        return 0;
    }
    static const char *secs[] = {"dependencies",     "devdependencies",    "peerdependencies",
                                 "dev-dependencies", "build-dependencies", NULL};
    for (int i = 0; secs[i]; i++) {
        if (cbm_strcasestr(value, secs[i]) != NULL) {
            return 1;
        }
    }
    return 0;
#endif
}

int cbm_pipeline_configlink_is_cargo_dep_section(const char *qn) {
#if defined(CBM_USE_RUST_PIPELINE_CONFIGLINK_HELPERS)
    return cbm_rs_pipeline_configlink_is_cargo_dep_section_v1(qn);
#else
    if (!qn) {
        return 0;
    }
    char qn_copy[CBM_SZ_512];
    snprintf(qn_copy, sizeof(qn_copy), "%s", qn);
    char *saveptr = NULL;
    char *part = strtok_r(qn_copy, ".", &saveptr);
    while (part) {
        char lower[CBM_SZ_128];
        size_t plen = strlen(part);
        if (plen >= sizeof(lower)) {
            plen = sizeof(lower) - SKIP_ONE;
        }
        for (size_t j = 0; j < plen; j++) {
            lower[j] = (char)tolower((unsigned char)part[j]);
        }
        lower[plen] = '\0';
        static const char *dep_secs[] = {"dependencies",       "devdependencies",
                                         "peerdependencies",   "dev-dependencies",
                                         "build-dependencies", NULL};
        for (int k = 0; dep_secs[k]; k++) {
            if (strcmp(lower, dep_secs[k]) == 0) {
                return 1;
            }
        }
        part = strtok_r(NULL, ".", &saveptr);
    }
    return 0;
#endif
}

void cbm_pipeline_configlink_lowercase_into(char *buf, size_t bufsize, const char *src) {
#if defined(CBM_USE_RUST_PIPELINE_CONFIGLINK_HELPERS)
    cbm_rs_pipeline_configlink_lowercase_into_v1(src, buf, bufsize);
#else
    if (!buf || bufsize == 0) {
        return;
    }
    size_t len = src ? strlen(src) : 0;
    for (size_t j = 0; j < len && j < bufsize - SKIP_ONE; j++) {
        buf[j] = (char)tolower((unsigned char)src[j]);
    }
    buf[len < bufsize ? len : bufsize - SKIP_ONE] = '\0';
#endif
}

double cbm_pipeline_configlink_match_dep_to_import(const char *target_name, const char *target_qn,
                                                   const char *dep_lower) {
#if defined(CBM_USE_RUST_PIPELINE_CONFIGLINK_HELPERS)
    return cbm_rs_pipeline_configlink_match_dep_to_import_v1(target_name, target_qn, dep_lower);
#else
    if (!target_name || !dep_lower) {
        return 0.0;
    }
    char target_lower[CBM_SZ_256];
    cbm_pipeline_configlink_lowercase_into(target_lower, sizeof(target_lower), target_name);
    if (strcmp(target_lower, dep_lower) == 0) {
        return CONF_DEP_EXACT;
    }
    if (target_qn) {
        char qn_lower[CBM_SZ_512];
        cbm_pipeline_configlink_lowercase_into(qn_lower, sizeof(qn_lower), target_qn);
        if (strstr(qn_lower, dep_lower) != NULL) {
            return CONF_DEP_QN_SUBSTR;
        }
    }
    return 0.0;
#endif
}
