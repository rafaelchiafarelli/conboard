#!/bin/bash

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

clean() {

	echo "generic compiling done"
}

trap clean EXIT
trap "exit 1" INT TERM

set -e

ExecRecipe() {
	for SCRIPT in "${RECIPE[@]}"; do
	  echo "Executing \"$SCRIPT\""
	  $SCRIPT
	done
}

make_common(){
    cd $SCRIPTS_DIR/../
    mkdir -p build
    cd build
	cmake ..
	make -j$(nproc)	
}


compile(){
	RECIPE=(
			make_common

			)
	ExecRecipe
}
if [ "$#" -ne 1 ]; then
	compile
	exit 0
fi


