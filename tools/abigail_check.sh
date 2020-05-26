#!/bin/bash

set -e

declare -A abi_var_for=(\
    ["mircore"]="MIRCORE_ABI" \
    ["mirclient"]="MIRCLIENT_ABI" \
    ["mircommon"]="MIRCOMMON_ABI" \
    ["mirplatform"]="MIRPLATFORM_ABI" \
    ["mirserver"]="MIRSERVER_ABI" \
    ["mircookie"]="MIRCOOKIE_ABI" \
    ["mirplatformgraphicsmesakms"]="MIR_SERVER_GRAPHICS_PLATFORM_ABI" \
    ["mirplatforminputevdev"]="MIR_SERVER_INPUT_PLATFORM_ABI" )

declare -A libtype=(\
    ["mirplatformgraphicsmesakms"]="plugin" \
    ["mirplatforminputevdev"]="plugin" )

declare -A package_name=(\
    ["mirplatformgraphicsmesakms"]="mir-platform-graphics-mesa-kms" \
    ["mirplatforminputevdev"]="mir-platform-input-evdev" )

declare -A libsoname=(\
    ["mirplatformgraphicsmesakms"]="mir/server-platform/graphics-mesa-kms" \
    ["mirplatforminputevdev"]="mir/server-platform/input-evdev" )

declare -A buildsoname=(\
    ["mirplatformgraphicsmesakms"]="server-modules/graphics-mesa-kms" \
    ["mirplatforminputevdev"]="server-modules/input-evdev" )

