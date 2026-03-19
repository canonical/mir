#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"

build_dir="${repo_root}/build"
wayland_explorer_dir=""
install_deps=false
install_wlprobe=false
skip_build=false
prerelease=false
show_diff=false
keep_temp_dir=false
release_tag=""

mir_pid=""
mir_log=""
temp_runtime_dir=""
temp_wayland_explorer_dir=""

function usage() {
  cat <<'EOF'
Usage: tools/test-update-wayland-explorer.sh [options]

Runs the same local generation flow as .github/workflows/update-wayland-explorer.yml:
release gate -> optional build -> run miral-shell (virtual) -> wlprobe -> update mir.json -> diff.

Options:
  --release-tag vX.Y.Z      Release tag to test. Defaults to v<project VERSION> from CMakeLists.txt.
  --prerelease              Simulate prerelease=true (gate should skip).
  --build-dir PATH          Build directory to use. Default: ./build
  --wayland-explorer-dir    Existing wayland-explorer checkout to update in place.
  --install-deps            Install apt dependencies used by the workflow.
  --install-wlprobe         Install wlprobe with cargo if not present.
  --skip-build              Skip CMake configure/build (expects miral-shell already built).
  --show-diff               Print the full git diff for mir.json when changed.
  --keep-temp-dir           Keep temporary wayland-explorer clone and runtime dir.
  -h, --help                Show this help.

Examples:
  tools/test-update-wayland-explorer.sh --release-tag v2.26.0
  tools/test-update-wayland-explorer.sh --release-tag v2.26.1
  tools/test-update-wayland-explorer.sh --prerelease
  tools/test-update-wayland-explorer.sh --release-tag v2.26.0 --install-deps --install-wlprobe
EOF
}

function cleanup() {
  if [[ -n "${mir_pid}" ]] && kill -0 "${mir_pid}" 2>/dev/null; then
    kill "${mir_pid}" || true
    wait "${mir_pid}" || true
  fi

  if [[ "${keep_temp_dir}" != "true" ]]; then
    if [[ -n "${temp_runtime_dir}" && -d "${temp_runtime_dir}" ]]; then
      rm -rf -- "${temp_runtime_dir}"
    fi
    if [[ -n "${temp_wayland_explorer_dir}" && -d "${temp_wayland_explorer_dir}" ]]; then
      rm -rf -- "${temp_wayland_explorer_dir}"
    fi
  fi
}

trap cleanup EXIT

while [[ $# -gt 0 ]]; do
  case "$1" in
    --release-tag)
      shift
      release_tag="${1:-}"
      ;;
    --prerelease)
      prerelease=true
      ;;
    --build-dir)
      shift
      build_dir="${1:-}"
      ;;
    --wayland-explorer-dir)
      shift
      wayland_explorer_dir="${1:-}"
      ;;
    --install-deps)
      install_deps=true
      ;;
    --install-wlprobe)
      install_wlprobe=true
      ;;
    --skip-build)
      skip_build=true
      ;;
    --show-diff)
      show_diff=true
      ;;
    --keep-temp-dir)
      keep_temp_dir=true
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
  shift
done

if [[ -z "${release_tag}" ]]; then
  project_version="$(awk '
    /^[[:space:]]*VERSION[[:space:]]+[0-9]+\.[0-9]+\.[0-9]+$/ {
      print $2
      exit
    }
  ' "${repo_root}/CMakeLists.txt")"

  if [[ -z "${project_version}" ]]; then
    echo "Failed to derive project VERSION from ${repo_root}/CMakeLists.txt" >&2
    exit 1
  fi

  release_tag="v${project_version}"
fi

echo "Testing release gate with tag: ${release_tag}"

if [[ "${prerelease}" == "true" ]]; then
  echo "SKIP: Release is marked as prerelease"
  exit 0
fi

