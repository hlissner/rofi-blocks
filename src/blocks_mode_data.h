// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro
#ifndef ROFI_BLOCKS_MODE_DATA_H
#define ROFI_BLOCKS_MODE_DATA_H
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <gmodule.h>
#include <time.h>

#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <rofi/rofi-types.h>
#include <rofi/helper.h>

#include "string_utils.h"
#include "page_data.h"
#include "json_glib_extensions.h"

typedef struct {
    PageData* currentPageData;
    GString* event_format;
    gint64 entry_to_focus;
    rofi_int_matcher **tokens;

    JsonParser* parser;
    JsonObject* root;
    GError* error;
    GString* active_line;
    GString* buffer;
    
    GPid cmd_pid;
    gboolean close_on_child_exit;
    GIOChannel* write_channel;
    GIOChannel* read_channel;
    int write_channel_fd;
    int read_channel_fd;
    guint read_channel_watcher;
} BlocksModePrivateData;

BlocksModePrivateData* blocks_mode_private_data_new();

void blocks_mode_private_data_update_destroy(BlocksModePrivateData* data);

void blocks_mode_private_data_update_page(BlocksModePrivateData* data);

#endif // ROFI_BLOCKS_MODE_DATA_H


