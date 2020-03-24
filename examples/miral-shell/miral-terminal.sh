#!/bin/sh

for terminal in gnome-terminal.real gnome-terminal weston-terminal qterminal lxterminal x-terminal-emulator xdg-terminal
do
  if which $terminal > /dev/null
  then break;
  fi
done

if [ "$terminal" != "gnome-terminal.real" ] && [ "$terminal" != "gnome-terminal" ]
then
  exec $terminal "$@"
else
  for terminal_server in /usr/libexec/gnome-terminal-server /usr/lib/gnome-terminal/gnome-terminal-server
  do
    if [[ -x "$terminal_server" ]];  then break; fi
  done
  $terminal_server --app-id io.mirserver.Terminal&
  pid=$!
  sleep 0.1
  $terminal --app-id io.mirserver.Terminal
  wait $pid
fi