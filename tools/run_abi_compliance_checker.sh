#!/bin/bash

declare -A abi_var_for
abi_var_for=( ["mirclient"]="MIRCLIENT_ABI" \
    ["mircommon"]="MIRCOMMON_ABI" \
    ["mirplatform"]="MIRPLATFORM_ABI" \
    ["mirprotobuf"]="MIRPROTOBUF_ABI" \
    ["mirserver"]="MIRSERVER_ABI" \
    ["mirclientplatformandroid"]="MIR_CLIENT_PLATFORM_ABI" \
    ["mirclientplatformmesa"]="MIR_CLIENT_PLATFORM_ABI" \
    ["mirplatformgraphicsandroid"]="MIR_SERVER_GRAPHICS_PLATFORM_ABI" \
    ["mirplatformgraphicsmesakms"]="MIR_SERVER_GRAPHICS_PLATFORM_ABI" )

print_help_and_exit()
{
    local prog=$(basename $0)

    local all_libs=${!abi_var_for[@]}
    local all_libs=${all_libs// /|}
    echo "Usage: $prog <libname> <old_release_dir> <new_release_dir> <old_abi_dump> <new_abi_dump>"
    echo "Runs a full ABI check if the ABI number has not been changed"
    echo ""
    echo "    libname=[${all_libs}]"
    exit 0
}

get_abi_number()
{
    local abi_var=${1}
    local search_dir=${2}
    grep -hR --include=CMakeLists.txt "set($abi_var [[:digit:]]\+)" ${search_dir}/src | grep -o '[[:digit:]]\+'
}

need_abi_check()
{
    local libname=${1}
    local old_release_dir=${2}
    local next_release_dir=${3}
    local abi_var=${abi_var_for[${libname}]}

    local old_abi=$(get_abi_number ${abi_var} ${old_release_dir})
    local new_abi=$(get_abi_number ${abi_var} ${next_release_dir})
    if [ -z "${old_abi}" ];
    then
        old_abi=0
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

if [ $# -ne 5 ];
then
    print_help_and_exit
fi

LIB_NAME=${1}
OLD_RELEASE_DIR=${2}
NEXT_RELEASE_DIR=${3}
OLD_ABI_DUMP=${4}
NEW_ABI_DUMP=${5}

SKIP_SYMBOLS_FILE=${NEXT_RELEASE_DIR}/tools/abi-check-${LIB_NAME}-skip-symbols
if [ -f ${SKIP_SYMBOLS_FILE} ];
then
SKIP_SYMBOLS_OPT="-skip-symbols ${SKIP_SYMBOLS_FILE}"
fi

if need_abi_check ${LIB_NAME} ${OLD_RELEASE_DIR} ${NEXT_RELEASE_DIR};
then
    if [ ! -f "${OLD_ABI_DUMP}" ];
    then
        echo "Error: No base abi dump exists for ${LIB_NAME}"
        exit 1;
    else
        echo "Running abi-compliance-checker for ${LIB_NAME}"
        abi-compliance-checker -l ${LIB_NAME} -old "${OLD_ABI_DUMP}" -new "${NEW_ABI_DUMP}" -check-implementation ${SKIP_SYMBOLS_OPT}
    fi
else
    echo "No need for abi-compliance-checker, ABI has already been bumped for ${LIB_NAME}"
fi
