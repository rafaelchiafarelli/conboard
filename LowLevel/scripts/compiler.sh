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
	  "${SCRIPTS_DIR}/$SCRIPT"
	done
}


compile(){
	RECIPE=(
			build_launcher.sh
            build_common.sh
            build_JoyStick.sh
            build_KeyBoard.sh
            build_MIDI.sh
            build_mouse.sh
			)   
	build
}

echo "Compiler"
if (( $# == 0 )); then
	compile 
	exit 0
fi
