#ifndef CBM_PIPELINE_URL_PATH_H
#define CBM_PIPELINE_URL_PATH_H

/* 回傳 NULL，或指向輸入字串內部／靜態根路徑的唯讀 borrowed pointer；呼叫端不得釋放。 */
const char *cbm_pipeline_url_path(const char *url);

#endif