function print_help_and_exit()
{
    local prog=$(basename $0)

    local all_libs=${!abi_var_for[@]}
    local all_libs=${all_libs// /|}
    echo "Usage: $prog <libname> <abi_dump_dir> <source_dir> <build_dir>"
    echo "Runs a full ABI check on the given library."
    echo ""
    echo "    libname=[${all_libs}]"
    exit 0
}

function get_abi_number()
{
    local abi_var=${1}
    local search_dir=${2}
    grep -hR --include=CMakeLists.txt "set($abi_var [[:digit:]]\+)" ${search_dir}/src | grep -o '[[:digit:]]\+'
}

function is_plugin()
{
    local name=${1}
    if [[ ${libtype[${name}]} == "plugin"* ]];
    then
        return 0
    else
        return 1
    fi
}

function package_abi_for()
{
    local name=${1}
    local pkg_name=lib${name}

    if is_plugin ${name};
    then
        pkg_name=${package_name[${name}]}
    fi

    apt-cache show ${pkg_name}* | grep Package: | grep -o '[[:digit:]]\+' | sort -n -r | head -n 1
}

function needs_abi_check()
{
    local name=${1}
    local source_dir=${2}
    local abi_var=${abi_var_for[${name}]}

    local old_abi=$(package_abi_for ${name})
    local new_abi=$(get_abi_number ${abi_var} ${source_dir})
    if [ -z "${old_abi}" ];
    then
        echo "Failed to find old ${abi_var}" >&2
        exit 1
    fi

    if [ -z "${new_abi}" ];
    then
        echo "Failed to find new ${abi_var}" >&2
        exit 1
    fi
    echo "Detected ${abi_var}_new=${new_abi}"
    echo "Detected ${abi_var}_old=${old_abi}"

    if [ "${old_abi}" == "${new_abi}" ];
    then
        return 0
    else
        return 1
    fi
}

function prepare_pkg_dir()
{
    local pkg_dir=${1}

    # "partial" is required by apt-get
    mkdir -p ${pkg_dir}/archives/partial
}

function package_name_for()
{
    local name=${1}
    local pkg_name=lib${name}

    if is_plugin ${name};
    then
        pkg_name=${package_name[${name}]}
    fi

    local abi_number=$(package_abi_for ${name})
    echo ${pkg_name}${abi_number}
}

function header_packages_for()
{
    local name=${1}
    local pkg=lib${name}-dev

    if is_plugin ${name};
    then
        # Some plugin headers are defined in this package
        pkg="libmirplatform-dev"
    fi

    # Only interested in other mir dev package deps
    local dev_deps=$(apt-cache depends ${pkg} | grep Depends: | grep -o "\<libmir.*-dev\>"|  tr '\n' ' ')

    # Workaround missing dependency
    if [ ${name} == "mirserver" ] || [ ${name} == "mirplatforminputevdev" ];
    then
        dev_deps="${dev_deps} libmirclient-dev"
    fi

    echo "${pkg} ${dev_deps}"
}

function debug_package_for()
{
    local name=${1}
    local so_name=lib${name}
    if is_plugin ${name};
    then
        so_name=${package_name[${name}]}
    fi
    local pkg=${so_name}$(package_abi_for ${name})-dbgsym

    echo ${pkg}
}

function soname_for()
{
    local name=${1}
    local pkg_dir=${2}
    local arch=$(gcc -dumpmachine)
    local abi_number=$(package_abi_for ${name})
    local so_name=lib${name}

    if is_plugin ${name};
    then
        so_name=${libsoname[${name}]}
    fi

    echo ${pkg_dir}/prev-release-pkgs/usr/lib/${arch}/${so_name}.so.${abi_number}
}

function download_packages()
{
    local name=${1}
    local pkg_dir=${2}
    local pkg_name=$(package_name_for ${name})
    local debug_pkgs=$(debug_package_for ${name})

    echo "Downloading: ${pkg_name} ${debug_pkgs} for ${name} to ${pkg_dir}"
    echo "apt-get -d -o Dir::Cache=${pkg_dir} -o Debug::NoLocking=1 install --reinstall -y ${pkg_name} ${debug_pkgs}"
    apt-get -d -o Dir::Cache=${pkg_dir} -o Debug::NoLocking=1 install --reinstall -y ${pkg_name} ${debug_pkgs}
}

function unpack_debs()
{
    local pkg_dir=${1}

    for deb in ${pkg_dir}/archives/* ;
    do
        if [ ! -d ${deb} ] ; then
            echo "unpacking: ${deb}"
            dpkg -x ${deb} ${pkg_dir}
        fi
    done
}

function unpack_prev_release()
{
    local name=${1}
    local abi_dump_dir=${2}
    local pkg_dir=${abi_dump_dir}/prev-release-pkgs

    prepare_pkg_dir ${pkg_dir}
    download_packages ${name} ${pkg_dir}
    unpack_debs ${pkg_dir}
}

function run_abi_check()
{
    local name=${1}
    local unpack_dir=${2}

    local soname=$(soname_for ${name} ${unpack_dir})
    local suppressions="--suppressions ${GENERIC_SUPPRESSIONS}"

    if [ name == "mirclient" ];
    then
        local suppressions="--suppressions ${GENERIC_SUPPRESSIONS} --suppressions ${CLIENT_SUPPRESSIONS}"
    fi

    local new_soname=${BUILD_DIR}/lib/lib${name}.so
    if is_plugin ${name};
    then
        new_soname=${BUILD_DIR}/lib/${buildsoname[${name}]}.so.$(package_abi_for ${name})
    fi

    echo "Running abidiff for ${name}"
    set +e
    abidiff ${suppressions} --debug-info-dir1 ${unpack_dir} ${soname} ${new_soname}
    local result=$?
    set -e

    if [ ${result} == 0 ]
    then
      echo "ABI unchanged"
    elif [ $((${result} & 0x03)) != 0 ]
    then
      echo "ERROR in abidiff command"
      exit 1
    elif [ $((${result} & 0x08)) != 0 ]
    then
      echo "Incompatable ABI change"
      exit 1
    else
      echo "Compatable ABI change (please check report manually)"
    fi
}

if [ $# -ne 4 ];
then
    print_help_and_exit
fi

NAME=${1}
UNPACK_DIR=${2}
SOURCE_DIR=${3}
BUILD_DIR=${4}

GENERIC_SUPPRESSIONS=${SOURCE_DIR}/tools/abigail_suppressions_generic
CLIENT_SUPPRESSIONS=${SOURCE_DIR}/tools/abigail_suppressions_client

if needs_abi_check ${NAME} ${SOURCE_DIR};
then
  unpack_prev_release ${NAME} ${UNPACK_DIR}
  run_abi_check ${NAME} ${UNPACK_DIR}
else
    echo "No need for abi checker, ABI has already been bumped for ${NAME}"
fi
