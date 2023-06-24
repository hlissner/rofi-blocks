// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro

#ifndef ROFI_BLOCKS_PAGE_DATA_H
#define ROFI_BLOCKS_PAGE_DATA_H
#include <gmodule.h>
#include <stdint.h>
#include <json-glib/json-glib.h>

typedef enum {
    MarkupStatus_UNDEFINED = 0,
    MarkupStatus_ENABLED = 1,
    MarkupStatus_DISABLED = 2
} MarkupStatus;

typedef struct {
    MarkupStatus markup_default;
    GString* message;
    GString* overlay;
    GString* prompt;
    GString* input;
    GArray* lines;
} PageData;

typedef struct {
    gchar* text;
    gchar* icon;
    gchar* data;
    gboolean urgent;
    gboolean highlight;
    gboolean markup;
    gboolean nonselectable;
    uint32_t icon_fetch_uid; //cache icon uid
} LineData;

PageData* page_data_new();

void page_data_destroy(PageData* page);

void page_data_set_string_member(GString** member, const char* new_string);

gboolean page_data_is_string_equal(GString* a, GString* b);

const char* page_data_get_message_or_empty_string(PageData* page);

gboolean page_data_is_message_empty(PageData* page);

void page_data_set_message(PageData* page, const char* message);

const char* page_data_get_overlay_or_empty_string(PageData* page);

gboolean page_data_is_overlay_empty(PageData* page);

void page_data_set_overlay(PageData* page, const char* overlay);

size_t page_data_get_number_of_lines(PageData* page);

LineData* page_data_get_line_by_index_or_else(PageData* page, unsigned int index, LineData* else_value);

void page_data_add_line(PageData* page,
                        const gchar* label,
                        const gchar* icon,
                        const gchar* data,
                        gboolean urgent,
                        gboolean highlight,
                        gboolean markup,
                        gboolean nonselectable);

void page_data_add_line_json_node(PageData* page, JsonNode* node);

void page_data_clear_lines(PageData* page);

#endif // ROFI_BLOCKS_PAGE_DATA_H
