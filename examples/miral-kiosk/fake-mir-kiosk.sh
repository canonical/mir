#!/bin/sh

bindir=$(dirname $0)
if [ "${bindir}" != "" ]; then bindir="${bindir}/"; fi

while [ $# -gt 0 ]
do
  if [ "$1" == "--help" -o "$1" == "-h" ]
  then
    echo "$(basename $0) - Script to launch a fake mir-kiosk daemon on the desktop"
    echo "Usage: $(basename $0) [miral-kiosk options]"
    echo ""
    echo "This allows snap daemons that expect mir-kiosk to be tested on the desktop"
    exit 0
  elif [ "${1:0:2}" == "--" ];          then break
  fi
  shift
done

unset WAYLAND_DISPLAY
sudo mkdir -p /run/user/0 && sudo XDG_RUNTIME_DIR=/run/user/0 ${bindir}miral-kiosk "$@"