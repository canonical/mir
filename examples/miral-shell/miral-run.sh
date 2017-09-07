#!/bin/sh

if   [ -e "${XDG_RUNTIME_DIR}/miral_socket" ];
then
  socket=${XDG_RUNTIME_DIR}/miral_socket
elif [ -e "${XDG_RUNTIME_DIR}/mir_socket" ];
then
  socket=${XDG_RUNTIME_DIR}/mir_socket
else
  echo "Error: Cannot detect Mir endpoint"; exit 1
fi

if [ "$1" = "gnome-terminal" ]
then extras='--app-id com.canonical.miral.Terminal'
fi
unset QT_QPA_PLATFORMTHEME
MIR_SOCKET=${socket} XDG_SESSION_TYPE=mir GDK_BACKEND=mir QT_QPA_PLATFORM=ubuntumirclient SDL_VIDEODRIVER=mir "$@" ${extras}&
