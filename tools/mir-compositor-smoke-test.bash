#!/bin/bash
set +ex

date --utc --iso-8601=seconds | xargs echo "[timestamp] Start time :"

mir_rc=0
timeout=3
options="--test-timeout=${timeout}"

root="$( dirname "${BASH_SOURCE[0]}" )"
compositor_list="`find ${root} -name miral* | grep -v -e bin$ -e test -e terminal -e app`"

unset MIR_SERVER_LOGGING

if [ -z "$XDG_RUNTIME_DIR" ]; then
  export XDG_RUNTIME_DIR=/tmp
fi

for compositor in ${compositor_list}; do
  echo Test ${compositor}
  wayland_host=wayland_host
#  WAYLAND_DISPLAY=${wayland_host} MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:virtual MIR_SERVER_VIRTUAL_OUTPUT=1280x1024 ${compositor}&
  WAYLAND_DISPLAY=${wayland_host} ${compositor}&
  sc_pid=$!
  until [ -O "${XDG_RUNTIME_DIR}/${wayland_host}" ]
  do
    if ! kill -0 ${sc_pid} &> /dev/null
    then
      echo "E: ${compositor} [pid=${sc_pid}] is not running"
      mir_rc=-1
      break
    fi
    inotifywait -qq --timeout ${timeout} --event create $(dirname "${XDG_RUNTIME_DIR}/${wayland_host}")
  done

  if env -u WAYLAND_DISPLAY -u DISPLAY MIR_SERVER_WAYLAND_HOST=${wayland_host} ${root}/mir_demo_server ${options} --test-client ${root}/mir_demo_client_wayland
  then
    successes="${successes} ${compositor}"
  else
    echo "E: running on ${compositor} failed"
    failures="${failures} ${compositor}"
    mir_rc=-1
  fi
  kill ${sc_pid}
  wait %1
done

if [ -n "${successes}" ]
then
    echo "I: The following compositors executed successfully:${successes}"
fi

if [ -n "${failures}" ]
then
    echo "I: The following compositors failed to execute successfully:${failures}"
fi

echo "I: compositor testing complete with returncode ${mir_rc}"
exit ${mir_rc}