if [[ ! "${release_tag}" =~ ^v([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
  echo "SKIP: Release tag '${release_tag}' is not in vX.Y.Z format"
  exit 0
fi

major="${BASH_REMATCH[1]}"
minor="${BASH_REMATCH[2]}"
patch="${BASH_REMATCH[3]}"

if [[ "${patch}" != "0" ]]; then
  echo "SKIP: Patch release ${major}.${minor}.${patch} is intentionally ignored"
  exit 0
fi

mir_version_major_minor="${major}.${minor}"
echo "Gate passed. Mir version for JSON: ${mir_version_major_minor}"

if [[ "${install_deps}" == "true" ]]; then
  echo "Installing workflow dependencies via apt..."
  sudo apt-get update
  sudo apt-get install --yes --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    jq \
    git \
    devscripts \
    equivs \
    cargo

  sudo mk-build-deps \
    --install \
    --remove \
    --tool 'apt-get --yes --no-install-recommends' \
    "${repo_root}/debian/control"
fi

for command in cmake git jq; do
  if ! command -v "${command}" >/dev/null 2>&1; then
    echo "Missing required command: ${command}" >&2
    echo "Install dependencies or rerun with --install-deps" >&2
    exit 1
  fi
done

if [[ "${skip_build}" != "true" ]]; then
  echo "Building miral-shell and miral-app in ${build_dir}..."
  cmake -S "${repo_root}" -B "${build_dir}" -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DMIR_ENABLE_TESTS=OFF
  cmake --build "${build_dir}" --target miral-shell miral-app --parallel "$(nproc)"
else
  echo "Skipping build as requested (--skip-build)."
fi

function find_build_output() {
  local requested_build_dir="$1"

  local candidate_bins=(
    "${requested_build_dir}/bin/miral-shell"
    "${requested_build_dir}/bin/miral-shell.bin"
    "${repo_root}/build/bin/miral-shell"
    "${repo_root}/build/bin/miral-shell.bin"
    "${repo_root}/cmake-build-debug/bin/miral-shell"
    "${repo_root}/cmake-build-debug/bin/miral-shell.bin"
    "${repo_root}/cmake-build-release/bin/miral-shell"
    "${repo_root}/cmake-build-release/bin/miral-shell.bin"
  )

  local candidate_bin
  for candidate_bin in "${candidate_bins[@]}"; do
    if [[ -x "${candidate_bin}" ]]; then
      local candidate_build_dir
      candidate_build_dir="$(cd -- "$(dirname -- "${candidate_bin}")/.." && pwd)"
      local candidate_modules="${candidate_build_dir}/lib/server-modules"

      if [[ -d "${candidate_modules}" ]]; then
        echo "${candidate_bin}|${candidate_modules}|${candidate_build_dir}"
        return 0
      fi
    fi
  done

  return 1
}

resolved_build=""
if ! resolved_build="$(find_build_output "${build_dir}")"; then
  echo "Expected executable not found: ${build_dir}/bin/miral-shell" >&2
  echo "Could not find a usable miral-shell + server-modules build output." >&2
  echo "Try one of:" >&2
  echo "  1) Run without --skip-build" >&2
  echo "  2) Pass --build-dir to an existing build tree (for example --build-dir ${repo_root}/cmake-build-debug)" >&2
  echo "  3) Build manually: cmake -S ${repo_root} -B ${repo_root}/build -GNinja && cmake --build ${repo_root}/build --target miral-shell miral-app" >&2
  exit 1
fi

miral_shell_bin="${resolved_build%%|*}"
remaining="${resolved_build#*|}"
server_modules_dir="${remaining%%|*}"
build_dir="${resolved_build##*|}"

echo "Using build directory: ${build_dir}"
echo "Using miral-shell binary: ${miral_shell_bin}"

wlprobe_bin=""
if command -v wlprobe >/dev/null 2>&1; then
  wlprobe_bin="$(command -v wlprobe)"
elif [[ -x "${HOME}/.cargo/bin/wlprobe" ]]; then
  wlprobe_bin="${HOME}/.cargo/bin/wlprobe"
fi

if [[ -z "${wlprobe_bin}" ]]; then
  if [[ "${install_wlprobe}" == "true" ]]; then
    if ! command -v cargo >/dev/null 2>&1; then
      echo "cargo is required to install wlprobe" >&2
      exit 1
    fi

    echo "Installing wlprobe via cargo..."
    cargo install --locked --git https://github.com/PolyMeilex/wlprobe.git wlprobe
    wlprobe_bin="${HOME}/.cargo/bin/wlprobe"
  else
    echo "wlprobe not found" >&2
    echo "Install it first or rerun with --install-wlprobe" >&2
    exit 1
  fi
fi

if [[ -z "${wayland_explorer_dir}" ]]; then
  temp_wayland_explorer_dir="$(mktemp -d /tmp/wayland-explorer.XXXXXX)"
  wayland_explorer_dir="${temp_wayland_explorer_dir}/wayland-explorer"
  echo "Cloning vially/wayland-explorer into ${wayland_explorer_dir}..."
  git clone --depth=1 https://github.com/vially/wayland-explorer.git "${wayland_explorer_dir}"
else
  wayland_explorer_dir="$(cd -- "${wayland_explorer_dir}" && pwd)"
fi

if [[ ! -d "${wayland_explorer_dir}/.git" ]]; then
  echo "Not a git checkout: ${wayland_explorer_dir}" >&2
  exit 1
fi

target_json="${wayland_explorer_dir}/src/data/compositors/mir.json"
if [[ ! -f "${target_json}" ]]; then
  echo "Target file not found: ${target_json}" >&2
  exit 1
fi

temp_runtime_dir="$(mktemp -d /tmp/xdg-runtime.XXXXXX)"
chmod 700 "${temp_runtime_dir}"
wayland_display="wayland-98"
mir_socket="${temp_runtime_dir}/${wayland_display}"
mir_log="/tmp/miral-shell-wayland-explorer.log"

echo "Starting miral-shell..."
XDG_RUNTIME_DIR="${temp_runtime_dir}" \
MIR_SERVER_PLATFORM_PATH="${server_modules_dir}" \
MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:virtual \
MIR_SERVER_VIRTUAL_OUTPUT=1280x1024 \
WAYLAND_DISPLAY="${wayland_display}" \
"${miral_shell_bin}" --add-wayland-extension=all > "${mir_log}" 2>&1 &
mir_pid=$!

for _ in $(seq 1 60); do
  if [[ -S "${mir_socket}" ]]; then
    break
  fi

  if ! kill -0 "${mir_pid}" 2>/dev/null; then
    echo "miral-shell exited before creating ${mir_socket}" >&2
    cat "${mir_log}" >&2
    exit 1
  fi

  sleep 1
done

if [[ ! -S "${mir_socket}" ]]; then
  echo "Timed out waiting for ${mir_socket}" >&2
  cat "${mir_log}" >&2
  exit 1
fi

echo "Generating ${target_json} with wlprobe (${wlprobe_bin})..."
XDG_RUNTIME_DIR="${temp_runtime_dir}" \
WAYLAND_DISPLAY="${wayland_display}" \
"${wlprobe_bin}" --unique \
  | jq --arg version "${mir_version_major_minor}" '{generationTimestamp, version:$version, globals}' \
  > "${target_json}"

echo "Checking for changes in wayland-explorer..."
if git -C "${wayland_explorer_dir}" diff --quiet -- src/data/compositors/mir.json; then
  echo "No changes detected in src/data/compositors/mir.json"
else
  echo "Changes detected in src/data/compositors/mir.json"
  git -C "${wayland_explorer_dir}" --no-pager diff --stat -- src/data/compositors/mir.json

  if [[ "${show_diff}" == "true" ]]; then
    git -C "${wayland_explorer_dir}" --no-pager diff -- src/data/compositors/mir.json
  fi
fi

echo "Done."
echo "Wayland Explorer checkout: ${wayland_explorer_dir}"
echo "Mir log: ${mir_log}"

if [[ "${keep_temp_dir}" == "true" ]]; then
  echo "Temporary runtime directory kept: ${temp_runtime_dir}"
  if [[ -n "${temp_wayland_explorer_dir}" ]]; then
    echo "Temporary wayland-explorer directory kept: ${temp_wayland_explorer_dir}"
  fi
fi
