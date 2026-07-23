#ifndef CBM_PIPELINE_CONFIGLINK_PATH_BASENAME_H
#define CBM_PIPELINE_CONFIGLINK_PATH_BASENAME_H

/* 回傳 path 最後一個 ASCII '/' 後的借用指標，不配置或寫入。 */
const char *cbm_pipeline_configlink_path_basename(const char *path);

#endif
