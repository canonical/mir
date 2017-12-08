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

qt_qpa_platform=ubuntumirclient
qtubuntu_desktop_installed=$(apt list qtubuntu-desktop
 2>/dev/null | grep installed | wc -l)
if [ "${qtubuntu_desktop_installed}" == "0" ]
then
    echo "** Warning ** defaulting to Wayland backend for Qt"
    echo "For the best experience install qtubuntu-desktop - run \"sudo apt install qtubuntu-desktop\""
    qt_qpa_platform=wayland
fi

MIR_SOCKET=${mir_socket} WAYLAND_DISPLAY=${wayland_socket} XDG_SESSION_TYPE=mir GDK_BACKEND=mir QT_QPA_PLATFORM=${qt_qpa_platform} SDL_VIDEODRIVER=wayland "$@" ${extras}&
