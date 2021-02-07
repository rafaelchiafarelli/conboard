#!/bin/bash

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

clean() {

	echo "compiler done"
}

trap clean EXIT
trap "exit 1" INT TERM

set -e

clean() {
	for SCRIPT in "${RECIPE[@]}"; do
	  echo "running \"$SCRIPT\""
	  $SCRIPT
	done
}

clean_launcher(){
	sudo rm -rf $SCRIPTS_DIR/../launcher/build

}
clean_common(){
	sudo rm -rf $SCRIPTS_DIR/../Common/build

}
clean_MIDI(){
	sudo rm -rf $SCRIPTS_DIR/../MIDI/build
}

clean_mouse(){
	sudo rm -rf $SCRIPTS_DIR/../Mouse/build
}

clean_KeyBoard(){
	sudo rm -rf $SCRIPTS_DIR/../KeyBoard/build
}

clean_JoyStick(){
	sudo rm -rf $SCRIPTS_DIR/../Joystick/build

}

compile(){
	RECIPE=(
            clean_common
            clean_JoyStick
            clean_KeyBoard
            clean_MIDI
            clean_mouse
			clean_launcher
			)   
	clean
}

echo "Compiler"
if (( $# == 0 )); then
	compile 
	exit 0
fi
