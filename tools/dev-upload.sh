#!/bin/bash

set -e

if [ -z "${RELEASE}" ]; then
  echo "ERROR: RELEASE environment variable needs to be set to the" >&2
  echo "  target Ubuntu version." >&2
  exit 1
fi

MIR_VERSION=$( dpkg-parsechangelog --show-field Version \
               | perl -n -e '/(^[^~-]+)/ && print $1' )
GIT_COMMITS=$( git rev-list --count HEAD )
GIT_REVISION=$( git rev-parse --short HEAD )

source <( python <<EOF
import os
from launchpadlib.launchpad import Launchpad

lp = Launchpad.login_anonymously("mir-ci",
                                 "production",
                                 "/tmp/lplib",
                                 version="devel")

ubuntu = lp.distributions["ubuntu"]
series = ubuntu.getSeries(name_or_version=os.environ['RELEASE'])

print("UBUNTU_SERIES={}".format(series.name))
print("UBUNTU_VERSION={}".format(series.version))
EOF
)

if [ -z "${UBUNTU_SERIES}" -o -z "${UBUNTU_VERSION}" ]; then
  echo "ERROR: \"${RELEASE}\" was not recognized as a valid Ubuntu series" >&2
  exit 2
fi

DEV_VERSION=${MIR_VERSION}~dev-${GIT_COMMITS}-g${GIT_REVISION}~ubuntu${UBUNTU_VERSION}

echo "Setting version to:"
echo "  ${DEV_VERSION}"

debchange \
  --newversion ${DEV_VERSION} \
  --force-bad-version \
  "Automatic build of revision ${GIT_REVISION}"

debchange \
  --release \
  --distribution ${UBUNTU_SERIES} \
  "Build for Ubuntu ${UBUNTU_VERSION}"

dpkg-buildpackage \
    -I".git" \
    -I"build" \
    -i"^.git|^build" \
    -d -S

dput ppa:mir-team/dev ../mir_${DEV_VERSION}_source.changes
