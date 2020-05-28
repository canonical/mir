#!/bin/bash
set +ex

date --utc --iso-8601=seconds | xargs echo "[timestamp] Start time :"

mir_rc=0
timeout=3
wayland_display="mir-smoke-test"
options="--test-timeout=${timeout}"

if [ -v MIR_SOCKET ]
then
  if [ ! -e "${MIR_SOCKET}" ]
  then
    echo "Error: Host endpoint '${MIR_SOCKET}' does not exist"; exit 1
  fi
  options="${options} --host-socket ${MIR_SOCKET}"
fi

root="$( dirname "${BASH_SOURCE[0]}" )"

client_list=`find ${root} -name mir_demo_client_* | grep -v bin$ | sed s?${root}/??`
echo "I: client_list=" ${client_list}

### Run Tests ###

# Start with eglinfo for the system
echo Running eglinfo client
date --utc --iso-8601=seconds | xargs echo "[timestamp] Start :" ${client}
echo MIR_SERVER_ENABLE_MIRCLIENT= WAYLAND_DISPLAY=${wayland_display} ${root}/mir_demo_server ${options} --test-client eglinfo
MIR_SERVER_ENABLE_MIRCLIENT= WAYLAND_DISPLAY=${wayland_display} ${root}/mir_demo_server ${options} --test-client eglinfo
date --utc --iso-8601=seconds | xargs echo "[timestamp] End :" ${client}

for client in ${client_list}; do
    echo running client ${client}
    date --utc --iso-8601=seconds | xargs echo "[timestamp] Start :" ${client}
    echo MIR_SERVER_ENABLE_MIRCLIENT= WAYLAND_DISPLAY=${wayland_display} ${root}/mir_demo_server ${options} --test-client ${root}/${client}
    if   MIR_SERVER_ENABLE_MIRCLIENT= WAYLAND_DISPLAY=${wayland_display} ${root}/mir_demo_server ${options} --test-client ${root}/${client}
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
