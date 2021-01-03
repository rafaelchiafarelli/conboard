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
    $SCRIPTS_DIR/compile.sh
    mkdir -p /conboard
    cp -r $SCRIPTS_DIR/../ /conboard
}


install_usb_otg(){
    cp /conboard/LowLevel/assets/usb-otg.service /etc/systemd/system/
    systemctl enable usb-otg.service
    systemctl restart usb-otg.service
    systemctl enable serial-getty@ttyGS0
    systemctl restart serial-getty@ttyGS0
    grep ttyGS0 /etc/securetty || echo -e "# usb gadget serial\n/dev/ttyGS0\n" >> /etc/securetty
}

install_frontend(){
    cd /conboard/frontend
    if [ ! -d venv ]; then
        python3 -m venv venv
        source ./venv/bin/activate
        pip install --upgrade pip
        pip install update pip
        pip install -r /conboard/frontend/assets/requirements.txt
        pip install wheel numpy scipy matplotlib scikit-image scikit-learn ipython
        deactivate
    fi
    cp /conboard/frontend/assets/frontend.service /etc/systemd/system/
    systemctl daemon-reload
    systemctl enable frontend.service
    systemctl restart frontend.service
    

}

install_lowlevel(){
    echo "launcher"
    cp /conboard/LowLevel/assets/launcher.sh /conboard/launcher.sh
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
