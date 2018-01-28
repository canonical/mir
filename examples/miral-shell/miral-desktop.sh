#! /bin/bash

socket=${XDG_RUNTIME_DIR}/miral_socket
wayland_display=miral_wayland
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
    echo "    -kiosk                        use miral-kiosk instead of ${miral_server}"
    echo "    -launcher <launcher>          use <launcher> instead of '${launcher}'"
    echo "    -vt <termid>                  set the virtual terminal [${vt}]"
    echo "    -socket <socket>              set the legacy mir socket [${socket}]"
    echo "    -wayland-socket-name <socket> set the wayland socket [${wayland_display}]"
    echo "    -bindir <bindir>              path to the miral executable [${bindir}]"
    exit 0
  elif [ "$1" == "-kiosk" ];              then miral_server=miral-kiosk
  elif [ "$1" == "-launcher" ];           then shift; launcher=$1
  elif [ "$1" == "-vt" ];                 then shift; vt=$1
  elif [ "$1" == "-socket" ];             then shift; socket=$1
  elif [ "$1" == "-wayland-socket-name" ];then shift; wayland_display=$1
  elif [ "$1" == "-bindir" ];             then shift; bindir=$1
  elif [ "${1:0:2}" == "--" ];            then break
  fi
  shift
done

if [ "${bindir}" != "" ]; then bindir="${bindir}/"; fi

if [ -e "${socket}" ]; then echo "Error: session endpoint '${socket}' already exists"; exit 1 ;fi
if [ -e "${XDG_RUNTIME_DIR}/${wayland_display}" ]; then echo "Error: wayland endpoint '${wayland_display}' already exists"; exit 1 ;fi

vt_login_session=$(who -u | grep tty${vt} | grep ${USER} | wc -l)
if [ "${vt_login_session}" == "0" ]; then echo "Error: please log into tty${vt} first"; exit 1 ;fi

oldvt=$(sudo fgconsole)
sudo --background --preserve-env sh -c "${bindir}${miral_server} --wayland-socket-name ${wayland_display} --vt ${vt} --arw-file --file ${socket} $*; chvt ${oldvt}"

while [ ! -e "${socket}" ]; do echo "waiting for ${socket}"; sleep 1 ;done

unset QT_QPA_PLATFORMTHEME
MIR_SOCKET=${socket} XDG_SESSION_TYPE=mir GDK_BACKEND=wayland,mir QT_QPA_PLATFORM=wayland SDL_VIDEODRIVER=wayland WAYLAND_DISPLAY=${wayland_display} dbus-run-session -- ${launcher}
sudo killall ${bindir}${miral_server} || sudo killall ${bindir}${miral_server}.bin

