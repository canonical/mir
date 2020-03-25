#!/bin/env bash

miral_server=miral-shell
bindir=$(dirname $0)

terminal=${bindir}/miral-terminal

while [ $# -gt 0 ]
do
  if [ "$1" == "--help" -o "$1" == "-h" ]
  then
    echo "$(basename $0) - Handy launch script for a hosted miral \"desktop session\""
    echo "Usage: $(basename $0) [options] [shell options]"
    echo "Options are:"
    echo "    -kiosk                      use miral-kiosk instead of ${miral_server}"
    echo "    -terminal <terminal>        use <terminal> instead of '${terminal}'"
    echo "    -bindir <bindir>            path to the miral executable [${bindir}]"
    # omit    -demo-server as mir_demo_server is in the mir-test-tools package
    exit 0
  elif [ "$1" == "-kiosk" ];            then miral_server=miral-kiosk
  elif [ "$1" == "-terminal" ];         then shift; terminal=$1
  elif [ "$1" == "-bindir" ];           then shift; bindir=$1
  elif [ "$1" == "-demo-server" ];      then miral_server=mir_demo_server
  elif [ "${1:0:2}" == "--" ];          then break
  fi
  shift
done

if [ "${bindir}" != "" ]; then bindir="${bindir}/"; fi

if [ "${miral_server}" == "miral-shell" ]
then
  if [ "$(lsb_release -c -s)" == "xenial" ]
  then
    export MIR_SERVER_APP_ENV="GDK_BACKEND=x11:QT_QPA_PLATFORM=xcb:SDL_VIDEODRIVER=x11:-QT_QPA_PLATFORMTHEME:NO_AT_BRIDGE=1:QT_ACCESSIBILITY:QT_LINUX_ACCESSIBILITY_ALWAYS_ON:_JAVA_AWT_WM_NONREPARENTING=1:-GTK_MODULES:-OOO_FORCE_DESKTOP:-GNOME_ACCESSIBILITY:"
  fi

  # miral-shell can launch it's own terminal with Ctrl-Alt-T
  MIR_SERVER_ENABLE_X11=1 MIR_SERVER_SHELL_TERMINAL_EMULATOR=${terminal} exec ${bindir}${miral_server} $*
else
  if [ "$(lsb_release -c -s)" != "xenial" ]
  then
    qt_qpa=wayland
    gdk_backend=wayland
    sdl_videodriver=wayland
  else
    qt_qpa=xcb
    gdk_backend=x11
    sdl_videodriver=x11
  fi

  port=0
  while [ -e "${XDG_RUNTIME_DIR}/wayland-${port}" ]; do
      let port+=1
  done
  wayland_display=wayland-${port}

  if [ -e "${XDG_RUNTIME_DIR}/${wayland_display}" ]; then echo "Error: wayland endpoint '${wayland_display}' already exists"; exit 1 ;fi

  # With mir_demo_server we will get the display saved to this file
  x11_display_file=$(tempfile)
  # miral-kiosk (and mir_demo_server) need a terminal launched
  if [ "${miral_server}" == "mir_demo_server" ]
  then
    MIR_SERVER_ENABLE_X11=1 WAYLAND_DISPLAY=${wayland_display} ${bindir}${miral_server} $* --x11-displayfd 5 5>${x11_display_file}&
  else # miral-kiosk
                            WAYLAND_DISPLAY=${wayland_display} ${bindir}${miral_server} $*&
  fi

  while [ ! -e "${XDG_RUNTIME_DIR}/${wayland_display}" ]; do echo "waiting for ${wayland_display}"; sleep 1 ;done

  # With mir_demo_server ${x11_display_file} contains the X11 display
  if [ -e "${x11_display_file}" ]
  then
    export DISPLAY=:$(cat "${x11_display_file}")
    rm "${x11_display_file}"
  else
    unset DISPLAY
  fi

  XDG_SESSION_TYPE=mir GDK_BACKEND=${gdk_backend} QT_QPA_PLATFORM=${qt_qpa} SDL_VIDEODRIVER=${sdl_videodriver} WAYLAND_DISPLAY=${wayland_display} NO_AT_BRIDGE=1 ${terminal}
  killall ${bindir}${miral_server} || killall ${bindir}${miral_server}.bin
fi
