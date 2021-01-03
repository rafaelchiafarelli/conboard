#!/bin/bash

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

clean() {

	echo "compiler done"
}

trap clean EXIT
trap "exit 1" INT TERM

set -e

build() {
	for SCRIPT in "${RECIPE[@]}"; do
	  echo "running \"$SCRIPT\""
	  $SCRIPT
	done
}

build_launcher(){
	$SCRIPTS_DIR/../launcher/scripts/build_launcher.sh

}
build_common(){
	$SCRIPTS_DIR/../Common/scripts/build_common.sh

}
build_MIDI(){
	$SCRIPTS_DIR/../MIDI/scripts/build_MIDI.sh
}

build_mouse(){
	$SCRIPTS_DIR/../Mouse/scripts/build_mouse.sh
}

build_KeyBoard(){
	$SCRIPTS_DIR/../KeyBoard/scripts/build_KeyBoard.sh
}

build_JoyStick(){
	$SCRIPTS_DIR/../Joystick/scripts/build_JoyStick.sh

}

compile(){
	RECIPE=(
			build_launcher
            build_common
            build_JoyStick
            build_KeyBoard
            build_MIDI
            build_mouse
			)   
	build
}

echo "Compiler"
if (( $# == 0 )); then
	compile 
	exit 0
fi
