#!/usr/bin/env bash

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
  # If there's already a compositor for WAYLAND_DISPLAY let Mir choose another
  if [ -O "${XDG_RUNTIME_DIR}/${WAYLAND_DISPLAY}" ]
  then
    unset WAYLAND_DISPLAY
  fi
  # miral-shell can launch it's own terminal with Ctrl-Alt-T
  MIR_SERVER_ENABLE_X11=1 MIR_SERVER_SHELL_TERMINAL_EMULATOR=${terminal} exec ${bindir}${miral_server} $*
else
  # miral-kiosk (and mir_demo_server) need a terminal launched, so we need to manage the WAYLAND_DISPLAY etc. here.
  port=0
  while [ -e "${XDG_RUNTIME_DIR}/wayland-${port}" ]; do
      let port+=1
  done
  wayland_display=wayland-${port}

  if [ "${miral_server}" == "miral-kiosk" ]
  then
    # Start miral-kiosk server with the chosen WAYLAND_DISPLAY
    WAYLAND_DISPLAY=${wayland_display} ${bindir}${miral_server} $*&
    miral_server_pid=$!
    unset DISPLAY
  elif [ "${miral_server}" == "mir_demo_server" ]
  then
    # With mir_demo_server we will get the display saved to this file
    x11_display_file=$(tempfile)

    # Start mir_demo_server with the chosen WAYLAND_DISPLAY
    MIR_SERVER_ENABLE_X11=1 WAYLAND_DISPLAY=${wayland_display} ${bindir}${miral_server} $* --x11-displayfd 5 5>${x11_display_file}&
    miral_server_pid=$!

    if inotifywait -qq --timeout 5 --event close_write "${x11_display_file}" && [ -s "${x11_display_file}" ]
    then
      # ${x11_display_file} contains the X11 display
      export DISPLAY=:$(cat "${x11_display_file}")
      rm "${x11_display_file}"
    else
      echo "ERROR: Failed to get X11 display from ${miral_server}"
      rm "${x11_display_file}"
      kill ${miral_server_pid}
      exit 1
    fi
  fi

  # When the server starts, launch a terminal. When the terminal exits close the server.
  until [ -O "${XDG_RUNTIME_DIR}/${wayland_display}" ]
  do
    if ! kill -0 ${miral_server_pid} &> /dev/null
    then
      echo "ERROR: ${miral_server} [pid=${miral_server_pid}] is not running"
      exit 1
    fi
    inotifywait -qq --timeout 5 --event create $(dirname "${XDG_RUNTIME_DIR}/${wayland_display}")
  done

  XDG_SESSION_TYPE=mir GDK_BACKEND=wayland,x11 QT_QPA_PLATFORM=wayland SDL_VIDEODRIVER=wayland WAYLAND_DISPLAY=${wayland_display} NO_AT_BRIDGE=1 ${terminal}
  kill ${miral_server_pid}
fi
