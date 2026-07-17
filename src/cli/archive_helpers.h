#ifndef CBM_CLI_ARCHIVE_HELPERS_H
#define CBM_CLI_ARCHIVE_HELPERS_H

#include <stdbool.h>
#include <stdint.h>

/* 公開 bridge（pure byte helpers；無 I/O／ownership）：
 * - cbm_cli_is_tar_end_of_archive：hdr 為 NULL → false；否則檢查前 512 bytes 是否全 0。
 * - cbm_cli_zip_read_u16/u32：little-endian 讀取；data NULL 或 off < 0 → 0。
 * gzip 解壓、tar/zip entry 邊界、malloc ownership 與安裝流程仍由 C 負責。
 */
bool cbm_cli_is_tar_end_of_archive(const unsigned char *hdr);
uint16_t cbm_cli_zip_read_u16(const unsigned char *data, int off);
uint32_t cbm_cli_zip_read_u32(const unsigned char *data, int off);

#endif
