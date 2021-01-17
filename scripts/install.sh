#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

set -e

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"


install_process() {
	for SCRIPT in "${RECIPE[@]}"; do
	  echo "running \"$SCRIPT\""
	  $SCRIPT
	done

}

preconditions(){
    echo "compilation"
    cd $SCRIPTS_DIR
    cd ../
    LOCAL_REPO=$(pwd)
    echo $LOCAL_REPO
    $LOCAL_REPO/LowLevel/scripts/compiler.sh

    if [ -d /conboard ]; then
        systemctl stop usb-otg.service
        #systemctl disable usb-otg.service
        
        systemctl stop launcher.service
        #systemctl disable launcher.service

        systemctl stop frontend.service

        systemctl stop serial-getty@ttyGS0

        rm -rf /conboard
    fi
    echo "create files"
    mkdir -p /conboard
    cp -r $LOCAL_REPO /
}


install_usb_otg(){
    echo "stop all gadgets"
    $SCRIPTS_DIR/usb-gadget-stop.sh
    
    echo "recreate all pertinent services"
    cp /conboard/LowLevel/assets/usb-otg.service /etc/systemd/system/
   
    systemctl daemon-reload
    systemctl enable usb-otg.service

    systemctl restart usb-otg.service
    systemctl enable serial-getty@ttyGS0

    systemctl restart serial-getty@ttyGS0
    grep ttyGS0 /etc/securetty || echo -e "# usb gadget serial\n/dev/ttyGS0\n" >> /etc/securetty

}

install_frontend(){

    echo "python install"
    cd /conboard/frontend
    if [ ! -d venv ]; then
        python3 -m venv venv
        source ./venv/bin/activate
        pip3 install --upgrade pip
        pip3 install update pip
        pip3 install wheel numpy scipy matplotlib scikit-image scikit-learn ipython
        pip3 install -r /conboard/frontend/assets/requirements.txt
        deactivate
    fi

    echo "services"
    cp /conboard/frontend/assets/frontend.service /etc/systemd/system/
    systemctl daemon-reload
    systemctl enable frontend.service
    systemctl restart frontend.service

    echo "nginx config"
    if [ -f /etc/nginx/sites-enabled/interface.conf ]; then
        sudo rm /etc/nginx/sites-enabled/interface.conf
    fi
    cp /conboard/frontend/assets/interface.conf /etc/nginx/sites-available

    ln -s /etc/nginx/sites-available/interface.conf /etc/nginx/sites-enabled/interface.conf
    systemctl restart nginx.service

}

install_lowlevel(){

    echo "launcher"
    cp /conboard/LowLevel/assets/launcher.service /etc/systemd/system/
    systemctl enable launcher.service

    echo "event handler"
    cp /conboard/LowLevel/assets/100-usb.rules /etc/udev/rules.d/
    udevadm control --reload-rules && sudo udevadm trigger
    service udev restart

}

install(){

	RECIPE=(
			preconditions
			install_usb_otg
            install_lowlevel
            install_frontend
			)
	install_process

}

if (( $# == 0 )); then

	install
	exit 0

fi
