#!/usr/bin/env bash
# shellcheck disable=SC2154

if [ -z "${SBUILD_PROXY_URL:-}" ]; then
    return 0
fi

if [ -n "${SBUILD_PROXY_URL:-}" ] && ! command -v update-ca-certificates > /dev/null; then
    ${proxy_install_certs}
fi

install -d "${proxy_ca_directory}"
printf '%s' "${SBUILD_PROXY_CA}" | base64 -d \
    > "${proxy_ca_directory}/mir-sbuild-proxy.crt"
export http_proxy="${SBUILD_PROXY_URL}" https_proxy="${SBUILD_PROXY_URL}"
export no_proxy=localhost,127.0.0.1,::1
unset NO_PROXY
"${proxy_update_ca}"
