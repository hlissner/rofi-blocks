// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro
#include "json_glib_extensions.h"
#include "page_data.h"
#include <rofi/helper.h>

static const gchar* EMPTY_STRING = "";

PageData* page_data_new() {
    PageData* page = g_malloc0(sizeof(*page));
    page->message = NULL;
    page->overlay = NULL;
    page->prompt = NULL;
    page->placeholder = NULL;
    page->filter = NULL;
    page->icon = NULL;
    page->trigger = NULL;
    page->case_sensitive = FALSE;
    page->input = g_string_sized_new(256);
    page->lines = g_array_new(FALSE, TRUE, sizeof(LineData));
    return page;
}

void page_data_destroy(PageData* page) {
    page_data_clear_lines(page);
    page->message != NULL && g_string_free(page->message, TRUE);
    page->overlay != NULL && g_string_free(page->overlay, TRUE);
    page->prompt != NULL && g_string_free(page->prompt, TRUE);
    page->placeholder != NULL && g_string_free(page->placeholder, TRUE);
    page->filter != NULL && g_string_free(page->filter, TRUE);
    page->icon != NULL && g_string_free(page->icon, TRUE);
    page->trigger != NULL && g_string_free(page->trigger, TRUE);
    g_string_free(page->input, TRUE);
    g_array_free(page->lines, TRUE);
    g_free(page);
}


static gboolean is_page_data_string_member_empty(GString* member) {
    return member == NULL || member->len <= 0;
}

static const char* get_page_data_string_member_or_empty_string(GString* member) {
    return member == NULL ? EMPTY_STRING : member->str;
}


void page_data_set_string_member(GString** member, const char* new_string) {
    gboolean is_defined = *member != NULL;
    gboolean will_define = new_string != NULL;
    if (is_defined && will_define) {
        g_string_assign(*member, new_string);
    } else if (is_defined && !will_define) {
        g_string_free(*member, TRUE);
        *member = NULL;
    } else if (!is_defined && will_define) {
        *member = g_string_new(new_string);
    }
    // else do nothing, *member is already NULL
}

gboolean page_data_is_string_equal(GString* a, GString* b) {
    if (a == NULL && b == NULL) {
        return TRUE;
    } else if (a != NULL && b != NULL) {
        return g_string_equal(a, b);
    }
    return FALSE;
}

gboolean page_data_is_message_empty(PageData* page) {
    return page ? is_page_data_string_member_empty(page->message) : TRUE;
}

const char* page_data_get_message_or_empty_string(PageData* page) {
    return page ? get_page_data_string_member_or_empty_string(page->message) : EMPTY_STRING;
}

void page_data_set_message(PageData* page, const char* message) {
    page_data_set_string_member(&page->message, message);
}

gboolean page_data_is_overlay_empty(PageData* page) {
    return page ? is_page_data_string_member_empty(page->overlay) : TRUE;
}

const char* page_data_get_overlay_or_empty_string(PageData* page) {
    return page ? get_page_data_string_member_or_empty_string(page->overlay) : EMPTY_STRING;
}

void page_data_set_overlay(PageData* page, const char* overlay) {
    page_data_set_string_member(&page->overlay, overlay);
}

void page_data_set_filter(PageData* page, const char* filter) {
    page_data_set_string_member(&page->filter, filter);
}



size_t page_data_get_number_of_lines(PageData* page) {
    return page->lines->len;
}

LineData* page_data_get_line_by_index_or_else(PageData* page, unsigned int index, LineData* else_value) {
    if (page == NULL || index >= page->lines->len) {
        return else_value;
    }
    LineData* result = &g_array_index(page->lines, LineData, index);
    return result;
}


void page_data_add_line(PageData* page,
                        const gchar* label,
                        const gchar* metatext,
                        const gchar* icon,
                        const gchar* data,
                        gboolean urgent,
                        gboolean highlight,
                        gboolean markup,
                        gboolean nonselectable,
                        gboolean filter) {
    LineData line = {
        .text = g_strdup(label),
        .metatext = g_strdup(metatext),
        .icon = g_strdup(icon),
        .data = g_strdup(data),
        .urgent = urgent,
        .highlight = highlight,
        .markup = markup,
        .nonselectable = nonselectable,
        .filter = filter
    };
    g_array_append_val(page->lines, line);
}

void page_data_add_line_json_node(PageData* page, JsonNode* node) {
    if (JSON_NODE_HOLDS_VALUE(node) && json_node_get_value_type(node) == G_TYPE_STRING) {
        page_data_add_line(page, json_node_get_string(node), NULL, EMPTY_STRING, EMPTY_STRING, FALSE, FALSE, page->markup_default == MarkupStatus_ENABLED, FALSE, TRUE);
    } else if (JSON_NODE_HOLDS_OBJECT(node)) {
        JsonObject* line_obj = json_node_get_object(node);
        const gchar* text = json_object_get_string_member_or_else(line_obj, "text", EMPTY_STRING);
        const gchar* metatext = json_object_get_string_member_or_else(line_obj, "metatext", NULL);
        const gchar* icon = json_object_get_string_member_or_else(line_obj, "icon", EMPTY_STRING);
        const gchar* data = json_object_get_string_member_or_else(line_obj, "data", EMPTY_STRING);
        gboolean urgent = json_object_get_boolean_member_or_else(line_obj, "urgent", FALSE);
        gboolean highlight = json_object_get_boolean_member_or_else(line_obj, "highlight", FALSE);
        gboolean markup = json_object_get_boolean_member_or_else(line_obj, "markup", page->markup_default == MarkupStatus_ENABLED);
        gboolean nonselectable = json_object_get_boolean_member_or_else(line_obj, "nonselectable", FALSE);
        gboolean filter = json_object_get_boolean_member_or_else(line_obj, "filter", TRUE);
        page_data_add_line(page, text, metatext, icon, data, urgent, highlight, markup, nonselectable, filter);
    }
}

void page_data_clear_lines(PageData* page) {
    GArray* lines = page->lines;
    int size = lines->len;
    for (int i = 0; i < size; ++i) {
        LineData line = g_array_index(lines, LineData, i);
        g_free(line.text);
        g_free(line.metatext);
        g_free(line.icon);
        g_free(line.data);
    }
    g_array_set_size(page->lines, 0);
}



