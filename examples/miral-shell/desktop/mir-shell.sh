#!/bin/bash
set -e

bindir="$(dirname "$0")"

while [ $# -gt 0 ]
do
  if [ "$1" = "--help" ] || [ "$1" = "-h" ]
  then
    echo "$(basename "$0") - Launch script for \"Mir Shell\""
    echo "Usage: $(basename "$0") [options] [shell options]"
    echo "Options are:"
    echo "    -bindir <bindir>            path to the miral-shell executable [${bindir}]"
    exit 0
  elif [ "$1" = "-bindir" ];  then shift; bindir=$1
  elif [ "${1:0:2}" = "--" ]; then break
  fi
  shift
done

if [ "${bindir}" != "" ]; then bindir="${bindir}/"; fi

unset QT_QPA_PLATFORMTHEME

if command -v gsettings > /dev/null
then
  keymap_index=$(gsettings get org.gnome.desktop.input-sources current | cut -d\  -f 2)
  if [ -n "$keymap_index" ]
  then
    keymap=$(gsettings get org.gnome.desktop.input-sources sources | grep -Po "'[[:alpha:]]+'\)" | sed -ne "s/['|)]//g;$((keymap_index+1))p")
    if [ -n "$keymap" ]
    then
      export MIR_SERVER_KEYMAP=${keymap}
    fi
  fi
fi

MIR_SERVER_ENABLE_X11=1 exec "${bindir}miral-shell" "$@"
