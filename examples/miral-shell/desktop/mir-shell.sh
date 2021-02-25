#!/bin/sh
set -e

bindir="$(dirname "$0")"

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
