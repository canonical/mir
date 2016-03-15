#!/bin/bash

set -e

declare -A abi_var_for=(\
    ["mirclient"]="MIRCLIENT_ABI" \
    ["mircommon"]="MIRCOMMON_ABI" \
    ["mirplatform"]="MIRPLATFORM_ABI" \
    ["mirserver"]="MIRSERVER_ABI" \
    ["mircookie"]="MIRCOOKIE_ABI" \
    ["mirclientplatformandroid"]="MIR_CLIENT_PLATFORM_ABI" \
    ["mirclientplatformmesa"]="MIR_CLIENT_PLATFORM_ABI" \
    ["mirplatformgraphicsandroid"]="MIR_SERVER_GRAPHICS_PLATFORM_ABI" \
    ["mirplatformgraphicsmesakms"]="MIR_SERVER_GRAPHICS_PLATFORM_ABI" \
    ["mirplatforminputevdev"]="MIR_SERVER_INPUT_PLATFORM_ABI" )

declare -A libtype=(\
    ["mirclientplatformandroid"]="plugin-client" \
    ["mirclientplatformmesa"]="plugin-client" \
    ["mirplatformgraphicsandroid"]="plugin" \
    ["mirplatformgraphicsmesakms"]="plugin" \
    ["mirplatforminputevdev"]="plugin" )

declare -A package_name=(\
    ["mirclientplatformandroid"]="mir-client-platform-android" \
    ["mirclientplatformmesa"]="mir-client-platform-mesa" \
    ["mirplatformgraphicsandroid"]="mir-platform-graphics-android" \
    ["mirplatformgraphicsmesakms"]="mir-platform-graphics-mesa-kms" \
    ["mirplatforminputevdev"]="mir-platform-input-evdev" )

declare -A libsoname=(\
    ["mirclientplatformandroid"]="mir/client-platform/android" \
    ["mirclientplatformmesa"]="mir/client-platform/mesa" \
    ["mirplatformgraphicsandroid"]="mir/server-platform/graphics-android" \
    ["mirplatformgraphicsmesakms"]="mir/server-platform/graphics-mesa-kms" \
    ["mirplatforminputevdev"]="mir/server-platform/input-evdev" )

declare -A headers_for=(\
    ["mirclientplatformandroid"]="src/include/client/mir/client_platform_factory.h" \
    ["mirclientplatformmesa"]="src/include/client/mir/client_platform_factory.h" \
    ["mirplatformgraphicsandroid"]="mirplatform/mir/graphics/platform.h" \
    ["mirplatformgraphicsmesakms"]="mirplatform/mir/graphics/platform.h" \
    ["mirplatforminputevdev"]="mirplatform/mir/input" )

declare -A exclusions_for=(\
    ["mirplatform"]="mir/input" \
    ["mirserver"]="mir/input/input_device_hub.h" )

