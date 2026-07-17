/*
 * split_command.c -- Compile-command 字串 tokenizer。
 */
#include "foundation/constants.h"
#include "pipeline/split_command.h"

#include <stdlib.h>
#include <string.h>

#ifdef CBM_USE_RUST_PIPELINE_SPLIT_COMMAND
extern int cbm_rs_pipeline_split_command_v1(const char *cmd, char **out, int max_out);
#endif

#ifndef CBM_USE_RUST_PIPELINE_SPLIT_COMMAND
/* 將目前非空 token 輸出，回傳更新後的數量。 */
static int emit_token(char *current, int *clen, char **out, int count, int max_out) {
    if (*clen > 0 && count < max_out) {
        current[*clen] = '\0';
        out[count++] = strdup(current);
        *clen = 0;
    }
    return count;
}
#endif

int cbm_split_command(const char *cmd, char **out, int max_out) {
#ifdef CBM_USE_RUST_PIPELINE_SPLIT_COMMAND
    return cbm_rs_pipeline_split_command_v1(cmd, out, max_out);
#else
    if (!cmd || !out || max_out <= 0) {
        return 0;
    }

    int count = 0;
    char current[CBM_SZ_4K];
    int clen = 0;
    char in_quote = 0;

    for (int i = 0; cmd[i]; i++) {
        char c = cmd[i];
        if (in_quote) {
            if (c == in_quote) {
                in_quote = 0;
            } else if (clen < (int)sizeof(current) - SKIP_ONE) {
                current[clen++] = c;
            }
        } else if (c == '"' || c == '\'') {
            in_quote = c;
        } else if (c == ' ' || c == '\t') {
            count = emit_token(current, &clen, out, count, max_out);
        } else if (clen < (int)sizeof(current) - SKIP_ONE) {
            current[clen++] = c;
        }
    }
    return emit_token(current, &clen, out, count, max_out);
#endif
}
