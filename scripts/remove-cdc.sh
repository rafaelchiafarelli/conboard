#!/usr/bin/env bash

function checkModule(){
  MODULE="$1"
  if lsmod | grep "$MODULE" &> /dev/null ; then
    echo "$MODULE" found.
    return 0
  else
    echo "$MODULE" not found.
    return 1
  fi
}


if which 'systemctl' | grep "systemctl" &> /dev/null ; then
 systemctl stop serial-getty@ttyGS0.service >/dev/null
fi

if checkModule "g_serial" == 0; then
  modprobe -r g_serial
fi

if checkModule "usb_f_acm" == 0; then
  modprobe -r usb_f_acm
fi

if ! checkModule "libcomposite" == 0; then
  modprobe libcomposite
fi
