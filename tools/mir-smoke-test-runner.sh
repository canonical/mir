#!/bin/bash
set +ex

date --utc --iso-8601=seconds | xargs echo "[timestamp] Start time :"

mir_rc=0
timeout=3
wayland_display="mir-smoke-test"
options="--test-timeout=${timeout}"

root="$( dirname "${BASH_SOURCE[0]}" )"

# Start with eglinfo for the system
client_list="eglinfo `find ${root} -name mir_demo_client_* | grep -v bin$`"
echo "I: client_list=" ${client_list}

function run_tests()
{
  platforms="${platforms} $1"
  for client in ${client_list}; do
    echo running client ${client}
    date --utc --iso-8601=seconds | xargs echo "[timestamp] Start :" ${client}
    echo WAYLAND_DISPLAY=${wayland_display} ${root}/mir_demo_server ${options} --test-client ${client}
    if   WAYLAND_DISPLAY=${wayland_display} ${root}/mir_demo_server ${options} --test-client ${client}
    then
      echo "I: [PASSED]" ${client}
    else
      echo "I: [FAILED]" ${client}
      failures="${failures} ${client}"/$1
      # eglinfo failing doesn't count as a smoke test failure
      if [ "${client}" != "eglinfo" ]
      then
        mir_rc=-1
      fi
    fi
   date --utc --iso-8601=seconds | xargs echo "[timestamp] End :" ${client}
  done
}

### Run Tests ###
# The CI environment sets this test-specific env var, mir_demo_server doesn't know it
unset MIR_SERVER_LOGGING

if [ -z "$XDG_RUNTIME_DIR" ]; then
  export XDG_RUNTIME_DIR=/tmp
fi

echo Test virtual platform
MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:virtual MIR_SERVER_VIRTUAL_OUTPUT=1200x900 run_tests virtual

if [ -n "${DISPLAY}" ]
then
  echo Test X11 platform
  MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:x11 run_tests x11
fi

if [ -O "${XDG_RUNTIME_DIR}/${WAYLAND_DISPLAY:-wayland-0}" ]
then
  echo Test Wayland platform
  MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:wayland MIR_SERVER_WAYLAND_HOST="${WAYLAND_DISPLAY:-wayland-0}" run_tests wayland
fi

if [ -z "$XDG_CURRENT_DESKTOP" ]
then
  echo Test gbm-kms platform
  MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:gbm-kms run_tests gbm-kms

  echo Test atomic-kms platform
  MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:atomic-kms run_tests atomic-kms
fi

date --utc --iso-8601=seconds | xargs echo "[timestamp] End time :"

if [ -n "${failures}" ]
then
    echo "I: The following clients failed to execute successfully:"
    for client in ${failures}; do
        echo "I:     ${client}"
    done
fi
echo "I: Smoke testing complete with returncode ${mir_rc}, platforms:${platforms}"
exit ${mir_rc}
