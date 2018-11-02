#!/usr/bin/env bash
width=1920
height=1080
output=screencast.mp4
socket=${XDG_RUNTIME_DIR}/miral_socket
if [ -v MIR_SOCKET ]; then socket=${MIR_SOCKET}; fi
bindir="$( dirname "${BASH_SOURCE[0]}" )"

while [ $# -gt 0 ]
do
  if [ "$1" == "--help" -o "$1" == "-h" ]
  then
    echo "$(basename $0) - screencast capture script for use with Mir servers"
    echo "Usage: $0 [options]"
    echo "Options are:"
    echo "    --width   set the capture width   [${width}]"
    echo "    --height  set the capture height  [${height}]"
    echo "    --output  set the output filename [${output}]"
    echo "    --socket  set the mir socket      [${socket}]"
    exit 0
  elif [ "$1" == "--socket" ]; then shift; socket=$1
  elif [ "$1" == "--output" ]; then shift; output=$1
  elif [ "$1" == "--width"  ]; then shift; width=$1
  elif [ "$1" == "--height" ]; then shift; height=$1
  fi
  shift
done

echo width = ${width}
echo height = ${height}
echo output = ${output}
echo socket = ${socket}

if ! which mirscreencast >> /dev/null ; then echo "Need mirscreencast - run \"sudo apt install mir-utils\""; exit 1 ;fi
if ! which mencoder >> /dev/null ;      then echo "Need mencoder - run \"sudo apt install mencoder\""; exit 1 ;fi

if [ -e ${output} ]; then echo "Output exists, moving to ${output}~"; mv ${output} ${output}~ ;fi
while [ ! -e "${socket}" ]; do echo "waiting for ${socket}"; sleep 1 ;done

if [ ! -v tempfile ]; then tempfile=$(mktemp); fi
${bindir}/mirscreencast -m ${socket}   --size ${width}   ${height} --stdout | \
mencoder -demuxer rawvideo -rawvideo fps=60:w=${width}:h=${height}:format=bgra -ovc x264 -o ${output} -
