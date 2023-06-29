// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro
#include "blocks_mode_data.h"


static const char* UNDEFINED = "";


static void blocks_mode_private_data_update_string(BlocksModePrivateData* data, GString** str, const char* json_root_member, gboolean allow_null) {
    JsonNode* node = json_object_get_member(data->root, json_root_member);
    if (node == NULL) {
        return;
    } else if (json_node_is_null(node)) {
        page_data_set_string_member(str, allow_null ? NULL : "");
    } else if (json_node_get_value_type(node) == G_TYPE_STRING) {
        page_data_set_string_member(str, json_node_get_string(node));
    }
}

static void blocks_mode_private_data_update_icon(BlocksModePrivateData* data) {
    blocks_mode_private_data_update_string(data, &data->page->icon, "icon", TRUE);
}

static void blocks_mode_private_data_update_case_sensitivity(BlocksModePrivateData* data) {
    data->page->case_sensitive = json_object_get_boolean_member_or_else(
        data->root, "case_sensitive", data->page->case_sensitive
    );
}

static void blocks_mode_private_data_update_placeholder(BlocksModePrivateData* data) {
    blocks_mode_private_data_update_string(data, &data->page->placeholder, "placeholder", FALSE);
}

static void blocks_mode_private_data_update_filter(BlocksModePrivateData* data) {
    blocks_mode_private_data_update_string(data, &data->page->filter, "filter", TRUE);
}

static void blocks_mode_private_data_update_message(BlocksModePrivateData* data) {
    blocks_mode_private_data_update_string(data, &data->page->message, "message", TRUE);
}

static void blocks_mode_private_data_update_overlay(BlocksModePrivateData* data) {
    blocks_mode_private_data_update_string(data, &data->page->overlay, "overlay", TRUE);
}

static void blocks_mode_private_data_update_prompt(BlocksModePrivateData* data) {
    blocks_mode_private_data_update_string(data, &data->page->prompt, "prompt", TRUE);
}

static void blocks_mode_private_data_update_input(BlocksModePrivateData* data) {
    blocks_mode_private_data_update_string(data, &data->page->input, "input", FALSE);
}

static void blocks_mode_private_data_update_event_format(BlocksModePrivateData* data) {
    blocks_mode_private_data_update_string(data, &data->event_format, "event_format", FALSE);
}

static void blocks_mode_private_data_update_focus_entry(BlocksModePrivateData* data) {
    data->entry_to_focus = json_object_get_int_member_or_else(data->root, "active_line", -1);
}

static void blocks_mode_private_data_update_close_on_child_exit(BlocksModePrivateData* data) {
    gboolean orig = data->close_on_child_exit;
    gboolean now = json_object_get_boolean_member_or_else(data->root, "close_on_exit" , orig);
    data->close_on_child_exit = now;
}

static void blocks_mode_private_data_update_lines(BlocksModePrivateData* data) {
    JsonObject* root = data->root;
    PageData* page = data->page;
    const char* LINES_PROP = "lines";
    if (json_object_has_member(root, LINES_PROP)) {
        JsonArray* lines = json_object_get_array_member(data->root, LINES_PROP);
        page_data_clear_lines(page);
        size_t len = json_array_get_length(lines);
        for (int i = 0; i < len; ++i) {
            page_data_add_line_json_node(page, json_array_get_element(lines, i));
        }
    }
}



BlocksModePrivateData* blocks_mode_private_data_new() {
    BlocksModePrivateData* pd = g_malloc0(sizeof(*pd));
    pd->page = page_data_new();
    pd->page->markup_default = MarkupStatus_UNDEFINED;
    pd->event_format = g_string_new("{\"event\":\"{{event}}\", \"value\":\"{{value_escaped}}\", \"data\":\"{{data_escaped}}\"}");
    pd->entry_to_focus = -1;
    pd->tokens = NULL;
    pd->close_on_child_exit = TRUE;
    pd->cmd_pid = 0;
    pd->buffer = g_string_sized_new(1024);
    pd->active_line = g_string_sized_new(1024);
    pd->parser = json_parser_new();
    return pd;
}

void blocks_mode_private_data_update_destroy(BlocksModePrivateData* data){
    if (data->cmd_pid > 0) {
        kill(data->cmd_pid, SIGTERM);
    }
    if (data->read_channel_watcher > 0) {
        g_source_remove(data->read_channel_watcher);
    }
    if (data->parser) {
        g_object_unref(data->parser);
    }
    if (data->tokens) {
        helper_tokenize_free(data->tokens);
    }
    page_data_destroy(data->page);
    close(data->write_channel_fd);
    close(data->read_channel_fd);
    g_free(data->write_channel);
    g_free(data->read_channel);
    g_free(data);
}

void blocks_mode_private_data_update_page(BlocksModePrivateData* data){
    GError* error = NULL;
    if (!json_parser_load_from_data(data->parser, data->active_line->str, data->active_line->len, &error)) {
        fprintf(stderr, "Unable to parse line: %s\n", error->message);
        g_error_free(error);
        return;
    }

    data->root = json_node_get_object(json_parser_get_root(data->parser));

    blocks_mode_private_data_update_icon(data);
    blocks_mode_private_data_update_case_sensitivity(data);
    blocks_mode_private_data_update_placeholder(data);
    blocks_mode_private_data_update_filter(data);
    blocks_mode_private_data_update_message(data);
    blocks_mode_private_data_update_overlay(data);
    blocks_mode_private_data_update_input(data);
    blocks_mode_private_data_update_prompt(data);
    blocks_mode_private_data_update_close_on_child_exit(data);
    blocks_mode_private_data_update_event_format(data);
    blocks_mode_private_data_update_lines(data);
    blocks_mode_private_data_update_focus_entry(data);
}
