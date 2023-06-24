#!/bin/bash
 
ACTIONS="clear input"$'\n'\
"set input as \"lorem ipsum\""$'\n'\
"clear prompt"$'\n'\
"set prompt as \"lorem ipsum\""$'\n'\
"hide message (empty string)"$'\n'\
"hide message (null)"$'\n'\
"set message as \"hello there\""$'\n'\
"hide overlay (empty string)"$'\n'\
"hide overlay (null)"$'\n'\
"set overlay as \"overlay\""$'\n'\
"filter setter entries"$'\n'\
"do not filter by user input"$'\n'\
"restore default filter behavior"

toStringJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e '$!s/.*/&\\n/' | paste -sd "" -
}

toLinesJson(){
	echo "$1" | sed -e 's/\\/\\\\/g' -e 's/\"/\\"/g' -e 's/.*/"&"/' | paste -sd "," -
}
echo '{"event_format": "{{event}} {{value}}"}'
log_action(){
	JSON_LINES="$(toLinesJson "$ACTIONS")"
 	TEXT=$(cat <<EOF | tr -d "\n" | tr -d "\t"
{
	"message": "message",
	"lines":[${JSON_LINES}]
}
EOF
)
	printf '%s\n' "$TEXT"
}

log_action

while read -r line; do
	case "$line" in
		"ACCEPT_ENTRY clear input" )
			stdbuf -oL echo '{"input": ""}'
			;;
		"ACCEPT_ENTRY clear prompt" )
			stdbuf -oL echo '{"prompt": ""}'
			;;
		"ACCEPT_ENTRY hide message (empty string)" )
			stdbuf -oL echo '{"message": ""}'
			;;
		"ACCEPT_ENTRY hide message (null)" )
			stdbuf -oL echo '{"message": null}'
			;;
		"ACCEPT_ENTRY hide overlay (empty string)" )
			stdbuf -oL echo '{"overlay": ""}'
			;;
		"ACCEPT_ENTRY hide overlay (null)" )
			stdbuf -oL echo '{"overlay": null}'
			;;
		"ACCEPT_ENTRY set input as \"lorem ipsum\"" )
			stdbuf -oL echo '{"input": "lorem ipsum"}'
			;;
		"ACCEPT_ENTRY set prompt as \"lorem ipsum\"" )
			stdbuf -oL echo '{"prompt": "lorem ipsum"}'
			;;
		"ACCEPT_ENTRY set message as \"hello there\"" )
			stdbuf -oL echo '{"message": "hello there"}'
			;;
		"ACCEPT_ENTRY set overlay as \"overlay\"" )
			stdbuf -oL echo '{"overlay": "overlay"}'
			;;
		"ACCEPT_ENTRY filter setter entries" )
			stdbuf -oL echo '{"filter": "set"}'
			;;
		"ACCEPT_ENTRY do not filter by user input" )
			stdbuf -oL echo '{"filter": ""}'
			;;
		"ACCEPT_ENTRY restore default filter behavior" )
			stdbuf -oL echo '{"filter": null}'
			;;

	esac
done
