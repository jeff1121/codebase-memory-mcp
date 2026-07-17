#include "pipeline/envscan_classifiers.h"

#include "foundation/constants.h"

#include <ctype.h>
#include <string.h>

#define SLEN(s) (sizeof(s) - 1)
enum { ENV_EXT_LEN = 4 }; /* strlen(".env") */

#if defined(CBM_USE_RUST_PIPELINE_ENVSCAN_CLASSIFIERS)
extern int cbm_rs_pipeline_envscan_is_dockerfile_name_v1(const char *name);
extern int cbm_rs_pipeline_envscan_is_env_file_name_v1(const char *name);
extern int cbm_rs_pipeline_envscan_is_secret_file_v1(const char *name);
extern int cbm_rs_pipeline_envscan_is_ignored_dir_v1(const char *name);
extern int cbm_rs_pipeline_envscan_detect_file_type_v1(const char *name);
#endif

int cbm_pipeline_envscan_is_dockerfile_name(const char *name) {
#if defined(CBM_USE_RUST_PIPELINE_ENVSCAN_CLASSIFIERS)
    return cbm_rs_pipeline_envscan_is_dockerfile_name_v1(name);
#else
    if (!name) {
        return 0;
    }
    char lower[CBM_SZ_256];
    size_t len = strlen(name);
    if (len >= sizeof(lower)) {
        return 0;
    }
    for (size_t i = 0; i <= len; i++) {
        lower[i] = (char)tolower((unsigned char)name[i]);
    }
    if (strcmp(lower, "dockerfile") == 0) {
        return SKIP_ONE;
    }
#define DOCKERFILE_SUFFIX_LEN 11
    if (strncmp(lower, "dockerfile.", DOCKERFILE_SUFFIX_LEN) == 0) {
        return SKIP_ONE;
    }
    if (len > DOCKERFILE_SUFFIX_LEN &&
        strcmp(lower + len - DOCKERFILE_SUFFIX_LEN, ".dockerfile") == 0) {
        return SKIP_ONE;
    }
    return 0;
#endif
}

int cbm_pipeline_envscan_is_env_file_name(const char *name) {
#if defined(CBM_USE_RUST_PIPELINE_ENVSCAN_CLASSIFIERS)
    return cbm_rs_pipeline_envscan_is_env_file_name_v1(name);
#else
    if (!name) {
        return 0;
    }
    char lower[CBM_SZ_256];
    size_t len = strlen(name);
    if (len >= sizeof(lower)) {
        return 0;
    }
    for (size_t i = 0; i <= len; i++) {
        lower[i] = (char)tolower((unsigned char)name[i]);
    }
    if (strcmp(lower, ".env") == 0) {
        return SKIP_ONE;
    }
    if (strncmp(lower, ".env.", SLEN(".env.")) == 0) {
        return SKIP_ONE;
    }
    if (len > ENV_EXT_LEN && strcmp(lower + len - ENV_EXT_LEN, ".env") == 0) {
        return SKIP_ONE;
    }
    return 0;
#endif
}

int cbm_pipeline_envscan_is_secret_file(const char *name) {
#if defined(CBM_USE_RUST_PIPELINE_ENVSCAN_CLASSIFIERS)
    return cbm_rs_pipeline_envscan_is_secret_file_v1(name);
#else
    if (!name) {
        return 0;
    }
    char lower[CBM_SZ_256];
    size_t len = strlen(name);
    if (len >= sizeof(lower)) {
        return 0;
    }
    for (size_t i = 0; i <= len; i++) {
        lower[i] = (char)tolower((unsigned char)name[i]);
    }
    static const char *patterns[] = {
        "service_account", "credentials", "key.json", "key.pem", "id_rsa",
        "id_ed25519",      ".pem",        ".key",     NULL};
    for (int i = 0; patterns[i]; i++) {
        if (strstr(lower, patterns[i])) {
            return SKIP_ONE;
        }
    }
    return 0;
#endif
}

int cbm_pipeline_envscan_is_ignored_dir(const char *name) {
#if defined(CBM_USE_RUST_PIPELINE_ENVSCAN_CLASSIFIERS)
    return cbm_rs_pipeline_envscan_is_ignored_dir_v1(name);
#else
    if (!name) {
        return 0;
    }
    static const char *dirs[] = {
        ".git",  "node_modules", ".svn", ".hg",   "__pycache__", "vendor", ".terraform", ".cache",
        ".idea", ".vscode",      "dist", "build", ".next",       ".nuxt",  "target",     NULL};
    for (int i = 0; dirs[i]; i++) {
        if (strcmp(name, dirs[i]) == 0) {
            return SKIP_ONE;
        }
    }
    return 0;
#endif
}

int cbm_pipeline_envscan_detect_file_type(const char *name) {
#if defined(CBM_USE_RUST_PIPELINE_ENVSCAN_CLASSIFIERS)
    return cbm_rs_pipeline_envscan_detect_file_type_v1(name);
#else
    if (!name) {
        return CBM_ENVSCAN_FT_UNKNOWN;
    }
    if (cbm_pipeline_envscan_is_dockerfile_name(name)) {
        return CBM_ENVSCAN_FT_DOCKERFILE;
    }
    if (cbm_pipeline_envscan_is_env_file_name(name)) {
        return CBM_ENVSCAN_FT_ENVFILE;
    }
    const char *ext = strrchr(name, '.');
    if (!ext) {
        return CBM_ENVSCAN_FT_UNKNOWN;
    }
    if (strcmp(ext, ".yaml") == 0 || strcmp(ext, ".yml") == 0) {
        return CBM_ENVSCAN_FT_YAML;
    }
    if (strcmp(ext, ".tf") == 0 || strcmp(ext, ".hcl") == 0) {
        return CBM_ENVSCAN_FT_TERRAFORM;
    }
    if (strcmp(ext, ".sh") == 0 || strcmp(ext, ".bash") == 0 || strcmp(ext, ".zsh") == 0) {
        return CBM_ENVSCAN_FT_SHELL;
    }
    if (strcmp(ext, ".toml") == 0) {
        return CBM_ENVSCAN_FT_TOML;
    }
    if (strcmp(ext, ".properties") == 0 || strcmp(ext, ".cfg") == 0 || strcmp(ext, ".ini") == 0) {
        return CBM_ENVSCAN_FT_PROPERTIES;
    }
    return CBM_ENVSCAN_FT_UNKNOWN;
#endif
}
