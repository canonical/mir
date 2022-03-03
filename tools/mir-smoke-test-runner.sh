#!/bin/bash
set +ex

date --utc --iso-8601=seconds | xargs echo "[timestamp] Start time :"

mir_rc=0
timeout=3
wayland_display="mir-smoke-test"
options="--test-timeout=${timeout}"

root="$( dirname "${BASH_SOURCE[0]}" )"

client_list=`find ${root} -name mir_demo_client_* | grep -v bin$ | sed s?${root}/??`
echo "I: client_list=" ${client_list}

### Run Tests ###
# The CI environment sets this test-specific env var, mir_demo_server doesn't know it
unset MIR_SERVER_LOGGING

if [ -z "$XDG_RUNTIME_DIR" ]; then
  export XDG_RUNTIME_DIR=/tmp
fi

# Start with eglinfo for the system
echo Running eglinfo client
date --utc --iso-8601=seconds | xargs echo "[timestamp] Start :" ${client}
echo WAYLAND_DISPLAY=${wayland_display} ${root}/mir_demo_server ${options} --test-client eglinfo
WAYLAND_DISPLAY=${wayland_display} ${root}/mir_demo_server ${options} --test-client eglinfo
date --utc --iso-8601=seconds | xargs echo "[timestamp] End :" ${client}

for client in ${client_list}; do
  if [[ "$XAUTHORITY" =~ .*xvfb-run.* ]]; then
    if [[ "${client}" = mir_demo_client_wayland_egl_spinner ]]; then
      # Skipped because of https://github.com/MirServer/mir/issues/2154
      continue
    fi
  fi
    echo running client ${client}
    date --utc --iso-8601=seconds | xargs echo "[timestamp] Start :" ${client}
    echo WAYLAND_DISPLAY=${wayland_display} ${root}/mir_demo_server ${options} --test-client ${root}/${client}
    if   WAYLAND_DISPLAY=${wayland_display} ${root}/mir_demo_server ${options} --test-client ${root}/${client}
    then
      echo "I: [PASSED]" ${root}/${client}
    else
      echo "I: [FAILED]" ${root}/${client}
      failures="${failures} ${client}"
      mir_rc=-1
    fi
   date --utc --iso-8601=seconds | xargs echo "[timestamp] End :" ${client}
done

date --utc --iso-8601=seconds | xargs echo "[timestamp] End time :"

if [ -n "${failures}" ]
then
    echo "I: The following clients failed to execute successfully:"
    for client in ${failures}; do
        echo "I:     ${client}"
    done
fi
echo "I: Smoke testing complete with returncode ${mir_rc}"
exit ${mir_rc}
