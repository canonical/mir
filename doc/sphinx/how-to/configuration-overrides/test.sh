#!/bin/bash
# End-to-end smoke test for the demo-config-override example.
#
# Builds the example, seeds base + drop-in config files under a private
# XDG_CONFIG_HOME, runs the compositor against the virtual platform, and
# asserts the merged output reflects override precedence.

set -euo pipefail

cd "$(dirname "$0")"

# [doc:config-override:build]
cmake .
cmake --build .
# [doc:config-override:build-end]

export XDG_CONFIG_HOME="$(mktemp -d)"
export XDG_RUNTIME_DIR="$(mktemp -d)"
trap 'rm -rf "${XDG_CONFIG_HOME}" "${XDG_RUNTIME_DIR}" /tmp/config-override.log' EXIT

mkdir -p "${XDG_CONFIG_HOME}/demo-compositor.conf.d"
cat > "${XDG_CONFIG_HOME}/demo-compositor.conf" <<'CONF'
display_background=black
display_workspaces=home
display_workspaces=work
CONF
cat > "${XDG_CONFIG_HOME}/demo-compositor.conf.d/90-user.conf" <<'CONF'
display_background=blue
display_workspaces=gaming
CONF

timeout 10 bash -e <<'INNER_EOF' || [ $? -eq 124 ]

  trap "jobs -p | xargs -r kill" SIGINT SIGTERM EXIT

  export \
    MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:virtual \
    MIR_SERVER_VIRTUAL_OUTPUT=800x600 \
    XDG_CONFIG_HOME=${XDG_CONFIG_HOME}

  WAYLAND_DISPLAY=wayland-99 ./demo-config-override > /tmp/config-override.log 2>&1 &
  COMPOSITOR_PID=$!

  timeout --preserve-status 3 bash -c "while [ ! -S $XDG_RUNTIME_DIR/wayland-99 ]; do sleep 0.1; done"

kill $COMPOSITOR_PID
wait $COMPOSITOR_PID || true
INNER_EOF

for expected in "display_background=blue" "display_workspaces=home,work,gaming"; do
  grep -qF "${expected}" /tmp/config-override.log || {
    echo "FAIL: expected '${expected}' not found in compositor output:" >&2
    sed 's/^/  /' /tmp/config-override.log >&2
    exit 1
  }
done
