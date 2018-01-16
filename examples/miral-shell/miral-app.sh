#! /bin/bash

miral_server=miral-shell
launcher=qterminal
hostsocket=
bindir=$(dirname $0)

if [ -n "${MIR_SOCKET}" ]
then
  if [ ! -e "${MIR_SOCKET}" ]
  then
    echo "Error: Host endpoint '${MIR_SOCKET}' does not exist"; exit 1
  fi
  hostsocket='--host-socket ${MIR_SOCKET}'
fi

socket=${XDG_RUNTIME_DIR}/miral_socket
wayland_display=miral_wayland

while [ $# -gt 0 ]
do
  if [ "$1" == "--help" -o "$1" == "-h" ]
  then
    echo "$(basename $0) - Handy launch script for a hosted miral \"desktop session\""
    echo "Usage: $(basename $0) [options] [shell options]"
    echo "Options are:"
    echo "    -kiosk                        use miral-kiosk instead of ${miral_server}"
    echo "    -launcher <launcher>          use <launcher> instead of '${launcher}'"
    echo "    -socket <socket>              set the legacy mir socket [${socket}]"
    echo "    -wayland-socket-name <socket> set the wayland socket [${wayland_display}]"
    echo "    -bindir <bindir>              path to the miral executable [${bindir}]"
    exit 0
    elif [ "$1" == "-kiosk" ];              then miral_server=miral-kiosk
    elif [ "$1" == "-launcher" ];           then shift; launcher=$1
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


sh -c "${bindir}${miral_server} $* ${hostsocket} --file ${socket} --wayland-socket-name ${wayland_display} --desktop_file_hint=miral-shell.desktop"&

while [ ! -e "${socket}" ]; do echo "waiting for ${socket}"; sleep 1 ;done

unset QT_QPA_PLATFORMTHEME
MIR_SOCKET=${socket} XDG_SESSION_TYPE=mir GDK_BACKEND=mir QT_QPA_PLATFORM=wayland SDL_VIDEODRIVER=wayland WAYLAND_DISPLAY=${wayland_display} dbus-run-session -- ${launcher}
killall ${bindir}${miral_server} || killall ${bindir}${miral_server}.bin

