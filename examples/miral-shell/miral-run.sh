#!/usr/bin/env bash

while [ $# -gt 0 ]
do
  if [ "$1" == "--help" -o "$1" == "-h" ]
  then
    echo "$(basename $0) - Handy launch script for clients of a Mir server"
    echo "Usage: $(basename $0) <client> [client args]"
    exit 0
  else break
  fi
  shift
done

if [ "$(lsb_release -c -s)" == "xenial" ]
then
  qt_qpa=ubuntumirclient
  gdk_backend=mir
  sdl_videodriver=mir

  if   [ -e "${XDG_RUNTIME_DIR}/miral_socket" ];
  then
    mir_socket=${XDG_RUNTIME_DIR}/miral_socket
  elif [ -e "${XDG_RUNTIME_DIR}/mir_socket" ];
  then
    mir_socket=${XDG_RUNTIME_DIR}/mir_socket
  else
    echo "Error: Cannot detect Mir endpoint"; exit 1
  fi
else
  qt_qpa=wayland
  gdk_backend=wayland
  sdl_videodriver=wayland
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
then
  extras='--disable-factory'
fi
unset QT_QPA_PLATFORMTHEME

if [ "$1" = "gdb" ]
then
  MIR_SOCKET=${mir_socket} WAYLAND_DISPLAY=${wayland_socket} XDG_SESSION_TYPE=mir GDK_BACKEND=${gdk_backend} QT_QPA_PLATFORM=${qt_qpa} SDL_VIDEODRIVER=${sdl_videodriver} NO_AT_BRIDGE=1 "$@" ${extras}
else
  MIR_SOCKET=${mir_socket} WAYLAND_DISPLAY=${wayland_socket} XDG_SESSION_TYPE=mir GDK_BACKEND=${gdk_backend} QT_QPA_PLATFORM=${qt_qpa} SDL_VIDEODRIVER=${sdl_videodriver} NO_AT_BRIDGE=1 "$@" ${extras}&
fi

