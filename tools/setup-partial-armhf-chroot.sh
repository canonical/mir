#!/bin/bash

set -e

if [ -z ${1} ]; then
    echo "please supply directory to create partial chroot in. (eg, ./setup-partial-armhf-chroot.sh mychroot-dir)"
    exit
fi

echo "creating phablet-compatible armhf partial chroot for mir compilation in directory ${1}"

if [ ! -d ${1} ]; then
    mkdir -p ${1} 
fi

# Default to vivid as we don't seem to have any working wily devices right now.
# Also Jenkins expects this script to default to vivid (TODO: update CI?)
dist=vivid
if [ ! -z "$2" ]; then
    dist=$2
fi

DEBCONTROL=$(pwd)/../debian/control

pushd ${1} > /dev/null

# Empty dpkg status file, so that ALL dependencies are listed with dpkg-checkbuilddeps
echo "" > status

# Manual error code checking is needed for dpkg-checkbuilddeps
set +e

# Parse dependencies from debian/control
# dpkg-checkbuilddeps returns 1 when dependencies are not met and the list is sent to stderr
builddeps=$(dpkg-checkbuilddeps -a armhf --admindir=. ${DEBCONTROL} 2>&1 )
if [ $? -ne 1 ] ; then
    echo "${builddeps}"
    exit 2
fi

# now turn exit on error option
set -e

# Sanitize dependencies list for submission to debootstrap
# build-essential is not needed as we are cross-compiling
builddeps=$(echo ${builddeps} | sed -e 's/dpkg-checkbuilddeps://g' -e 's/Unmet build dependencies://g' -e 's/build-essential:native//g')
builddeps=$(echo ${builddeps} | sed 's/([^)]*)//g')
builddeps=$(echo ${builddeps} | sed 's/ /,/g')
builddeps=$(echo ${builddeps} | sed -e 's/abi-compliance-checker//g')

fakeroot debootstrap --include=${builddeps} --arch=armhf --download-only --variant=buildd ${dist} .

# Remove libc libraries that confuse the cross-compiler
rm var/cache/apt/archives/libc-dev*.deb
rm var/cache/apt/archives/libc6*.deb

for deb in var/cache/apt/archives/* ; do
    if [ ! -d ${deb} ] ; then
        echo "unpacking: ${deb}"
        dpkg -x ${deb} .
    fi
done

# Fix up symlinks which asssumed the usual root path
for broken_symlink in $(find . -name \*.so -type l -xtype l) ; do
    ln -sf $(pwd)$(readlink ${broken_symlink}) ${broken_symlink}
done

popd > /dev/null 
