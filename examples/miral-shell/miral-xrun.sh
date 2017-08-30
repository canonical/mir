#!/bin/bash
port=0

while [ -e "/tmp/.X11-unix/X${port}" ]; do
    let port+=1
done

unset QT_QPA_PLATFORMTHEME
unset GDK_BACKEND
unset QT_QPA_PLATFORM
unset SDL_VIDEODRIVER

if   [ -e "${XDG_RUNTIME_DIR}/miral_socket" ];
then
  socket=${XDG_RUNTIME_DIR}/miral_socket
elif [ -e "${XDG_RUNTIME_DIR}/mir_socket" ];
then
  socket=${XDG_RUNTIME_DIR}/mir_socket
else
  echo "Error: Cannot detect Mir endpoint"; exit 1
fi

while [ $# -gt 0 ]
do
  if [ "$1" == "--help" -o "$1" == "-h" ]
  then
    echo "$(basename $0) - Handy launch script for providing an Xmir X11 server"
    echo "Usage: $(basename $0) [options] command"
    echo "Options are:"
    echo "    -force  set toolkit environment variables to force X11 use"
    exit 0
  elif [ "$1" == "-force" ];
  then
    export XDG_SESSION_TYPE=x11
    export GDK_BACKEND=x11
    export QT_QPA_PLATFORM=xcb
    export SDL_VIDEODRIVER=x11
  else break
  fi
  shift
done

MIR_SOCKET=${socket} Xmir -rootless :${port} & pid=$!
DISPLAY=:${port} "$@"
kill ${pid}
