#!/usr/bin/env bash

bindir=$(dirname $0)

for terminal in gnome-terminal weston-terminal qterminal lxterminal x-terminal-emulator xdg-terminal
do
  if which $terminal > /dev/null
  then break;
  fi
done

while [ $# -gt 0 ]
do
  if [ "$1" == "--help" -o "$1" == "-h" ]
  then
    echo "$(basename $0) - Launch script for \"Mir Shell\""
    echo "Usage: $(basename $0) [options] [shell options]"
    echo "Options are:"
    echo "    -terminal <terminal>        use <terminal> instead of '${terminal}'"
    echo "    -bindir <bindir>            path to the miral-shell executable [${bindir}]"
    exit 0
  elif [ "$1" == "-terminal" ];         then shift; terminal=$1
  elif [ "$1" == "-bindir" ];           then shift; bindir=$1
  elif [ "${1:0:2}" == "--" ];          then break
  fi
  shift
done

if [ "${bindir}" != "" ]; then bindir="${bindir}/"; fi

unset QT_QPA_PLATFORMTHEME

# On Xenial fall back to X11 for all the toolkits
if [ "$(lsb_release -c -s)" == "xenial" ]
then
  export MIR_SERVER_APP_ENV="GDK_BACKEND=x11:QT_QPA_PLATFORM=xcb:SDL_VIDEODRIVER=x11:-QT_QPA_PLATFORMTHEME:NO_AT_BRIDGE=1:QT_ACCESSIBILITY:QT_LINUX_ACCESSIBILITY_ALWAYS_ON:_JAVA_AWT_WM_NONREPARENTING=1:-GTK_MODULES:-OOO_FORCE_DESKTOP:-GNOME_ACCESSIBILITY:"
fi

if which gsettings > /dev/null
then
  keymap_index=$(gsettings get org.gnome.desktop.input-sources current | cut -d\  -f 2)
  if [ ! -z "$keymap_index" ]
  then
    keymap=$(gsettings get org.gnome.desktop.input-sources sources | grep -Po "'[[:alpha:]]+'\)" | sed -ne "s/['|)]//g;$(($keymap_index+1))p")
    if [ ! -z "$keymap" ]
    then
      export MIR_SERVER_KEYMAP=${keymap}
    fi
  fi
fi

# miral-shell can launch it's own terminal with Ctrl-Alt-T
MIR_SERVER_ENABLE_X11=1 MIR_SERVER_SHELL_TERMINAL_EMULATOR=${terminal} exec ${bindir}miral-shell $*
