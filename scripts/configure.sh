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
	  "$SCRIPT"
	done
}

preconditions(){
    sudo apt update
    sudo apt install udev udevil gitk libusb-dev
}

install_gcc() {
    sudo apt install -y libc6-armel-cross libc6-dev-armel-cross binutils-arm-linux-gnueabihf libncurses5-dev build-essential bison flex libssl-dev bc
    sudo apt install -y gcc g++ cmake x11-common x11-utils libx11-dev

}
install_gcc_x86() {
    sudo apt install -y g++-aarch64-linux-gnu 
    sudo apt install -y libc6-armel-cross libc6-dev-armel-cross binutils-arm-linux-gnueabi libncurses5-dev build-essential bison flex libssl-dev bc
    sudo apt install -y gcc-arm-linux-gnueabi g++-arm-linux-gnueabi
}

install_alsa(){
    
    sudo apt install -y libasound2 libasound2-dev --fix-missing

}

install_python3(){
    wget https://www.python.org/ftp/python/3.8.0/Python-3.8.0.tgz
    sudo apt-get update
    sudo apt-get install -y build-essential tk-dev libncurses5-dev libncursesw5-dev libreadline6-dev libdb5.3-dev libgdbm-dev libsqlite3-dev libssl-dev libbz2-dev libexpat1-dev liblzma-dev zlib1g-dev libffi-dev tar wget vim
    sudo apt-get install -y python-setuptools
    sudo tar zxf Python-3.8.0.tgz
    cd Python-3.8.0
    sudo ./configure --enable-optimizations
    sudo make
    sudo make altinstall
    echo "alias python=/usr/local/bin/python3.8" >> ~/.bashrc
    source ~/.bashrc
    sudo apt install python3-venv python3-pip
    sudo python3.8 -m easy_install pip
    sudo pip3.8 install --upgrade pip
    sudo pip3.8 install update pip
}

compile_all(){

    cd $SCRIPT
    compiler.sh

}

compile(){
	RECIPE=(
            preconditions
            install_gcc
            install_alsa
            install_python3
            compile_all
			)   
	build
}

echo "Compiler"
if (( $# == 0 )); then
	compile 
	exit 0
fi
