// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro

#define G_LOG_DOMAIN "BlocksMode"

#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include <rofi/mode.h>
#include <rofi/helper.h>
#include <rofi/mode-private.h>
#include <rofi/rofi-icon-fetcher.h>

#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <pango/pango.h>

#include <stdint.h>

#include "string_utils.h"
#include "page_data.h"
#include "json_glib_extensions.h"
#include "blocks_mode_data.h"


typedef struct RofiViewState RofiViewState;
void rofi_view_switch_mode(RofiViewState* state, Mode* mode);
RofiViewState* rofi_view_get_active(void);
extern uint32_t rofi_view_set_icon(RofiViewState *state, const char *icon, gboolean preload);
extern void rofi_view_set_overlay(RofiViewState* state, const char* text);
extern void rofi_view_set_placeholder(RofiViewState* state, const char* text);
extern void rofi_view_set_case_sensitive(RofiViewState* state, unsigned int case_sensitive);
extern void rofi_view_reload(void);
const char* rofi_view_get_user_input(const RofiViewState* state);
unsigned int rofi_view_get_selected_line(const RofiViewState* state);
void rofi_view_set_selected_line(const RofiViewState* state, unsigned int selected_line);
unsigned int rofi_view_get_next_position(const RofiViewState* state);
void rofi_view_handle_text(RofiViewState* state, char* text);
void rofi_view_clear_input(RofiViewState* state);
G_MODULE_EXPORT Mode mode;


const gchar* CmdArg__BLOCKS_WRAP = "-blocks-wrap";
const gchar* CmdArg__BLOCKS_PROMPT = "-blocks-prompt";
const gchar* CmdArg__MARKUP_ROWS = "-markup-rows";
const gchar* CmdArg__EVENT_FORMAT = "-event-format";

static const gchar* EMPTY_STRING = "";

typedef enum {
    Event__INIT,
    Event__INPUT_CHANGE,
    Event__ACTIVE_ENTRY,
    Event__ACCEPT_ENTRY,
    Event__ACCEPT_ENTRY_ALT,
    Event__ACCEPT_INPUT,
    Event__ACCEPT_INPUT_ALT,
    Event__COMPLETE,
    Event__DELETE_ENTRY,
    Event__CUSTOM_KEY,
    Event__CANCEL,
    Event__EXIT
} Event;

static const char* event_enum_labels[] = {
    "INIT",
    "INPUT_CHANGE",
    "ACTIVE_ENTRY",
    "ACCEPT_ENTRY",
    "ACCEPT_ENTRY_ALT",
    "ACCEPT_INPUT",
    "ACCEPT_INPUT_ALT",
    "COMPLETE",
    "DELETE_ENTRY",
    "CUSTOM_KEY",
    "CANCEL",
    "EXIT"
};


/**************
 rofi extension
****************/

unsigned int blocks_mode_rofi_view_get_current_position(RofiViewState* state, PageData* page){
    unsigned int next_position = rofi_view_get_next_position(state);
    unsigned int length = page_data_get_number_of_lines(page);
    if (next_position <= 0 || next_position >= UINT32_MAX - 10) {
        return length - 1;
    } else {
        return next_position - 1;
    }
}


/**************************************
  extended mode pirvate data methods
**************************************/

void blocks_mode_private_data_write_to_channel(BlocksModePrivateData* data, Event event, const char* action_value, const char* action_data) {
    GIOChannel* write_channel = data->write_channel;
    if (data->write_channel == NULL) {
        // when script exits or errors while loading
        return;
    }
    const gchar* format = data->event_format->str;
    gchar* format_result = str_replace(format, "{{event}}", event_enum_labels[event]);
    format_result = str_replace_in(&format_result, "{{value}}", action_value);
    format_result = str_replace_in(&format_result, "{{data}}", action_data);
    format_result = str_replace_in_escaped(&format_result, "{{value_escaped}}", action_value);
    format_result = str_replace_in_escaped(&format_result, "{{data_escaped}}", action_data);
    g_debug("sending event: %s", format_result);
    gsize bytes_witten;
    g_io_channel_write_chars(write_channel, format_result, -1, &bytes_witten, &data->error);
    g_io_channel_write_unichar(write_channel, '\n', &data->error);
    g_io_channel_flush(write_channel, &data->error);
    g_free(format_result);
}


