#include "pipeline/definitions_json.h"

#ifdef CBM_USE_RUST_PIPELINE_DEFINITIONS_JSON
extern int cbm_rs_pipeline_definitions_json_escape_char(char *buf, size_t avail, int byte);
extern size_t cbm_rs_pipeline_definitions_json_escaped_len(const char *value);

int cbm_pipeline_definitions_json_escape_char(char *buf, size_t avail, int byte) {
    return cbm_rs_pipeline_definitions_json_escape_char(buf, avail, (unsigned char)byte);
}

size_t cbm_pipeline_definitions_json_escaped_len(const char *value) {
    return cbm_rs_pipeline_definitions_json_escaped_len(value);
}
#else
int cbm_pipeline_definitions_json_escape_char(char *buf, size_t avail, int byte) {
    const unsigned char value = (unsigned char)byte;
    char escaped = '\0';

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
        break;
    }

    if (escaped == '\0') {
        if (buf && avail >= 1) {
            buf[0] = value < 0x20 ? ' ' : (char)value;
        }
        return 1;
    }
    if (buf && avail >= 2) {
        buf[0] = '\\';
        buf[1] = escaped;
    }
    return 2;
}

size_t cbm_pipeline_definitions_json_escaped_len(const char *value) {
    const unsigned char *byte = (const unsigned char *)value;
    size_t length = 0;

    if (!byte) {
        return 0;
    }
    for (; *byte; byte++) {
        switch (*byte) {
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
}
#endif
