#! /bin/bash

socket=${XDG_RUNTIME_DIR}/miral_socket
miral_server=miral-shell
launcher=qterminal
bindir=$(dirname $0)
vt=4

while [ $# -gt 0 ]
do
  if [ "$1" == "--help" -o "$1" == "-h" ]
  then
    echo "$(basename $0) - Handy launch script for a miral \"desktop session\""
    echo "Usage: $(basename $0) [options] [shell options]"
    echo "Options are:"
    echo "    -kiosk               use miral-kiosk instead of ${miral_server}"
    echo "    -launcher <launcher> use <launcher> instead of '${launcher}'"
    echo "    -vt       <termid>   set the virtual terminal [${vt}]"
    echo "    -socket   <socket>   set the mir socket [${socket}]"
    echo "    -bindir   <bindir>   path to the miral executable [${bindir}]"
    exit 0
    elif [ "$1" == "-kiosk" ];              then miral_server=miral-kiosk
    elif [ "$1" == "-launcher" ];           then shift; launcher=$1
    elif [ "$1" == "-vt" ];                 then shift; vt=$1
    elif [ "$1" == "-socket" ];             then shift; socket=$1
    elif [ "$1" == "-bindir" ];             then shift; bindir=$1/
    elif [ "${1:0:2}" == "--" ];            then break
    fi
    shift
done

if [ "${bindir}" != "" ]; then bindir="${bindir}/"; fi

if [ -e "${socket}" ]; then echo "Error: '${socket}' already exists"; exit 1 ;fi

sudo ls >> /dev/null
oldvt=$(sudo fgconsole)
sudo sh -c "LD_LIBRARY_PATH=${LD_LIBRARY_PATH} ${bindir}${miral_server} --vt ${vt} --arw-file --file ${socket} $*; chvt ${oldvt}"&

while [ ! -e "${socket}" ]; do echo "waiting for ${socket}"; sleep 1 ;done

unset QT_QPA_PLATFORMTHEME
MIR_SOCKET=${socket} XDG_SESSION_TYPE=mir GDK_BACKEND=mir QT_QPA_PLATFORM=ubuntumirclient SDL_VIDEODRIVER=mir dbus-run-session -- ${launcher}
sudo killall ${bindir}${miral_server}

