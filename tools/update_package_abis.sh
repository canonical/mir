#!/bin/sh
# Script to check and update debian packaging to match current ABIs

set -e

if [ ! -d debian ];
then
    echo "Failed to find debian/ directory. Make sure you run the script from the source tree root." >&2
    exit 1
fi

abi_vars="\
    MIRCLIENT_ABI \
    MIRCLIENT_DEBUG_EXTENSION_ABI \
    MIR_CLIENT_PLATFORM_ABI \
    MIRCOMMON_ABI \
    MIRPLATFORM_ABI \
    MIRPROTOBUF_ABI \
    MIRSERVER_ABI \
    MIR_SERVER_GRAPHICS_PLATFORM_ABI"

packages="\
    libmirclient:MIRCLIENT_ABI \
    libmirclient-debug-extension:MIRCLIENT_DEBUG_EXTENSION_ABI \
    libmircommon:MIRCOMMON_ABI \
    libmirplatform:MIRPLATFORM_ABI \
    libmirprotobuf:MIRPROTOBUF_ABI \
    libmirserver:MIRSERVER_ABI \
    mir-client-platform-android:MIR_CLIENT_PLATFORM_ABI \
    mir-client-platform-mesa:MIR_CLIENT_PLATFORM_ABI \
    mir-platform-graphics-android:MIR_SERVER_GRAPHICS_PLATFORM_ABI \
    mir-platform-graphics-mesa:MIR_SERVER_GRAPHICS_PLATFORM_ABI"

print_help_and_exit()
{
    local prog=$(basename $0)

    echo "Usage: $prog [OPTION]..."
    echo "Update debian packaging to match current ABIs"
    echo ""
    echo "      --no-vcs   do not register file changes (e.g., moves) with the detected VCS"
    echo "      --check    check for ABI mismatches without updating debian packaging"
    echo "                 exit status = 0: no mismatches, 1: mismatches detected"
    echo "                 use --verbose to get more information about the mismatches"
    echo "  -v, --verbose  enable verbore output"
    echo "  -h, --help     display this help and exit"

    exit 0
}

log()
{
    if [ "$option_verbose" = "yes" ];
    then
        echo $@
    fi
}

get_vcs()
{
    if [ "$option_vcs" = "yes" ];
    then
        if [ -d .bzr ];
        then
            echo "bzr"
        elif [ -d .git ];
        then
            echo "git"
        else
            echo "Failed to detect VCS" >&2
            exit 1
        fi
    fi
}

get_abi_number()
{
    local abi_var=$1
    grep -hR --include=CMakeLists.txt "set($abi_var [[:digit:]]\+)" src/ | grep -o '[[:digit:]]\+'
}

populate_abi_variables()
{
    for abi_var in $abi_vars;
    do
        local abi=$(get_abi_number $abi_var)
        if [ -z "$abi" ];
        then
            echo "Failed to find ABI number for $abi_var" >&2
            exit 1
        else
            log "Detected $abi_var=$abi"
        fi
        eval "$abi_var=$abi"
    done
}

update_control_file()
{
    for p in $packages;
    do
        local pkg=${p%%:*}
        local abi_var=${p##*:}
        local abi=$(eval "echo \$${abi_var}")
        sed -i "s/${pkg}[[:digit:]]\+/${pkg}$abi/" debian/control
    done
}

get_current_install_file()
{
    local pkg=$1
    ls -1 debian/${pkg}*.install | grep -o "${pkg}[[:digit:]]\+.install"
}

move_file()
{
    local src=$1
    local dst=$2
    local vcs=$(get_vcs)

    log "Renaming $src => $dst"

    case $vcs in
        "bzr") bzr mv $1 $2 ;;
        "git") git mv $1 $2 ;;
        *) mv $1 $2 ;;
    esac
}

update_install_files()
{
    for p in $packages;
    do
        local pkg=${p%%:*}
        local abi_var=${p##*:}
        local abi=$(eval "echo \$${abi_var}")
        local current_file="debian/$(get_current_install_file $pkg)"
        local new_file="debian/${pkg}${abi}.install"

        if [ "$current_file" != "$new_file" ];
        then
            move_file $current_file $new_file
        fi
        sed -i "s/.so.[[:digit:]]\+/.so.${abi}/" debian/${pkg}${abi}.install
    done
}

report_abi_mismatch()
{
    log "ABI mismatch: $1"
    has_check_error=yes
}

check_control_file()
{
    for p in $packages;
    do
        local pkg=${p%%:*}
        local abi_var=${p##*:}
        local abi=$(eval "echo \$${abi_var}")
        local result=$(grep -o "${pkg}[[:digit:]]\+" debian/control | uniq | sed -e "/\b${pkg}${abi}\b/ d" | tr '\n' ' ')
        if [ -n "$result" ];
        then
            report_abi_mismatch "debian/control contains $result, but $pkg ABI is $abi"
        fi
    done
}

check_install_files()
{
    for p in $packages;
    do
        local pkg=${p%%:*}
        local abi_var=${p##*:}
        local abi=$(eval "echo \$${abi_var}")
        local current_file=debian/$(get_current_install_file $pkg)
        local expected_file="debian/${pkg}${abi}.install"
        local expected_suffix=".so.${abi}"

        if [ "$current_file" != "$expected_file" ];
        then
            report_abi_mismatch "debian/$current_file found, but $pkg ABI is $abi"
        else
            local suffix=$(grep -o ".so.[[:digit:]]\+" $expected_file)
            if [ "$suffix" != "$expected_suffix" ];
            then
                report_abi_mismatch "$current_file contains $suffix, but $pkg ABI is $abi"
            fi
        fi
    done
}

option_vcs=yes
option_verbose=no
option_check=no

while [ $# -gt 0 ];
do
    case "$1" in
        --no-vcs) option_vcs=no;;
        --check) option_check=yes;;
        --verbose|-v) option_verbose=yes;;
        --help|-h) print_help_and_exit;;
        --) shift; break;;
    esac
    shift
done

populate_abi_variables

if [ "$option_check" = "yes" ];
then
    check_control_file
    check_install_files
    if [ "$has_check_error" = "yes" ];
    then
        log "Consider running tools/update_package_abis.sh to fix the mismatches."
        exit 1
    fi
else
    update_control_file
    update_install_files
fi
