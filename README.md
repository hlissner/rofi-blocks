> This was forked from the fantastic OmarCastro/rofi-blocks and is being used as
> the backbone for a Rofi UI framework I'm developing for [my
> dotfiles](https://github.com/hlissner/dotfiles). 
>
> Users that have used rofi-blocks before and are interested in the work here,
> be warned, it contains many backwards-incompatible changes:
>
> - This fork depends on [my fork of Rofi](https://github.com/hlissner/rofi).
> - Events:
>   - Changes the default event format to `{"event":"{{event}}",
>     "value":"{{value_escaped}}", "data":"{{data_escaped}}"}`. `{{event}}` was
>     formerly `{{name_enum}}`, which is now the only option; both `name` and
>     `name_escaped` have been removed.
>   - Adopts a new naming convention for input and event parameters for simpler
>     parsing, by replacing all spaces with underscores: E.g. `event format` =>
>     `event_format`.
>   - Renames two events to closer match the keybindings that invoke them:
>     - `SELECT_ENTRY` => `ACCEPT_ENTRY`
>     - `EXEC_CUSTOM_INPUT` => `ACCEPT_INPUT`
>   - Adds six new events: `INIT`, `CANCEL`, `EXIT`, `COMPLETE`,
>     `ACCEPT_ENTRY_ALT`, and `ACCEPT_INPUT_ALT`. (Details below).
>   - Changes the behavior of three pre-existing events:
>     - `ACTIVE_ENTRY` events are emitted whenever the user changes the selected
>       line, and is no longer tied specifically (or exclusively) to the
>       `CUSTOM_KEY` event.
>     - `CUSTOM_KEY` events can now be emitted when the list is empty.
>     - `INPUT_CHANGE` is now emitted unconditionally, when the input field is
>       changed.
> - Input parameters:
>   - Renames `input_format` to `event_format`.
>   - Renames `active_entry` to `active_line`.
>   - Removes `input_action` and replaces it with `filter` and `case_sensitive`
>     fields. (Details below)
>   - Adds a `icon` parameter. (Details below)
>   - Adds a `filter` parameter to lines. (Details below)

# rofi-blocks
This Rofi modi allows for live manipulation of Rofi's content through an
external process (or STDIN, in the shell), as an extension of Rofi's scripting
capabilities. This allows users to change the message, input, overlay, or prompt
text on-the-fly; modify the list of entries asynchronously; and respond to user
actions, specific keybinds, or react/affect Rofi as they type.

```bash
rofi -modi blocks -show blocks
     [ -blocks-wrap /path/to/program ]
     [ -blocks-prompt "Initial prompt text" ]
     [ -event-format '{"event":"{{event}}", "value":"{{value_escaped}}", "data":"{{data_escaped}}"}' ]
     [ -input-action send|filter ]
     [ -markup-rows ]
```

## Dependencies
- rofi 1.7.5-dev (with my patch)
- json-glib 1.0+


# Installation
**rofi-blocks** uses autotools as a build system. When installing from git, the
following steps should install it:
```bash
$ autoreconf -i
$ mkdir build
$ cd build/
$ ../configure
$ make
$ make install
```


# Examples
See the `examples/` folder for example use-cases for this modi.


# How it works
This module reads STDIN (or the STDOUT of a background process) line-by-line.
These are output payloads (i.e. from a user/process), and they may embed
commands to manipulate Rofi's content.

The module will also write events to STDOUT line-by-line. Each line is a full
input payload (i.e. from Rofi to the user/background process). **No payload
spans more than one line** -- newlines within are escaped.

All payloads are formatted as JSON, but this can be changed.

## Output format
An output payload contains only the Rofi state you want changed. For example:
```json
{
  "active_line": 0,
  "case_sensitive": false,
  "event_format": "{\"event\":\"{{event}}\", \"value\":\"{{value_escaped}}\", \"data\":\"{{data_escaped}}\"}",
  "filter": "",
  "icon": "",
  "input": "",
  "message": "",
  "overlay": "",
  "prompt": "",

  "lines":[
    "one line is a line from input", 
    {"text":"can be a string or object"}, 
    {"text":"with object, you can use flags", "urgent":true},
    {"text":"such as urgent and highlight", "highlight": true},
    {"text":"as well as markup", "markup": true},
    {"text":"or both", "urgent":true, "highlight": true, "markup":true},
    {"text":"you can put an icon too", "icon": "folder"},
    {"text":"this line will always be visible", "filter": false},
    {"text":"you can put metadata that it will echo on the event", "data": "entry_number:\"8 (7 starting from 0)\"\nthis_is:yaml\nbut_it_can_be:any format"},
  ]
}
```

### Output properties
| Property       | Description                                                                                                                                                                        |
|----------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| active_line    | Zero-based index of the screen line to select: <br> - a value equal or larger than the number of lines will focus the last entry. <br> - negative or floating numbers are ignored. |
| case_sensitive | If true, filtering is case sensitive                                                                                                                                               |
| close_on_exit  | If true, close rofi when the connected process exits                                                                                                                               |
| event_format   | Format used for events emitted to stdout; details in next section                                                                                                                  |
| filter         | The search query to filter lines against. Set to an empty string to filter nothing, or null to revert to default behavior                                                          |
| icon           | Changes the icon displayed in the element named "icon". Accepts a icon name or path to image. If null, resets to default icon                                                      |
| input          | Sets input text, to clear use empty string or null                                                                                                                                 |
| lines          | A list of strings or json objects representing rofi's listview content                                                                                                             |
| message        | Sets Rofi message, hides it if empty or null                                                                                                                                       |
| overlay        | Shows overlay with text, hides it if empty or null                                                                                                                                 |
| placeholder    | Sets the input text while it is empty                                                                                                                                              |
| prompt         | Sets prompt text. Note: due to a Rofi limitation, the prompt still consumes space if empty or null                                                                                 |
| trigger        | Trigger a rofi keybinding by name (e.g. `kb-mode-complete`)                                                                                                                        |

### Line properties
| Property      | Description                                                                  |
|---------------|------------------------------------------------------------------------------|
| text          | entry text                                                                   |
| urgent        | flag: defines entry as urgent                                                |
| highlight     | flag: highlight the entry                                                    |
| markup        | flag: enables/disables pango markup. If omitted, `-markup-rows` is respected |
| filter        | flag: if false, this line is never filtered (e.g. always visible)            |
| nonselectable | flag: if true, accepting this entry does nothing                             |
| icon          | the name or path to an icon in your active icon theme                        |
| data          | metadata associated with that line; can contain any arbitrary data           |

## Input format
rofi-blocks emits an input payload whenever an event is triggered. The format of
this payload is set according to the `event_format` property. The default format
is:
```json
{"event":"{{event}}", "value":"{{value_escaped}}", "data":"{{data_escaped}}"}
```

### Event format parameters
Each `{{parameter}}` is replaced as per the table below:
| Parameter     | Format              | Description                                                                                                                                                                                                                                               |
|---------------|---------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| event         | `{{event}}`         | name of the event                                                                                                                                                                                                                                         |
| value         | `{{value}}`         | information of the event: <br> - **entry text** on entry select or delete <br> - **input content** on input change, exec custom input, or complete <br> - **custom key number** on custom key typed                                                       |
| value escaped | `{{value_escaped}}` | information of the event, escaped to be inserted on a json string                                                                                                                                                                                         |
| data          | `{{data}}`          | additional data of the event: <br> - **entry metadata** on entry select or delete <br> - **"1"** on custom command or complete, indicating that an active entry event was emitted prior<br> - **empty sting** if entry has no metadata, or on other event |
| data_escaped  | `{{data_escaped}}`  | additional data of the event,  escaped to be inserted on a json string                                                                                                                                                                                    |

### Events
| Name             | Value                          | Data                           | Description                                                                                            |
|------------------|--------------------------------|--------------------------------|--------------------------------------------------------------------------------------------------------|
| INIT             | rofi_blocks_version (string)   | rofi_abi_version (string)      | emitted once, when rofi-blocks is ready                                                                |
| ACTIVE_ENTRY     | active entry text or "" if n/a | active entry data or "" if n/a | emitted each time the selected entry changes                                                           |
| ACCEPT_ENTRY     | active entry text              | active entry data              | when selecting an entry on the list (with the `kb-accept` keybind)                                     |
| ACCEPT_ENTRY_ALT | active entry text              | active entry data              | when selecting an entry (with the `kb-accept-alt` keybind)                                             |
| DELETE_ENTRY     | active entry text              | active entry data              | when deleting an entry (with the `kb-delete-entry` keybind)                                            |
| ACCEPT_INPUT     | input text                     | "1" if active entry, else ""   | when submitting custom input by typing `kb-accept-custom` (or `kb-accept`, when list is empty)         |
| ACCEPT_INPUT_ALT | input text                     | "1" if active entry, else ""   | when submitting custom input by typing `kb-accept-custom-alt` (or `kb-accept-alt`, when list is empty) |
| COMPLETE         | ""                             | ""                             | when `kb-mode-complete` is typed                                                                       |
| CUSTOM_KEY       | keycode (integer)              | "1" if active entry, else ""   | when a custom key is typed, follows an `ACTIVE_ENTRY` event if list isn't empty                        |
| INPUT_CHANGE     | input text                     | ""                             | when input changes and input action is set to `send`                                                   |
| CANCEL           | ""                             | ""                             | when Rofi is aborted by the user (typically with `kb-cancel`)                                          |
| EXIT             | ""                             | ""                             | as Rofi is closing the mode, whether or not the user initiated it                                      |

> Details on Rofi keybinds are available [in the Rofi manual](https://github.com/davatorium/rofi/blob/next/doc/rofi-keys.5.markdown).
