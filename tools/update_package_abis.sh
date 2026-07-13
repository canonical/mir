#!/bin/sh
# Script to check and update packaging to match current ABIs

set -e

if [ ! -d debian ];
then
    echo "Failed to find debian/ directory. Make sure you run the script from the source tree root." >&2
    exit 1
fi

packages="\
    libmiroil:MIROIL_ABI \
    libmiral:MIRAL_ABI \
    libmircore:MIRCORE_ABI \
    libmircommon:MIRCOMMON_ABI \
    libmirplatform:MIRPLATFORM_ABI \
    libmirserver:MIRSERVER_ABI \
    mir-platform-graphics-x:MIR_SERVER_GRAPHICS_PLATFORM_ABI \
    mir-platform-graphics-atomic-kms:MIR_SERVER_GRAPHICS_PLATFORM_ABI \
    mir-platform-graphics-gbm-kms:MIR_SERVER_GRAPHICS_PLATFORM_ABI \
    mir-platform-input-evdev:MIR_SERVER_INPUT_PLATFORM_ABI\
    libmirwayland:MIRWAYLAND_ABI\
    mir-platform-graphics-wayland:MIR_SERVER_GRAPHICS_PLATFORM_ABI\
    mir-platform-rendering-egl-generic:MIR_SERVER_GRAPHICS_PLATFORM_ABI\
    mir-platform-graphics-virtual:MIR_SERVER_GRAPHICS_PLATFORM_ABI\
    mir-platform-graphics-stub:MIR_SERVER_GRAPHICS_PLATFORM_ABI\
    mir-platform-input-stub:MIR_SERVER_INPUT_PLATFORM_ABI"

rpm_abi_macros="\
    miral_sover:MIRAL_ABI \
    mircommon_sover:MIRCOMMON_ABI \
    mircore_sover:MIRCORE_ABI \
    miroil_sover:MIROIL_ABI \
    mirplatform_sover:MIRPLATFORM_ABI \
    mirserver_sover:MIRSERVER_ABI \
    mirwayland_sover:MIRWAYLAND_ABI \
    mirplatformgraphics_sover:MIR_SERVER_GRAPHICS_PLATFORM_ABI \
    mirplatforminput_sover:MIR_SERVER_INPUT_PLATFORM_ABI"

package_name()
{
    echo "${1%%:*}"
}

package_abi_var()
{
    echo "${1##*:}"
}

print_help_and_exit()
{
    local prog=$(basename $0)

    echo "Usage: $prog [OPTION]..."
    echo "Update packaging to match current ABIs"
    echo ""
    echo "      --no-vcs   do not register file changes (e.g., moves) with the detected VCS"
    echo "      --check    check for ABI mismatches without updating packaging"
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
    for p in $packages;
    do
        local abi_var=$(package_abi_var $p)
        if $(eval "[ -n \"\${$abi_var}\" ]");
        then
            continue;
        fi

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
        local pkg=$(package_name $p)
        local abi_var=$(package_abi_var $p)
        local abi=$(eval "echo \$${abi_var}")
        sed -i "s/${pkg}[[:digit:]]\+/${pkg}$abi/" debian/control
    done
}

update_rpm_spec()
{
    for entry in $rpm_abi_macros;
    do
        local macro=${entry%%:*}
        local abi_var=${entry##*:}
        local abi=$(eval "echo \$${abi_var}")
        sed -i "s/^%global ${macro} [[:digit:]]\{1,\}/%global ${macro} ${abi}/" rpm/mir.spec
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
        local pkg=$(package_name $p)
        local abi_var=$(package_abi_var $p)
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
        local pkg=$(package_name $p)
        local abi_var=$(package_abi_var $p)
        local abi=$(eval "echo \$${abi_var}")
        local result="$(grep -o "${pkg}[[:digit:]]\+" debian/control | sort | uniq | sed -e "/\b${pkg}${abi}\b/ d" | tr '\n' ' ')"
        if [ -n "$result" ];
        then
            report_abi_mismatch "debian/control contains $result, but $pkg ABI is $abi"
        fi
    done
}

check_rpm_spec()
{
    for entry in $rpm_abi_macros;
    do
        local macro=${entry%%:*}
        local abi_var=${entry##*:}
        local abi=$(eval "echo \$${abi_var}")
        local current=$(sed -n "s/^%global ${macro} \([[:digit:]]\{1,\}\)/\1/p" rpm/mir.spec)
        if [ "$current" != "$abi" ];
        then
            report_abi_mismatch "rpm/mir.spec defines ${macro} as ${current}, but ${abi_var} is ${abi}"
        fi
    done
}

check_install_files()
{
    for p in $packages;
    do
        local pkg=$(package_name $p)
        local abi_var=$(package_abi_var $p)
        local abi=$(eval "echo \$${abi_var}")
        local current_file="debian/$(get_current_install_file $pkg)"
        local expected_file="debian/${pkg}${abi}.install"
        local expected_suffix=".so.${abi}"

        if [ "$current_file" != "$expected_file" ];
        then
            report_abi_mismatch "$current_file found, but $pkg ABI is $abi"
        else
            local suffix=$(grep -o ".so.[[:digit:]]\+" $expected_file)
            if [ "$suffix" != "$expected_suffix" ];
            then
                report_abi_mismatch "$current_file contains $suffix, but $pkg ABI is $abi"
            fi
        fi
    done
}

report_unknown_package()
{
    echo "Unknown package: $1" >&2
    has_unknown_packages=yes
}

check_for_unknown_packages()
{
    local control_pkgs="$(grep "Package:" debian/control | cut -d ":" -f 2 | grep "[[:digit:]]$" | tr -d ' [0-9]' | tr '\n' ' ')"
    for p in $control_pkgs;
    do
        local result="$(echo "${packages}" | grep -v "\b${p}:")"
        if [ -n "$result" ];
        then
            report_unknown_package "debian/control contains versioned package ${p} but it is unknown to this script"
        fi
    done

    if [ "$has_unknown_packages" = "yes" ];
    then
        echo "The package list in this script needs to be updated" >&2
        exit 1
    fi
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
    check_for_unknown_packages
    check_control_file
    check_install_files
    check_rpm_spec
    if [ "$has_check_error" = "yes" ];
    then
        log "Consider running tools/update_package_abis.sh to fix the mismatches."
        exit 1
    fi
else
    check_for_unknown_packages
    update_control_file
    update_install_files
    update_rpm_spec
fi