/**************************
  mode extension methods
**************************/

static BlocksModePrivateData* mode_get_private_data_extended_mode(const Mode* sw) {
    return (BlocksModePrivateData*) mode_get_private_data(sw);
}

static PageData* mode_get_private_data_current_page(const Mode* sw) {
    return mode_get_private_data_extended_mode(sw)->page;
}


/*******************
 main loop sources
********************/

static void on_icon_retry(gpointer context) {
    PageData *page = (PageData *) context;
    RofiViewState* state = rofi_view_get_active();
    rofi_view_set_icon(state, page->icon != NULL ? page->icon->str : NULL, FALSE);
}

static gboolean next_line(BlocksModePrivateData* data, GIOChannel* source, GIOCondition condition, gpointer context) {
    GString* buffer = data->buffer;
    GString* active_line = data->active_line;
    GError* error = NULL;
    gunichar unichar;
    GIOStatus status = g_io_channel_read_unichar(source, &unichar, &error);

    // when there is nothing to read, status is G_IO_STATUS_AGAIN
    while(status == G_IO_STATUS_NORMAL) {
        g_string_append_unichar(buffer, unichar);
        if (unichar == '\n') {
            if (buffer->len > 1) { //input is not an empty line
                g_debug("received new line: %s", buffer->str);
                g_string_assign(active_line, buffer->str);
            }
            g_string_set_size(buffer, 0);
            return TRUE;
        }
        status = g_io_channel_read_unichar(source, &unichar, &error);
    }
    return FALSE;
}

// GIOChannel watch, called when there is output to read from child proccess
static gboolean on_new_input(GIOChannel* source, GIOCondition condition, gpointer context) {
    Mode* sw = (Mode*) context;
    BlocksModePrivateData* data = mode_get_private_data_extended_mode(sw);

    while (next_line(data, source, condition, context)) {
        g_debug("handling received line");

        GString* overlay = data->page->overlay;
        GString* prompt = data->page->prompt;
        GString* placeholder = data->page->placeholder;
        GString* icon = data->page->icon;
        GString* input = data->page->input;
        GString* filter = data->page->filter;

        GString* old_overlay = overlay ? g_string_new(overlay->str) : NULL;
        GString* old_prompt = prompt ? g_string_new(prompt->str) : NULL;
        GString* old_placeholder = placeholder ? g_string_new(placeholder->str) : NULL;
        GString* old_icon = icon ? g_string_new(icon->str) : NULL;
        GString* old_input = input ? g_string_new(input->str) : NULL;
        GString* old_filter = filter ? g_string_new(filter->str) : NULL;
        gboolean old_case_sensitive = data->page->case_sensitive;

        blocks_mode_private_data_update_page(data);

        GString* new_overlay = data->page->overlay;
        GString* new_prompt = data->page->prompt;
        GString* new_placeholder = data->page->placeholder;
        GString* new_icon = data->page->icon;
        GString* new_input = data->page->input;
        GString* new_filter = data->page->filter;
        gboolean new_case_sensitive = data->page->case_sensitive;

        RofiViewState* state = rofi_view_get_active();

        if (!page_data_is_string_equal(old_icon, new_icon)) {
            if (rofi_view_set_icon(state, new_icon ? new_icon->str : NULL, FALSE) != 0) {
                // Icons are fetched asynchronously and may not be immediately
                // available. rofi_view_set_icon returns non-zero if this is the
                // case (or the icon wasn't found), so try again shortly after:
                g_idle_add(G_SOURCE_FUNC(on_icon_retry), (void*) data->page);
            }
        }

        if ((old_case_sensitive != new_case_sensitive)
            || (!page_data_is_string_equal(old_filter, new_filter))) {
            if (data->tokens) {
                helper_tokenize_free(data->tokens);
            }
            data->tokens = new_filter == NULL
                ? NULL
                : helper_tokenize(new_filter->str, new_case_sensitive);
            rofi_view_set_case_sensitive(state, new_case_sensitive);
        }

        if (!page_data_is_string_equal(old_overlay, new_overlay)) {
            rofi_view_set_overlay(state, (new_overlay->len > 0) ? new_overlay->str : NULL);
        }

        if (!page_data_is_string_equal(old_placeholder, new_placeholder)) {
            rofi_view_set_placeholder(state, (new_placeholder->len > 0) ? new_placeholder->str : NULL);
        }

        if (!page_data_is_string_equal(old_input, new_input)) {
            rofi_view_clear_input(state);
            rofi_view_handle_text(state, new_input->str);
        }

        if (!page_data_is_string_equal(old_prompt, new_prompt)) {
            g_free(sw->display_name);
            sw->display_name = g_strdup(new_prompt->str);
            // rofi_view_reload does not update prompt, that is why this is needed
            rofi_view_switch_mode(state, sw);
        }

        if (data->entry_to_focus >= 0) {
            g_debug("entry_to_focus %li", data->entry_to_focus);
            rofi_view_set_selected_line(state, (unsigned int) data->entry_to_focus);
        }

        old_overlay && g_string_free(old_overlay, TRUE);
        old_prompt && g_string_free(old_prompt, TRUE);
        old_placeholder && g_string_free(old_placeholder, TRUE);
        old_icon && g_string_free(old_icon, TRUE);
        old_input && g_string_free(old_input, TRUE);
        old_filter && g_string_free(old_filter, TRUE);

        g_debug("reloading rofi view");
        rofi_view_reload();
    }

    return G_SOURCE_CONTINUE;
}

