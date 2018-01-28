#!/bin/sh

if   [ -e "${XDG_RUNTIME_DIR}/miral_socket" ];
then
  mir_socket=${XDG_RUNTIME_DIR}/miral_socket
elif [ -e "${XDG_RUNTIME_DIR}/mir_socket" ];
then
  mir_socket=${XDG_RUNTIME_DIR}/mir_socket
else
  echo "Error: Cannot detect Mir endpoint"; exit 1
fi

if [ -e "${XDG_RUNTIME_DIR}/miral_wayland" ];
then
  wayland_socket=miral_wayland
elif [ -e "${XDG_RUNTIME_DIR}/wayland-1" ]
then
  wayland_socket=wayland-1
elif [ -e "${XDG_RUNTIME_DIR}/wayland-0" ]
then
  wayland_socket=wayland-0
else
  echo "Error: Cannot detect Mir-Wayland endpoint"; exit 1
fi

if [ "$1" = "gnome-terminal" ]
then extras='--app-id com.canonical.miral.Terminal'
fi
unset QT_QPA_PLATFORMTHEME

MIR_SOCKET=${mir_socket} WAYLAND_DISPLAY=${wayland_socket} XDG_SESSION_TYPE=mir GDK_BACKEND=wayland,mir QT_QPA_PLATFORM=wayland SDL_VIDEODRIVER=wayland "$@" ${extras}&
