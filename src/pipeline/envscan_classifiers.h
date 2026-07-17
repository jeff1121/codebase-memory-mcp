#ifndef CBM_PIPELINE_ENVSCAN_CLASSIFIERS_H
#define CBM_PIPELINE_ENVSCAN_CLASSIFIERS_H

/* 公開 bridge（pure 檔名／目錄 classifier；無 I/O／regex）：
 * - name/NULL 契約與 Rust pipeline_envscan 對齊：NULL 或 len≥256 → 0/false
 * - detect 回傳 file_type 整數：0=UNKNOWN … 7=PROPERTIES
 */
int cbm_pipeline_envscan_is_dockerfile_name(const char *name);
int cbm_pipeline_envscan_is_env_file_name(const char *name);
int cbm_pipeline_envscan_is_secret_file(const char *name);
int cbm_pipeline_envscan_is_ignored_dir(const char *name);
int cbm_pipeline_envscan_detect_file_type(const char *name);

enum {
    CBM_ENVSCAN_FT_UNKNOWN = 0,
    CBM_ENVSCAN_FT_DOCKERFILE = 1,
    CBM_ENVSCAN_FT_YAML = 2,
    CBM_ENVSCAN_FT_TERRAFORM = 3,
    CBM_ENVSCAN_FT_SHELL = 4,
    CBM_ENVSCAN_FT_ENVFILE = 5,
    CBM_ENVSCAN_FT_TOML = 6,
    CBM_ENVSCAN_FT_PROPERTIES = 7
};

#endif
