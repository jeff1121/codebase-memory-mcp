#include "pipeline/parallel_json.h"

#ifdef CBM_USE_RUST_PIPELINE_PARALLEL_JSON
extern int cbm_rs_pipeline_parallel_json_escape_char(char *output, size_t available, int byte);
extern size_t cbm_rs_pipeline_parallel_json_escaped_len(const char *value);
#endif

int cbm_pipeline_parallel_json_escape_char(char *output, size_t available, int byte) {
#ifdef CBM_USE_RUST_PIPELINE_PARALLEL_JSON
    return cbm_rs_pipeline_parallel_json_escape_char(output, available, byte);
#else
    char escaped = 0;
    char value = (char)(unsigned char)byte;
    switch (value) {
    case '"':
        escaped = '"';
        break;
    case '\\':
        escaped = '\\';
        break;
    case '\n':
        escaped = 'n';
        break;
    case '\r':
        escaped = 'r';
        break;
    case '\t':
        escaped = 't';
        break;
    default:
        if (output != NULL && available >= 1) {
            output[0] = ((unsigned char)value < 0x20) ? ' ' : value;
        }
        return 1;
    }
    if (output != NULL && available >= 2) {
        output[0] = '\\';
        output[1] = escaped;
    }
    return 2;
#endif
}

size_t cbm_pipeline_parallel_json_escaped_len(const char *value) {
#ifdef CBM_USE_RUST_PIPELINE_PARALLEL_JSON
    return cbm_rs_pipeline_parallel_json_escaped_len(value);
#else
    size_t length = 0;
    if (value == NULL) {
        return 0;
    }
    for (; *value; value++) {
        switch (*value) {
        case '"':
        case '\\':
        case '\n':
        case '\r':
        case '\t':
            length += 2;
            break;
        default:
            length++;
            break;
        }
    }
    return length;
#endif
}
