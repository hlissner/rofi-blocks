# rofi-blocks

Rofi modi that allows controlling rofi content through communication with an external program

To run this you need an up to date checkout of rofi git installed.

Run rofi like:

```bash
rofi -modi blocks -show blocks
```
or
```bash
rofi -modi blocks -show blocks -blocks-wrap /path/to/program
```

### Dependencies

| Dependency | Version |
|------------|---------|
| rofi 	     | 1.4     |
| json-glib  | 1.0     |


### Installation

**Rofi-blocks** uses autotools as build system. When installing from git, the following steps should install it:

```bash
$ autoreconf -i
$ mkdir build
$ cd build/
$ ../configure
$ make
$ make install
```


### Background

This module was created to extend rofi scripting capabilities, such as:

 - Changing the message and list content on the fly

 - Allow update of content on intervals, without the need of interaction (e.g. top)

 - Allow changing the input prompt on the fly, or be defined on the script instead of rofi command line

 - Allow custom overlay message
 

### Documentation

The module works by reading the stdin line by line to update rofi content. It can execute an application in the background, which will be used to communicate with the modi. The communication is made using file descriptors. The modi reads the output of the backgroung program and writes events its input.

#### Communication

This modi updates rofi each time it reads a line from the stdin or background program output, and writes an event made from rofi, in most cases is an interaction with the window, as a single line to stdout or its input. The format used for communication is JSON format.

#### Why JSON:

For the following reasons:
1. All information can be stored in one line
2. It is simple to write

##### All information can be stored in one line

 New lines on message and text can be escaped with `\n` , this helps the modi to know when to stop reading and starts parsing the message, preventing cases of the program flushing incomplete lines, and json parser failing due to incomplete messages.

##### It is simple to write
There is no need to have a library or framework to write json, there are, however, few consideration when transforming text to json string, such as escaping backlashes, newlines and double quotes, something like that can be done with a string replacer, like the `replace()` method in python or `sed` command in bash


#### Output JSON format

The JSON format used to communicate with the modi is the one on the next figure, all fields are optional:

```json
{
  "message": "rofi message text",
  "overlay": "overlay text",
  "prompt": "prompt text",
  "input": "input text",
  "input action": "input action, 'send' or 'filter'",
  "event format": "event format",
  "active entry": 3,
  "lines":[
    "one line is a line from input", 
    {"text":"can be a string or object"}, 
    {"text":"with object, you can use flags", "urgent":true},
    {"text":"such as urgent and highlight", "highlight": true},
    {"text":"as well as markup", "markup": true},
    {"text":"or both", "urgent":true, "highlight": true, "markup":true},
    {"text":"you can put an icon too", "icon": "folder"},
    {"text":"you can put metadata that it will echo on the event", "data": "entry_number:\"8 (7 starting from 0)\"\nthis_is:yaml\nbut_it_can_be:any format"},
  ]
}
```

##### Property table

| Property     | Description                                          |
|--------------|------------------------------------------------------|
| message 	   | Sets rofi message, hides it if empty or null         |
| overlay      | Shows overlay with text, hides it if empty or null   |
| prompt       | sets prompt text                                     |
| input        | sets input text, to clear use empty string           |
| input action | Sets input change action only two values are accepted, any other is ignored. <br> **filter***(default)*: rofi filters the content, no event is sent to program input <br> **send**: prevents rofi filter and sends an "*input change*" event to program input |
| event format | event format used to send to input, more of it on next section |
| active entry | entry to be focused/active: <br> - the first entry number is 0; <br> - a value equal or larger than the number of lines will focus the first one; <br> - negative or floating numbers are ignored.  |
| lines        | a list of sting or json object to set rofi list content, a string will show a text with all flags disabled.  |


##### Line Property table

| Property      | Description                                                                                         |
|---------------|-----------------------------------------------------------------------------------------------------|
| text          | entry text                                                                                          |
| urgent        | flag: defines entry as urgent                                                                       |
| highlight     | flag: highlight the entry                                                                           |
| markup        | flag: enables/disables highlight. if not defined, rofi config is used (enabled with `-markup-rows`) |
| nonselectable | flag: if true, selecting the entry does nothing                                                     |
| icon          | entry icon                                                                                          |
| data          | entry metadata                                                                                      |

#### Input format

The default format that the plugin uses to write events to the program is a json object. However, there are scripting languages that does not have a builtin json parser, *shell* is one of them. For that reason, the event format is configurable by setting the "*event format*" property on output. 

The default format has the following text: 
```json
{"name":"{{name_escaped}}", "value":"{{value_escaped}}", "data":"{{data_escaped}}"}
```

It contains some text wrapped with double braces {{}}. These wrapped text indicates a parameter that will be replaced by the plugin formatter. The following parameters are supported:

| Parameter       | Format              | Description                                                               |
|-----------------|---------------------|---------------------------------------------------------------------------|
| name            | `{{name}}`          | name of the event                                                         |
| name escaped    | `{{name_escaped}}`  | name of the event, escaped to be inserted on a json string                |
| name as enum    | `{{name_enum}}`     | name of the event in all caps, separated by underscore for easier parsing |
| value           | `{{value}}`         | information of the event: <br> - **entry text** on entry select or delete <br> - **input content** on input change, exec custom input, or complete <br> - **custom key number** on custom key typed |
| value escaped   | `{{value_escaped}}` | information of the event, escaped to be inserted on a json string         |
| data            | `{{data}}`          | additional data of the event: <br> - **entry metadata** on entry select or delete <br> - **"1"** on custom command or complete, indicating that an active entry event was emitted prior<br> - **empty sting** if entry has no metadata, or on other event  |
| data_escaped    | `{{data_escaped}}`  | additional data of the event,  escaped to be inserted on a json string    |

##### Event names:

| Name                     | Name as enumerator    | Description                                                                                            |
|--------------------------|-----------------------|--------------------------------------------------------------------------------------------------------|
| input change             | INPUT_CHANGE          | when input changes and input action is set to `send`                                                   |
| custom key               | CUSTOM_KEY            | when a custom key is typed, follows an `active entry` event if list isn't empty                        |
| complete                 | COMPLETE              | when `kb-mode-complete` is typed                                                                       |
| cancel                   | CANCEL                | when Rofi is aborted (typically with `kb-cancel`)                                                      |
| active entry             | ACTIVE_ENTRY          | announces the currently selected entry in list, prior to a `custom key` or `complete` event            |
| select entry             | SELECT_ENTRY          | when selecting an entry on the list (with the `kb-accept` keybind)                                     |
| select entry alt         | SELECT_ENTRY_ALT      | when selecting an entry (with the `kb-accept-alt` keybind)                                             |
| delete entry             | DELETE_ENTRY          | when deleting an entry (with the `kb-delete-entry` keybind)                                            |
| execute custom input     | EXEC_CUSTOM_INPUT     | when submitting custom input by typing `kb-accept-custom` (or `kb-accept`, when list is empty)         |
| execute custom input alt | EXEC_CUSTOM_INPUT_ALT | when submitting custom input by typing `kb-accept-custom-alt` (or `kb-accept-alt`, when list is empty) |

> Details on Rofi keybinds are available [in the Rofi manual](https://github.com/davatorium/rofi/blob/next/doc/rofi-keys.5.markdown).


### Examples

Additional documentation is created in the form of functional examples, you can compile and install the modi and execute examples in examples folder to understand and play with the modi.
