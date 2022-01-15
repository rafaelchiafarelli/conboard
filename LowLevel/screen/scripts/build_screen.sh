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

cmake_run(){
    cd $SCRIPTS_DIR/..
    mkdir -p build
    cd build

	cmake ..
}

make_all(){
	make -j$(nproc)
	cd $SCRIPTS_DIR
}


compile(){
	RECIPE=(
			cmake_run 
			make_all
			)
	ExecRecipe
}
if [ "$#" -ne 2 ]; then
	compile
	exit 0
fi


