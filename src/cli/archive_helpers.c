#include "cli/archive_helpers.h"

#include "foundation/constants.h"

enum {
    CBM_TAR_BLOCK_SIZE = CBM_SZ_512,
    CBM_BYTE_SHIFT = 8,
    CBM_PAIR = 2,
    CBM_TRIPLE = 3
};

#if defined(CBM_USE_RUST_CLI_ARCHIVE_HELPERS)
extern int cbm_rs_cli_archive_tar_end_v1(const unsigned char *hdr);
extern uint16_t cbm_rs_cli_archive_zip_read_u16_v1(const unsigned char *data, int off);
extern uint32_t cbm_rs_cli_archive_zip_read_u32_v1(const unsigned char *data, int off);
#endif

bool cbm_cli_is_tar_end_of_archive(const unsigned char *hdr) {
#if defined(CBM_USE_RUST_CLI_ARCHIVE_HELPERS)
    return cbm_rs_cli_archive_tar_end_v1(hdr) != 0;
#else
    if (!hdr) {
        return false;
    }
    for (int i = 0; i < CBM_TAR_BLOCK_SIZE; i++) {
        if (hdr[i] != 0) {
            return false;
        }
    }
    return true;
#endif
}

uint16_t cbm_cli_zip_read_u16(const unsigned char *data, int off) {
#if defined(CBM_USE_RUST_CLI_ARCHIVE_HELPERS)
    return cbm_rs_cli_archive_zip_read_u16_v1(data, off);
#else
    if (!data || off < 0) {
        return 0;
    }
    return (uint16_t)((uint16_t)data[off] | ((uint16_t)data[off + 1] << CBM_BYTE_SHIFT));
#endif
}

uint32_t cbm_cli_zip_read_u32(const unsigned char *data, int off) {
#if defined(CBM_USE_RUST_CLI_ARCHIVE_HELPERS)
    return cbm_rs_cli_archive_zip_read_u32_v1(data, off);
#else
    if (!data || off < 0) {
        return 0;
    }
    return (uint32_t)data[off] | ((uint32_t)data[off + 1] << CBM_BYTE_SHIFT) |
           ((uint32_t)data[off + CBM_PAIR] << (CBM_BYTE_SHIFT * CBM_PAIR)) |
           ((uint32_t)data[off + CBM_TRIPLE] << (CBM_BYTE_SHIFT * CBM_TRIPLE));
#endif
}
