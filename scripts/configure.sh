#!/bin/bash

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
ASSETS="$SCRIPTS_DIR/assets"
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
    sudo dd if=/dev/zero of=/swapfile2 bs=1M count=1024 status=progress
    sudo mkswap /swapfile2
    sudo swapon /swapfile2
    sudo apt -y update
    sudo apt -y install udev udevil gitk libusb-dev python-dev 
    sudo apt -y install nginx
}
install_node(){
    echo "installing node and reactjs"
}

install_db(){
    sudo apt -y install postgresql
    sudo apt -y install postgresql-server-dev-all postgresql-all
    sudo pip3.8 install psycopg2

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

install_zeromq(){
    sudo apt -y install libtool pkg-config build-essential autoconf automake
    sudo apt -y install git libssl-dev libzmq3-dev
    cd $SCRIPTS_DIR
    git clone https://github.com/zeromq/cppzmq.git
    cp $ASSETS/zmq.hpp ./cppzmq/
    cd cppzmq
    mkdir build
    cd build
    cmake -DCPPZMQ_BUILD_TESTS=False ..
    sudo make -j4 install
    sudo cp ${SCRIPTS_DIR}/cppzmq/zmq.hpp /usr/include/
    sudo ldconfig

}
install_python3(){
    wget https://www.python.org/ftp/python/3.8.0/Python-3.8.0.tgz
    sudo apt -y update
    sudo apt -y install -y build-essential tk-dev libncurses5-dev libncursesw5-dev libreadline6-dev libdb5.3-dev libgdbm-dev libsqlite3-dev libssl-dev libbz2-dev libexpat1-dev liblzma-dev zlib1g-dev libffi-dev tar wget vim
    sudo apt -  install -y python-setuptools
    sudo tar zxf Python-3.8.0.tgz
    cd Python-3.8.0
    sudo ./configure --enable-optimizations
    sudo make
    sudo make altinstall
    echo "alias python=/usr/local/bin/python3.8" >> ~/.bashrc
    source ~/.bashrc
    sudo apt -y install python3-venv python3-pip
    sudo python3.8 -m easy_install pip
    sudo pip3.8 install --upgrade pip
    sudo pip3.8 install update pip
}
#install_pistache(){
    #sudo add-apt-repository ppa:pistache+team/unstable
    #sudo apt update
    #sudo apt install libpistache-dev
#    cd $SCRIPTS_DIR/../
#    git submodule init
#    git submodule update
#    cd $SCRIPTS_DIR/../LowLevel/Common/pistache
#    mkdir -p build
#    cd build
#    cmake ..
#    make -j4

#}
install_crow(){

    cd $SCRIPTS_DIR/../
    git submodule init
    git submodule update
    sudo apt install libboost-dev libboost-date-time-dev libboost-system-dev
    cp $SCRIPTS_DIR/assets/CMakeLists.txt $SCRIPTS_DIR/../LowLevel/Common/Crow/
    cd $SCRIPTS_DIR/../LowLevel/Common/Crow
    mkdir -p build
    cd build
    cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF
    sudo make -j4 install

}
compile_all(){
    cd $SCRIPTS_DIR
    sudo ./install.sh
}

compile(){
	RECIPE=(
            preconditions
            install_gcc
            install_alsa
            install_zeromq
            install_python3
            install_db
            install_node
#            install_pistache
            install_crow
            compile_all
			)   
	build
}

echo "Compiler"
if (( $# == 0 )); then
	compile 
	exit 0
fi
