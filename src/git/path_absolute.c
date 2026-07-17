#include "git/path_absolute.h"

#if defined(CBM_USE_RUST_GIT_PATH_ABSOLUTE)
extern int cbm_rs_git_path_is_absolute_v1(const char *path);
#endif

bool cbm_git_path_is_absolute(const char *path) {
#if defined(CBM_USE_RUST_GIT_PATH_ABSOLUTE)
    return cbm_rs_git_path_is_absolute_v1(path) != 0;
#else
    if (!path || !path[0]) {
        return false;
    }
    if (path[0] == '/') {
        return true;
    }
#ifdef _WIN32
    /* Windows 磁碟機代號依語法限定為 ASCII 字母，避免受 process locale 影響。 */
    const unsigned char drive = (unsigned char)path[0];
    return ((drive >= 'A' && drive <= 'Z') || (drive >= 'a' && drive <= 'z')) && path[1] == ':';
#else
    return false;
#endif
#endif
}
