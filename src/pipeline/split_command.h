/*
 * split_command.h -- 內部 compile-command tokenizer。
 */
#ifndef CBM_SPLIT_COMMAND_H
#define CBM_SPLIT_COMMAND_H

/* 將 shell-like command string 拆分為 arguments。
 * out 必須提供至少 max_out 個 slot。正常資源條件下，每個回傳的
 * out[i] 由 caller 擁有且必須 free。函式不初始化未使用的 slot，
 * 也不附加 out[count] sentinel；請以回傳的 count 作為陣列界線。 */
int cbm_split_command(const char *cmd, char **out, int max_out);

#endif /* CBM_SPLIT_COMMAND_H */
