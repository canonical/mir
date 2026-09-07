#!/bin/bash
#
# Run a TICS command-line tool inside the Workshop.
#
# `tiobe/tics-github-action` is a JavaScript action, so it always runs on the
# host runner and cannot be nested in `workshop exec`. The build it analyses,
# however, happens inside the Workshop, where the checkout is mounted at
# /project - so `compile_commands.json` refers to Workshop paths and a host-side
# analysis fails with "[ERROR 5000] Unable to find compilation line".
#
# With `installTics: false` the action invokes the TICS tools by name, resolved
# through the PATH. Installing this script under those names therefore runs the
# analysis itself inside the Workshop, sharing the filesystem view of the build,
# while the action keeps doing the reporting (annotations, quality gate) on the
# host.
set -euo pipefail

tool="$( basename "$0" )"

: "${GITHUB_WORKSPACE:?the host path of the checkout is needed to translate paths}"

# The action refers to files (like the changed files list) by their host path;
# quoting the pattern keeps it from being taken as a glob
args=()
for arg in "$@"; do
  args+=( "${arg//"${GITHUB_WORKSPACE}"//project}" )
done

# Only variable names are passed to `workshop exec`, so the values - including
# the authentication token - are never exposed in the process arguments
# shellcheck disable=SC2016  # the inner script is expanded inside the Workshop
exec workshop exec \
  --project "${GITHUB_WORKSPACE}" \
  --env TICS \
  --env TICSAUTHTOKEN \
  --env TICSCI \
  --env TICSIDE \
  --env TICSHOSTNAMEVERIFICATION \
  --env TICSTRUSTSTRATEGY \
  --env "TICS_TOOL=${tool}" \
  -- bash -c '
    set -euo pipefail

    # Bootstrap the TICS tools the way the action does on a runner: ask the
    # viewer for the Linux installation script, then source it
    base="${TICS%%/api/*}"
    install="$(
      curl --get --silent --show-error --fail \
        --header "authorization: Basic ${TICSAUTHTOKEN}" \
        --header "x-requested-with: tics" \
        --data-urlencode "platform=linux" \
        --data-urlencode "url=${base}" \
        "${TICS}" |
      python3 -c "import json, sys; print(json.load(sys.stdin)[\"links\"][\"installTics\"])"
    )"

    script="$( mktemp )"
    curl --silent --show-error --fail --output "${script}" "${base}${install}"

    # shellcheck source=/dev/null
    source "${script}"
    rm --force "${script}"

    "${TICS_TOOL}" "$@"
  ' -- "${args[@]}"