// spawn watch, called when child exited
static void on_child_status(GPid pid, gint status, gpointer context) {
    g_message("Child %" G_PID_FORMAT " exited %s", pid,
              g_spawn_check_wait_status (status, NULL) ? "normally" : "abnormally");
    Mode* sw = (Mode*) context;
    BlocksModePrivateData *data = mode_get_private_data_extended_mode(sw);
    g_spawn_close_pid(pid);
    data->write_channel = NULL;
    if (data->close_on_child_exit) {
        exit(0);
    }
}


/************************
 extended mode methods
***********************/

static int blocks_mode_init(Mode* sw) {
    if (mode_get_private_data(sw)) { return TRUE; }

    BlocksModePrivateData* pd = blocks_mode_private_data_new();
    mode_set_private_data(sw, (void*) pd);

    char* format = NULL;
    if (find_arg_str(CmdArg__EVENT_FORMAT, &format)) {
        pd->event_format = g_string_new(format);
    }

    if (find_arg(CmdArg__MARKUP_ROWS)) {
        pd->page->markup_default = MarkupStatus_ENABLED;
    }

    char* prompt = NULL;
    if (find_arg_str(CmdArg__BLOCKS_PROMPT, &prompt)) {
        sw->display_name = g_strdup(prompt);
    }

    char* cmd = NULL;
    if (find_arg_str(CmdArg__BLOCKS_WRAP, &cmd)) {
        GError* error = NULL;
        int cmd_input_fd;
        int cmd_output_fd;
        char** argv = NULL;
        if (!g_shell_parse_argv(cmd, NULL, &argv, &error)) {
            fprintf(stderr, "Unable to parse cmdline options: %s\n", error->message);
            g_error_free(error);
            return 0;
        }

        if (!g_spawn_async_with_pipes(
                NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
                NULL, NULL, &(pd->cmd_pid), &(cmd_input_fd), &(cmd_output_fd), NULL,
                &error)) {
            fprintf(stderr, "Unable to exec %s\n", error->message);
            char buffer[1024];
            int result = 4;
            char* cmd_escaped = str_new_escaped_for_json_string(cmd);
            char* error_message_escaped = str_new_escaped_for_json_string(error->message);
            snprintf(buffer, sizeof(buffer),
                     "{\"close_on_exit\": false, \"message\":\"Error loading %s:%s\"}\n",
                     cmd_escaped,
                     error_message_escaped);
            fprintf(stderr, "message:  %s\n", buffer);

            g_string_assign(pd->active_line, buffer);
            blocks_mode_private_data_update_page(pd);
            g_error_free(error);
            g_free(cmd_escaped);
            g_free(error_message_escaped);
            return TRUE;
        }
        g_strfreev(argv);

        pd->read_channel_fd = cmd_output_fd;
        pd->write_channel_fd = cmd_input_fd;

        int retval = fcntl(pd->read_channel_fd, F_SETFL, fcntl(pd->read_channel_fd, F_GETFL) | O_NONBLOCK);
        if (retval != 0) {
            fprintf(stderr,"Error setting non block on output pipe\n");
            kill(pd->cmd_pid, SIGTERM);
            exit(1);
        }
        pd->read_channel = g_io_channel_unix_new(pd->read_channel_fd);
        pd->write_channel = g_io_channel_unix_new(pd->write_channel_fd);
        g_child_watch_add(pd->cmd_pid, on_child_status, sw);
    } else {
        int retval = fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
        if (retval != 0) {
            fprintf(stderr,"Error setting non block on output pipe\n");
            exit(1);
        }
        pd->read_channel = g_io_channel_unix_new(STDIN_FILENO);
        pd->write_channel = g_io_channel_unix_new(STDOUT_FILENO);
    }

    pd->read_channel_watcher = g_io_add_watch(pd->read_channel, G_IO_IN, on_new_input, sw);

    char abi[8];
    snprintf(abi, 8, "%d", ABI_VERSION);
    // TODO: Read version dynamically
    blocks_mode_private_data_write_to_channel(pd, Event__INIT, "0.1.0", abi);
    return TRUE;
}

