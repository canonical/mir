#! /bin/bash

qt_qpa=wayland
gdk_backend=wayland,mir
sdl_videodriver=wayland

while [ $# -gt 0 ]
do
  if [ "$1" == "--help" -o "$1" == "-h" ]
  then
    echo "$(basename $0) - Handy launch script for clients of a Mir server"
    echo "Usage: $(basename $0) [options] <client> [client args]"
    echo "Options are:"
    echo "    -qt-mirclient                 use ubuntumirclient instead of qtwayland"
    echo "    -gtk-mirclient                GTK uses mir instead of wayland,mir"
    echo "    -sdl-mirclient                SDL uses mir instead of wayland"
    exit 0
  elif [ "$1" == "-qt-mirclient" ];       then qt_qpa=ubuntumirclient
  elif [ "$1" == "-gtk-mirclient" ];      then gdk_backend=mir
  elif [ "$1" == "-sdl-mirclient" ];      then sdl_videodriver=mir
  else break
  fi
  shift
done

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

if [ "$1" = "gdb" ]
then
  MIR_SOCKET=${mir_socket} WAYLAND_DISPLAY=${wayland_socket} XDG_SESSION_TYPE=mir GDK_BACKEND=${gdk_backend} QT_QPA_PLATFORM=${qt_qpa} SDL_VIDEODRIVER=${sdl_videodriver} NO_AT_BRIDGE=1 "$@" ${extras}
else
  MIR_SOCKET=${mir_socket} WAYLAND_DISPLAY=${wayland_socket} XDG_SESSION_TYPE=mir GDK_BACKEND=${gdk_backend} QT_QPA_PLATFORM=${qt_qpa} SDL_VIDEODRIVER=${sdl_videodriver} NO_AT_BRIDGE=1 "$@" ${extras}&
fi

