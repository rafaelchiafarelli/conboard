#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

#set -e

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"


quick_install() {
	for SCRIPT in "${RECIPE[@]}"; do
	  echo "running \"$SCRIPT\""
	  $SCRIPT
	done

}



build_common(){
    cd $SCRIPTS_DIR/../LowLevel/Common/script
    sudo ./build_common.sh
    sudo cp $SCRIPTS_DIR/../LowLevel/Common/build/libcommon.sq /conboard/LowLevel/Common/build/
    
}


build_launcher(){
    cd $SCRIPTS_DIR/../LowLevel/launcher/script
    sudo ./build_launcher.sh
    sudo cp $SCRIPTS_DIR/../LowLevel/launcher/build/launcher /conboard/LowLevel/launcher/build/
}

build_joystick(){
    cd $SCRIPTS_DIR/../LowLevel/joystick/script
    sudo ./build_joystick.sh
    sudo cp $SCRIPTS_DIR/../LowLevel/joystick/build/conJoyS /conboard/LowLevel/joystick/build/
}

build_KeyBoard(){
    cd $SCRIPTS_DIR/../LowLevel/KeyBoard/script
    sudo ./build_KeyBoard.sh
    sudo cp $SCRIPTS_DIR/../LowLevel/KeyBoard/build/conKeyB /conboard/LowLevel/KeyBoard/build/
}

build_MIDI(){
    cd $SCRIPTS_DIR/../LowLevel/MIDI/script
    sudo ./build_MIDI.sh
    sudo cp $SCRIPTS_DIR/../LowLevel/MIDI/build/conMIDI /conboard/LowLevel/MIDI/build/
}

build_mouse(){
    cd $SCRIPTS_DIR/../LowLevel/Mouse/script
    sudo ./build_mouse.sh
    sudo cp $SCRIPTS_DIR/../LowLevel/Mouse/build/conMIDI /conboard/LowLevel/Mouse/build/
}

install(){

	RECIPE=(
			build_common
			build_joystick
            build_KeyBoard
            build_MIDI
            build_mouse
			)
	quick_install

}

if (( $# == 0 )); then

	install
	exit 0

fi