static unsigned int blocks_mode_get_num_entries(const Mode* sw) {
    g_debug("%s", "blocks_mode_get_num_entries");
    PageData* page = mode_get_private_data_current_page(sw);
    return page_data_get_number_of_lines(page);
}

static ModeMode blocks_mode_result(Mode* sw, int mretv, char** input, unsigned int selected_line) {
    BlocksModePrivateData* data = mode_get_private_data_extended_mode(sw);

    if (mretv & MENU_NEXT) {
        return NEXT_DIALOG;
    } else if (mretv & MENU_PREVIOUS) {
        return PREVIOUS_DIALOG;
    }

    PageData* page = data->page;
    LineData* line;
    if (selected_line >= 0 && selected_line < page->lines->len) {
        line = &g_array_index(page->lines, LineData, selected_line);
    } else {
        selected_line = -1;
    }

    ModeMode retv = RELOAD_DIALOG;
    if (mretv & MENU_CUSTOM_COMMAND) {
        char keycode[8];
        snprintf(keycode, 8, "%d", (mretv & MENU_LOWER_MASK)%20 + 1);
        blocks_mode_private_data_write_to_channel(data, Event__CUSTOM_KEY, keycode, selected_line == -1 ? "" : "1");
    } else if (mretv & MENU_COMPLETE) {
        blocks_mode_private_data_write_to_channel(data, Event__COMPLETE, "", "");
    } else if (mretv & MENU_OK) {
        if (line->nonselectable) { return RELOAD_DIALOG; }
        blocks_mode_private_data_write_to_channel(
            data, (mretv & MENU_CUSTOM_ACTION) ? Event__ACCEPT_ENTRY_ALT : Event__ACCEPT_ENTRY,
            line->text, line->data);
    } else if (mretv & MENU_ENTRY_DELETE) {
        if (line->nonselectable) { return RELOAD_DIALOG; }
        blocks_mode_private_data_write_to_channel(data, Event__DELETE_ENTRY, line->text, line->data);
    } else if (mretv & MENU_CUSTOM_INPUT) {
        blocks_mode_private_data_write_to_channel(
            data, (mretv & MENU_CUSTOM_ACTION) ? Event__ACCEPT_INPUT_ALT : Event__ACCEPT_INPUT,
            *input, selected_line == -1 ? "" : "1");
    } else if (mretv & MENU_CANCEL) {
        blocks_mode_private_data_write_to_channel(data, Event__CANCEL, "", "");
        retv = MODE_EXIT;
    } else {
        retv = MODE_EXIT;
    }
    return retv;
}

static void blocks_mode_destroy(Mode* sw) {
    BlocksModePrivateData* data = mode_get_private_data_extended_mode(sw);
    if (data != NULL) {
        blocks_mode_private_data_write_to_channel(data, Event__EXIT, "", "");
        blocks_mode_private_data_update_destroy(data);
        mode_set_private_data(sw, NULL);
    }
}

 static cairo_surface_t* blocks_mode_get_icon(const Mode* sw, unsigned int selected_line, unsigned int height) {
    PageData* page = mode_get_private_data_current_page(sw);
    LineData* line = page_data_get_line_by_index_or_else(page, selected_line, NULL);
    if (line == NULL) {
        return NULL;
    }

    const gchar* icon = line->icon;
    if (icon == NULL || icon[0] == '\0') {
        return NULL;
    }

    if (line->icon_fetch_uid <= 0) {
        line->icon_fetch_uid = rofi_icon_fetcher_query(icon, height);
    } 
    return rofi_icon_fetcher_get(line->icon_fetch_uid);
 }



