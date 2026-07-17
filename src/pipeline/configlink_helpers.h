#ifndef CBM_PIPELINE_CONFIGLINK_HELPERS_H
#define CBM_PIPELINE_CONFIGLINK_HELPERS_H

#include <stddef.h>

/* 公開 bridge（pure classifier／buffer helper；無 graph ownership）：
 * - is_*：NULL → false/0
 * - lowercase_into：C 參數順序 (buf, bufsize, src)；ASCII lower
 * - match_dep_to_import：字串三元組（target_name, target_qn, dep_lower）
 */
int cbm_pipeline_configlink_is_manifest_file(const char *basename);
int cbm_pipeline_configlink_is_dep_section(const char *value);
int cbm_pipeline_configlink_is_cargo_dep_section(const char *qn);
void cbm_pipeline_configlink_lowercase_into(char *buf, size_t bufsize, const char *src);
double cbm_pipeline_configlink_match_dep_to_import(const char *target_name, const char *target_qn,
                                                   const char *dep_lower);

#endif
