/*
 * compat_regex.c — Portable regular expression implementation.
 *
 * POSIX: direct wrappers around <regex.h>.
 * Windows: vendored TRE regex library (BSD-licensed).
 */
#include "foundation/constants.h"
#include "foundation/compat_regex.h"

#include <string.h>

#if defined(CBM_USE_RUST_COMPAT_REGEX) || defined(CBM_USE_RUST_COMPAT_REGEX_ONLY)
extern int cbm_rs_compat_regex_known_flags(int flags);
extern int cbm_rs_compat_regex_match_cap(int nmatch, int has_matches);
extern int cbm_rs_compat_regex_status(int matched);
#endif

#ifdef _WIN32

/* ── Windows: use vendored TRE regex ─────────────────────────── */
#include "../../vendored/tre/regex.h"

_Static_assert(sizeof(regex_t) <= CBM_SZ_256,
               "cbm_regex_t opaque buffer too small for TRE regex_t");

static int translate_flags_tre(int flags) {
#if defined(CBM_USE_RUST_COMPAT_REGEX) || defined(CBM_USE_RUST_COMPAT_REGEX_ONLY)
    return cbm_rs_compat_regex_known_flags(flags);
#else
    int tre_flags = 0;
    if (flags & CBM_REG_EXTENDED)
        tre_flags |= REG_EXTENDED;
    if (flags & CBM_REG_ICASE)
        tre_flags |= REG_ICASE;
    if (flags & CBM_REG_NOSUB)
        tre_flags |= REG_NOSUB;
    if (flags & CBM_REG_NEWLINE)
        tre_flags |= REG_NEWLINE;
    return tre_flags;
#endif
}

int cbm_regcomp(cbm_regex_t *r, const char *pattern, int flags) {
    regex_t *re = (regex_t *)r->opaque;
    int rc = tre_regcomp(re, pattern, translate_flags_tre(flags));
    return rc == 0 ? CBM_REG_OK : rc;
}

int cbm_regexec(const cbm_regex_t *r, const char *str, int nmatch, cbm_regmatch_t *matches,
                int eflags) {
    const regex_t *re = (const regex_t *)r->opaque;
    if (nmatch <= 0 || !matches) {
        int rc = tre_regexec(re, str, 0, NULL, eflags);
#if defined(CBM_USE_RUST_COMPAT_REGEX) || defined(CBM_USE_RUST_COMPAT_REGEX_ONLY)
        return cbm_rs_compat_regex_status(rc == 0);
#else
        return rc == 0 ? CBM_REG_OK : CBM_REG_NOMATCH;
#endif
    }
    regmatch_t pmatch[CBM_SZ_32];
#if defined(CBM_USE_RUST_COMPAT_REGEX) || defined(CBM_USE_RUST_COMPAT_REGEX_ONLY)
    int n = cbm_rs_compat_regex_match_cap(nmatch, 1);
#else
    int n = nmatch > CBM_SZ_32 ? CBM_SZ_32 : nmatch;
#endif
    int rc = tre_regexec(re, str, (size_t)n, pmatch, eflags);
    if (rc != 0) {
#if defined(CBM_USE_RUST_COMPAT_REGEX) || defined(CBM_USE_RUST_COMPAT_REGEX_ONLY)
        return cbm_rs_compat_regex_status(0);
#else
        return CBM_REG_NOMATCH;
#endif
    }
    for (int i = 0; i < n; i++) {
        matches[i].rm_so = (int)pmatch[i].rm_so;
        matches[i].rm_eo = (int)pmatch[i].rm_eo;
    }
#if defined(CBM_USE_RUST_COMPAT_REGEX) || defined(CBM_USE_RUST_COMPAT_REGEX_ONLY)
    return cbm_rs_compat_regex_status(1);
#else
    return CBM_REG_OK;
#endif
}

void cbm_regfree(cbm_regex_t *r) {
    regex_t *re = (regex_t *)r->opaque;
    tre_regfree(re);
}

#else /* POSIX */

/* ── POSIX implementation ─────────────────────────────────────── */

#include <regex.h>

/* Static assert: our opaque buffer is large enough for regex_t.
 * If this fires, increase cbm_regex_t.opaque size. */
_Static_assert(sizeof(regex_t) <= CBM_SZ_256, "cbm_regex_t opaque buffer too small for regex_t");

static int translate_flags(int flags) {
#if defined(CBM_USE_RUST_COMPAT_REGEX) || defined(CBM_USE_RUST_COMPAT_REGEX_ONLY)
    return cbm_rs_compat_regex_known_flags(flags);
#else
    int posix_flags = 0;
    if (flags & CBM_REG_EXTENDED) {
        posix_flags |= REG_EXTENDED;
    }
    if (flags & CBM_REG_ICASE) {
        posix_flags |= REG_ICASE;
    }
    if (flags & CBM_REG_NOSUB) {
        posix_flags |= REG_NOSUB;
    }
    if (flags & CBM_REG_NEWLINE) {
        posix_flags |= REG_NEWLINE;
    }
    return posix_flags;
#endif
}

int cbm_regcomp(cbm_regex_t *r, const char *pattern, int flags) {
    regex_t *re = (regex_t *)r->opaque;
    int rc = regcomp(re, pattern, translate_flags(flags));
    return rc == 0 ? CBM_REG_OK : rc;
}

int cbm_regexec(const cbm_regex_t *r, const char *str, int nmatch, cbm_regmatch_t *matches,
                int eflags) {
    const regex_t *re = (const regex_t *)r->opaque;

    if (nmatch <= 0 || !matches) {
        int rc = regexec(re, str, 0, NULL, eflags);
#if defined(CBM_USE_RUST_COMPAT_REGEX) || defined(CBM_USE_RUST_COMPAT_REGEX_ONLY)
        return cbm_rs_compat_regex_status(rc == 0);
#else
        return rc == 0 ? CBM_REG_OK : CBM_REG_NOMATCH;
#endif
    }

    regmatch_t pmatch[CBM_SZ_32];
#if defined(CBM_USE_RUST_COMPAT_REGEX) || defined(CBM_USE_RUST_COMPAT_REGEX_ONLY)
    int n = cbm_rs_compat_regex_match_cap(nmatch, 1);
#else
    int n = nmatch > CBM_SZ_32 ? CBM_SZ_32 : nmatch;
#endif
    int rc = regexec(re, str, (size_t)n, pmatch, eflags);
    if (rc != 0) {
#if defined(CBM_USE_RUST_COMPAT_REGEX) || defined(CBM_USE_RUST_COMPAT_REGEX_ONLY)
        return cbm_rs_compat_regex_status(0);
#else
        return CBM_REG_NOMATCH;
#endif
    }

    for (int i = 0; i < n; i++) {
        matches[i].rm_so = (int)pmatch[i].rm_so;
        matches[i].rm_eo = (int)pmatch[i].rm_eo;
    }
#if defined(CBM_USE_RUST_COMPAT_REGEX) || defined(CBM_USE_RUST_COMPAT_REGEX_ONLY)
    return cbm_rs_compat_regex_status(1);
#else
    return CBM_REG_OK;
#endif
}

void cbm_regfree(cbm_regex_t *r) {
    regex_t *re = (regex_t *)r->opaque;
    regfree(re);
}

#endif /* _WIN32 */
