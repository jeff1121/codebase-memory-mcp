#ifndef CBM_GIT_JSON_ESCAPED_LEN_H
#define CBM_GIT_JSON_ESCAPED_LEN_H

/* 回傳 JSON escape 後、不含結尾 NUL 的長度；若加上 NUL 無法以 int 表示，回傳 -1。 */
int cbm_git_json_escaped_len(const char *src);

#endif
