/*
 * graph_buffer_mutation.c — graph buffer mutation command 轉接層。
 *
 * 此檔只把 command shape 同步 dispatch 到既有 C graph buffer API；
 * 不保存呼叫端指標、不延遲執行，也不改變底層 storage/ownership。
 */

#include "graph_buffer/graph_buffer.h"

enum { GB_MUTATION_ERR = -1 };

int64_t cbm_gbuf_apply_mutation(cbm_gbuf_t *gb, const cbm_gbuf_mutation_cmd_t *cmd) {
    if (!cmd || cmd->reserved0 != 0) {
        return GB_MUTATION_ERR;
    }
    switch ((cbm_gbuf_mutation_kind_t)cmd->kind) {
    case CBM_GBUF_MUTATION_UPSERT_NODE:
        return cbm_gbuf_upsert_node(gb, cmd->label, cmd->name, cmd->qualified_name, cmd->file_path,
                                    cmd->start_line, cmd->end_line, cmd->properties_json);
    case CBM_GBUF_MUTATION_INSERT_EDGE:
        return cbm_gbuf_insert_edge(gb, cmd->source_id, cmd->target_id, cmd->edge_type,
                                    cmd->properties_json);
    case CBM_GBUF_MUTATION_DELETE_BY_FILE:
        return cbm_gbuf_delete_by_file(gb, cmd->file_path);
    default:
        return GB_MUTATION_ERR;
    }
}

int64_t cbm_gbuf_apply_upsert_node(cbm_gbuf_t *gb, const char *label, const char *name,
                                   const char *qualified_name, const char *file_path,
                                   int start_line, int end_line, const char *properties_json) {
    const cbm_gbuf_mutation_cmd_t cmd = {
        .kind = CBM_GBUF_MUTATION_UPSERT_NODE,
        .label = label,
        .name = name,
        .qualified_name = qualified_name,
        .file_path = file_path,
        .start_line = start_line,
        .end_line = end_line,
        .properties_json = properties_json,
    };
    return cbm_gbuf_apply_mutation(gb, &cmd);
}

int64_t cbm_gbuf_apply_insert_edge(cbm_gbuf_t *gb, int64_t source_id, int64_t target_id,
                                   const char *type, const char *properties_json) {
    const cbm_gbuf_mutation_cmd_t cmd = {
        .kind = CBM_GBUF_MUTATION_INSERT_EDGE,
        .source_id = source_id,
        .target_id = target_id,
        .edge_type = type,
        .properties_json = properties_json,
    };
    return cbm_gbuf_apply_mutation(gb, &cmd);
}

int cbm_gbuf_apply_delete_by_file(cbm_gbuf_t *gb, const char *file_path) {
    const cbm_gbuf_mutation_cmd_t cmd = {
        .kind = CBM_GBUF_MUTATION_DELETE_BY_FILE,
        .file_path = file_path,
    };
    return (int)cbm_gbuf_apply_mutation(gb, &cmd);
}
