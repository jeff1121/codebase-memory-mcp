#include "pipeline/enrichment_tokens.h"

#include "foundation/compat.h"
#include "foundation/constants.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum { ENRICH_ATTR_SKIP = 2, ENRICH_MAX_CAMEL = 16 };

#ifdef CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS
extern int cbm_rs_pipeline_split_camel_case_v1(const char *value, char **out, int max_out);
extern int cbm_rs_pipeline_tokenize_decorator_v1(const char *value, char **out, int max_out);
#endif

#ifndef CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS
static bool is_decorator_stopword(const char *word) {
    static const char *stopwords[] = {"get",   "set",  "new",   "class",  "method", "function",
                                      "value", "type", "param", "return", "public", "private",
                                      "for",   "if",   "the",   "and",    "or",     "not",
                                      "with",  "from", "app",   "router", NULL};
    for (int i = 0; stopwords[i]; i++) {
        if (strcmp(word, stopwords[i]) == 0) {
            return true;
        }
    }
    return false;
}

static char ascii_tolower(char value) {
    unsigned char byte = (unsigned char)value;
    if (byte >= 'A' && byte <= 'Z') {
        return (char)(byte + ('a' - 'A'));
    }
    return value;
}
#endif

int cbm_split_camel_case(const char *s, char **out, int max_out) {
#ifdef CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS
    return cbm_rs_pipeline_split_camel_case_v1(s, out, max_out);
#else
    if (!s || !out || max_out <= 0) {
        return 0;
    }
    size_t len = strlen(s);
    if (len == 0) {
        return 0;
    }

    int count = 0;
    size_t start = 0;

    for (size_t i = SKIP_ONE; i < len; i++) {
        if (s[i] >= 'A' && s[i] <= 'Z' && s[i - SKIP_ONE] >= 'a' && s[i - SKIP_ONE] <= 'z') {
            if (count < max_out) {
                out[count] = cbm_strndup(s + start, i - start);
                count++;
            }
            start = i;
        }
    }
    if (count < max_out) {
        out[count] = cbm_strndup(s + start, len - start);
        count++;
    }
    return count;
#endif
}

#ifndef CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS
static char *strip_decorator_syntax(char *buf) {
    char *token = buf;
    if (*token == '@') {
        token++;
    }
    if (token[0] == '#' && token[SKIP_ONE] == '[') {
        token += ENRICH_ATTR_SKIP;
        size_t length = strlen(token);
        if (length > 0 && token[length - SKIP_ONE] == ']') {
            token[length - SKIP_ONE] = '\0';
        }
    }
    char *paren = strchr(token, '(');
    if (paren) {
        *paren = '\0';
    }
    for (char *cursor = token; *cursor; cursor++) {
        if (*cursor == '.' || *cursor == '_' || *cursor == '-' || *cursor == ':' ||
            *cursor == '/') {
            *cursor = ' ';
        }
    }
    return token;
}
#endif

int cbm_tokenize_decorator(const char *dec, char **out, int max_out) {
#ifdef CBM_USE_RUST_PIPELINE_ENRICHMENT_TOKENS
    return cbm_rs_pipeline_tokenize_decorator_v1(dec, out, max_out);
#else
    if (!dec || !out || max_out <= 0) {
        return 0;
    }

    char buf[CBM_SZ_256];
    size_t len = strlen(dec);
    if (len >= sizeof(buf)) {
        len = sizeof(buf) - SKIP_ONE;
    }
    memcpy(buf, dec, len);
    buf[len] = '\0';

    char *token = strip_decorator_syntax(buf);
    int count = 0;
    char *saveptr = NULL;
    char *part = strtok_r(token, " ", &saveptr);

    while (part && count < max_out) {
        char *camel_parts[ENRICH_MAX_CAMEL];
        int camel_count = cbm_split_camel_case(part, camel_parts, ENRICH_MAX_CAMEL);

        for (int i = 0; i < camel_count; i++) {
            if (count >= max_out) {
                free(camel_parts[i]);
                continue;
            }
            for (char *cursor = camel_parts[i]; *cursor; cursor++) {
                *cursor = ascii_tolower(*cursor);
            }
            if (strlen(camel_parts[i]) >= PAIR_LEN && !is_decorator_stopword(camel_parts[i])) {
                out[count++] = camel_parts[i];
            } else {
                free(camel_parts[i]);
            }
        }
        part = strtok_r(NULL, " ", &saveptr);
    }

    return count;
#endif
}
