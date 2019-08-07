#!/usr/bin/env bash

set -x

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

if [ -v WAYLAND_DISPLAY ]
then
  wayland_display=${WAYLAND_DISPLAY}
else
  port=0
  while [ -e "${XDG_RUNTIME_DIR}/wayland-${port}" ]; do
    wayland_display=wayland-${port}
    let port+=1
  done
fi

if [ "$1" = "gnome-terminal" ]
then
  extras='--disable-factory'
fi
unset QT_QPA_PLATFORMTHEME

if [ "$1" = "gdb" ]
then
  MIR_SOCKET=${mir_socket} WAYLAND_DISPLAY=${wayland_display} XDG_SESSION_TYPE=mir GDK_BACKEND=${gdk_backend} QT_QPA_PLATFORM=${qt_qpa} SDL_VIDEODRIVER=${sdl_videodriver} NO_AT_BRIDGE=1 "$@" ${extras}
else
  MIR_SOCKET=${mir_socket} WAYLAND_DISPLAY=${wayland_display} XDG_SESSION_TYPE=mir GDK_BACKEND=${gdk_backend} QT_QPA_PLATFORM=${qt_qpa} SDL_VIDEODRIVER=${sdl_videodriver} NO_AT_BRIDGE=1 "$@" ${extras}&
fi