static char* blocks_mode_get_display_value(const Mode* sw, unsigned int selected_line, int* state, G_GNUC_UNUSED GList** attr_list, int get_entry) {
    BlocksModePrivateData* data = mode_get_private_data_extended_mode(sw);
    PageData* page = mode_get_private_data_current_page(sw);
    LineData* line = page_data_get_line_by_index_or_else(page, selected_line, NULL);
    if (line == NULL) {
        *state |= 16;
        return get_entry ? g_strdup("") : NULL;
    }

    *state |= 
        1 * line->urgent +
        2 * line->highlight +
        8 * line->markup;
    return get_entry ? g_strdup(line->text) : NULL;
}

static int blocks_mode_token_match(const Mode* sw, rofi_int_matcher** tokens, unsigned int selected_line) {
    BlocksModePrivateData* data = mode_get_private_data_extended_mode(sw);
    PageData* page = data->page;
    LineData* line = page_data_get_line_by_index_or_else(page, selected_line, NULL);
    if (line == NULL) { return FALSE; }
    if (line->filter == FALSE) { return TRUE; }
    if (data->tokens == NULL) {
        if (page->filter != NULL && page->filter->str[0] == '\0') {
            return TRUE;
        }
    } else {
        tokens = data->tokens;
    }
    gchar* text = line->text;
    if (line->metatext != NULL) {
        text = line->metatext;
    } else if (!line->markup) {
        return helper_token_match(tokens, text);
    }
    // Strip out markup when matching
    char* esc = NULL;
    pango_parse_markup(text, -1, 0, NULL, &esc, NULL, NULL);
    return helper_token_match(tokens, esc);
}

static char* blocks_mode_get_message(const Mode* sw) {
    g_debug("%s", "blocks_mode_get_message");
    PageData* page = mode_get_private_data_current_page(sw);
    gchar* result = page_data_is_message_empty(page) ? NULL : g_strdup(page_data_get_message_or_empty_string(page));
    return result;
}

static char* blocks_mode_preprocess_input(Mode* sw, const char* new_input) {
    g_debug("%s", "blocks_mode_preprocess_input");
    BlocksModePrivateData* data = mode_get_private_data_extended_mode(sw);
    PageData* page = data->page;
    GString* input = page->input;
    if (g_strcmp0(input->str, new_input) != 0) {
        g_string_assign(input, new_input);
        blocks_mode_private_data_write_to_channel(data, Event__INPUT_CHANGE, new_input, "");
    }
    return g_strdup((page->filter == NULL ? input : page->filter)->str);
}

static void blocks_mode_selection_changed(Mode* sw, unsigned int index, unsigned int relative_index) {
    BlocksModePrivateData* data = mode_get_private_data_extended_mode(sw);
    if (index == UINT_MAX) {
        blocks_mode_private_data_write_to_channel(data, Event__ACTIVE_ENTRY, "", "");
    } else {
        PageData* page = mode_get_private_data_current_page(sw);
        LineData* line = page_data_get_line_by_index_or_else(page, index, NULL);
        blocks_mode_private_data_write_to_channel(data, Event__ACTIVE_ENTRY, line->text, line->data);
    }
}

Mode mode = {
    .abi_version        = ABI_VERSION,
    .name               = "blocks",
    .cfg_name_key       = "display-blocks",
    ._init              = blocks_mode_init,
    ._get_num_entries   = blocks_mode_get_num_entries,
    ._result            = blocks_mode_result,
    ._destroy           = blocks_mode_destroy,
    ._token_match       = blocks_mode_token_match,
    ._get_icon          = blocks_mode_get_icon,
    ._get_display_value = blocks_mode_get_display_value,
    ._get_message       = blocks_mode_get_message,
    ._get_completion    = NULL,
    ._selection_changed = blocks_mode_selection_changed,
    ._preprocess_input  = blocks_mode_preprocess_input,
    .private_data       = NULL,
    .free               = NULL,
    .type               = MODE_TYPE_SWITCHER
};
