#!/bin/bash

x11_server=Xmir

while [ $# -gt 0 ]
do
  if [ "$1" == "--help" -o "$1" == "-h" ]
  then
    echo "$(basename $0) - Handy launch script for providing an X11 server"
    echo "Usage: $(basename $0) [options] command"
    echo "Options are:"
    echo "    -Xmir      use Xmir"
    echo "    -Xwayland  use Xwayland"
    echo "(default is -${x11_server})"
    exit 0
  elif [ "$1" == "-Xmir" ];     then x11_server=Xmir
  elif [ "$1" == "-Xwayland" ]; then x11_server=Xwayland
  elif [ "${1:0:1}" == "-" ];   then echo "Unknown option: $1"; exit 1
  else break
  fi
  shift
done

unset QT_QPA_PLATFORMTHEME
export XDG_SESSION_TYPE=x11
export GDK_BACKEND=x11
export QT_QPA_PLATFORM=xcb
export SDL_VIDEODRIVER=x11

if [ "${x11_server}" == "Xmir" ];
then
  x_server_installed=$(apt list xmir 2>/dev/null | grep installed | wc -l)
  if [ "${x_server_installed}" == "0" ]; then echo "Need Xmir - run \"sudo apt install xmir\""; exit 1 ;fi

  if   [ -e "${XDG_RUNTIME_DIR}/miral_socket" ];
  then
    socket_value=${XDG_RUNTIME_DIR}/miral_socket
  elif [ -e "${XDG_RUNTIME_DIR}/mir_socket" ];
  then
    socket_value=${XDG_RUNTIME_DIR}/mir_socket
  else
    echo "Error: Cannot detect Mir endpoint"; exit 1
  fi
  socket_name=MIR_SOCKET
  x11_server_args=-rootless
elif [ "${x11_server}" == "Xwayland" ];
then
  x_server_installed=$(apt list xwayland 2>/dev/null | grep installed | wc -l)
  if [ "${x_server_installed}" == "0" ]; then echo "Need Xwayland - run \"sudo apt install xwayland\""; exit 1 ;fi

  if [ -e "${XDG_RUNTIME_DIR}/miral_wayland" ];
  then
    socket_value=miral_wayland
  else
    echo "Error: Cannot detect Mir-Wayland endpoint"; exit 1
  fi
  socket_name=WAYLAND_DISPLAY
  x11_server_args=
fi

port=0

while [ -e "/tmp/.X11-unix/X${port}" ]; do
    let port+=1
done

echo ${socket_name}=${socket_value} ${x11_server} -rootless :${port}

socket_name=${socket_value} ${x11_server} ${x11_server_args} :${port} & pid=$!
DISPLAY=:${port} "$@"
kill ${pid}