function print_help_and_exit()
{
    local prog=$(basename $0)

    local all_libs=${!abi_var_for[@]}
    local all_libs=${all_libs// /|}
    echo "Usage: $prog <libname> <abi_dump_dir> <source_dir>"
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
    mkdir -p ${pkg_dir}/partial
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

    echo ${pkg_dir}/usr/lib/${arch}/${so_name}.so.${abi_number}
}

function download_packages()
{
    local name=${1}
    local pkg_dir=${2}
    local pkg_name=$(package_name_for ${name})
    local header_pkgs=$(header_packages_for ${name})

    echo "Downloading: ${pkg_name} ${header_pkgs} for ${name}"
    apt-get -d -o Dir::Cache=${pkg_dir} -o Debug::NoLocking=1 install --reinstall -y ${pkg_name} ${header_pkgs}
}

function download_mir_source_into()
{
    local pkg_dir=${1}
    pushd ${pkg_dir} > /dev/null

    apt-get source --download-only mir
    mkdir -p mirsource

    local src_pkg=$(apt-cache showsrc mir | grep -o "mir.*orig.tar.gz" | head -n 1)
    tar xf ${src_pkg} -C mirsource --strip-components 1

    popd > /dev/null
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

function make_lib_descriptor()
{
    local name=${1}
    local pkg_dir=${2}
    local descriptor_file=${3}

    local abi_number=$(package_abi_for ${name})
    local lib=$(soname_for ${name} ${pkg_dir})

    local lib_includes=
    for path in $(ls ${pkg_dir}/usr/include);
    do
        lib_includes=${lib_includes}${pkg_dir}/usr/include/${path}"\n"
    done

    local lib_headers=${pkg_dir}/usr/include/${name}
    local lib_skip_headers=
    for path in ${exclusions_for[${name}]};
    do
        lib_skip_headers=${lib_excludes}${pkg_dir}/usr/include/${name}/${path}"\n"
    done

    if [ "${libtype[${name}]}" == "plugin-client" ];
    then
        download_mir_source_into ${pkg_dir}
        lib_headers=${pkg_dir}/mirsource/${headers_for[${name}]}
    elif [ "${libtype[${name}]}" == "plugin" ];
    then
        lib_headers=${pkg_dir}/usr/include/${headers_for[${name}]}
    fi

    cp ${SOURCE_DIR}/tools/lib_descriptor.xml.skel ${descriptor_file}
    sed -i 's@${LIB_DESC_LIBS}@'${lib}'@g' ${descriptor_file}
    sed -i 's@${LIB_DESC_HEADERS}@'${lib_headers}'@g' ${descriptor_file}
    sed -i 's@${LIB_DESC_INCLUDE_PATHS}@'${lib_includes}'@g' ${descriptor_file}
    sed -i 's/${LIB_DESC_GCC_OPTS}/-std=c++14/g' ${descriptor_file}
    sed -i 's@${LIB_DESC_SKIP_HEADERS}@'${lib_skip_headers}'@g' ${descriptor_file}
    sed -i 's/${LIB_DESC_DEFINES}//g' ${descriptor_file}
}

function dump_abi_for_prev_release()
{
    local name=${1}
    local abi_dump_dir=${2}
    local abi_dump=${3}
    local pkg_dir=${abi_dump_dir}/prev-release-pkgs

    if [ ! -s "${abi_dump}" ];
    then
        echo "Generating ${abi_dump}"
        prepare_pkg_dir ${pkg_dir}
        download_packages ${name} ${pkg_dir}
        unpack_debs ${pkg_dir}

        local descriptor_file=${pkg_dir}/${name}_desc.xml
        make_lib_descriptor ${name} ${pkg_dir} ${descriptor_file}

        abi-compliance-checker -gcc-path g++ -l ${name} -v1 previous -dump-path ${abi_dump} -dump-abi ${descriptor_file}
    fi

    if [ ! -s "${abi_dump}" ];
    then
        echo "Error: failed to generate abi dump for ${name}"
        exit 1;
    fi
}

function run_abi_check()
{
    local name=${1}
    local old_dump=${2}
    local new_dump=${3}
    local skip_symbols_file=${4}

    if [ -f ${skip_symbols_file} ];
    then
        local skip_symbols_opt="-skip-symbols ${skip_symbols_file}"
    fi

    echo "Running abi-compliance-checker for ${name}"
    abi-compliance-checker -l ${NAME} -old "${old_dump}" -new "${new_dump}" ${skip_symbols_opt}
}

if [ $# -ne 3 ];
then
    print_help_and_exit
fi

NAME=${1}
ABI_DUMP_DIR=${2}
SOURCE_DIR=${3}

OLD_ABI_DUMP_FILE=${ABI_DUMP_DIR}/${NAME}_prev.abi.tar.gz
NEW_ABI_DUMP_FILE=${ABI_DUMP_DIR}/${NAME}_next.abi.tar.gz
SKIP_SYMBOLS_FILE=${SOURCE_DIR}/tools/abi-check-${NAME}-skip-symbols

if needs_abi_check ${NAME} ${SOURCE_DIR};
then
    dump_abi_for_prev_release ${NAME} ${ABI_DUMP_DIR} ${OLD_ABI_DUMP_FILE}
    run_abi_check ${NAME} ${OLD_ABI_DUMP_FILE} ${NEW_ABI_DUMP_FILE} ${SKIP_SYMBOLS_FILE}
else
    echo "No need for abi-compliance-checker, ABI has already been bumped for ${NAME}"
fi
