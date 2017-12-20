#!/bin/bash
set +ex

date --utc --iso-8601=seconds | xargs echo "[timestamp] Start time :"

mir_rc=0
timeout=3

if [ -v MIR_SOCKET ]
then
  if [ ! -e "${MIR_SOCKET}" ]
  then
    echo "Error: Host endpoint '${MIR_SOCKET}' does not exists"; exit 1
  fi
  export MIR_SERVER_HOST_SOCKET=${MIR_SOCKET}
fi

export WAYLAND_DISPLAY=mir-smoke-test
export MIR_SERVER_WAYLAND_SOCKET_NAME=${WAYLAND_DISPLAY}
root="$( dirname "${BASH_SOURCE[0]}" )"

client_list=`find ${root} -name mir_demo_client_* | grep -v bin$ | sed s?${root}/??`
echo "I: client_list=" ${client_list}

### Run Tests ###

for client in ${client_list}; do
    echo running client ${client}
    date --utc --iso-8601=seconds | xargs echo "[timestamp] Start :" ${client}
    echo ${root}/mir_demo_server --test-timeout=${timeout} --test-client ${root}/${client}
    if   ${root}/mir_demo_server --test-timeout=${timeout} --test-client ${root}/${client}
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
