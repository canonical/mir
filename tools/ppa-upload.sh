#!/bin/bash

set -e

MAIN_RE='^(refs/heads/)?main$'  # main branch
VERSION_RE='v(([0-9]+)\.([0-9]+)\.([0-9]+))'  # version format vX.Y.Z (note not matching start/end)
RELEASE_RE="^(refs/heads/)?release/[0-9]+\.[0-9]+\$|^(refs/tags/)?${VERSION_RE}\$"  # release branch or tag
SUFFIX_RE='([0-9]+)-(g[0-9a-f]+)$'

LP_CREDS="$1"
if [ -z "${LP_CREDS}" ]; then
  echo "ERROR: pass the Launchpad credentials file as argument" >&2
  exit 2
fi

if [ -z "${RELEASE}" ]; then
  echo "ERROR: RELEASE environment variable needs to be set to the" >&2
  echo "  target Ubuntu version." >&2
  exit 1
fi

GIT_BRANCH=${GITHUB_REF-$( git rev-parse --abbrev-ref HEAD )}
# determine the patch release
if ! [[ "${GIT_BRANCH}" =~ ${MAIN_RE} ]] && ! [[ "${GIT_BRANCH}" =~ ${RELEASE_RE} ]]; then
  echo "ERROR: This script should only run on main or release tags" >&2
  echo "  or branches." >&2
  exit 3
fi

source <( python <<EOF
import os
from launchpadlib.launchpad import Launchpad

lp = Launchpad.login_anonymously("mir-ci",
                                 "production",
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

GIT_REVISION=$( git rev-parse --short HEAD )

if [[ "${GIT_BRANCH}" =~ ${RELEASE_RE} ]]; then
  # we're on a release branch
  TARGET_PPA=ppa:mir-team/rc
  if [[ "$( git describe --tags --exact-match )" =~ ^${VERSION_RE}$ ]]; then
    # this is a final release, use the tag version
    MIR_VERSION=${BASH_REMATCH[1]}
  else
    # determine the release candidate version string
    if [[ "$( git describe --match="*-rc" )" =~ ^${VERSION_RE}-rc-${SUFFIX_RE} ]]; then
      MIR_VERSION="${BASH_REMATCH[1]}~rc${BASH_REMATCH[2]}"
    else
      echo "ERROR: could not parse git describe output" >&2
      exit 4
    fi
  fi
else
  # look for a release tag within parents 2..n
  PARENT=2
  while git rev-parse HEAD^${PARENT} >/dev/null 2>&1; do
    if [[ "$( git describe --exact-match HEAD^${PARENT} )" =~ ^${VERSION_RE}$ ]]; then
      # copy packages from ppa:mir-team/rc to ppa:mir-team/release_ppa
      RELEASE_VERSION=${BASH_REMATCH[1]}-0ubuntu${UBUNTU_VERSION}
      echo "Copying mir_${RELEASE_VERSION} from ppa:mir-team/rc to ppa:mir-team/release…"
      python - ${RELEASE_VERSION} <<EOF
import os
import sys

from launchpadlib.credentials import (RequestTokenAuthorizationEngine,
                                      UnencryptedFileCredentialStore)
from launchpadlib.launchpad import Launchpad

try:
  lp = Launchpad.login_with(
    "mir-ci",
    "production",
    version="devel",
    authorization_engine=RequestTokenAuthorizationEngine("production",
                                                         "mir-ci"),
    credential_store=UnencryptedFileCredentialStore(
      os.path.expanduser("${LP_CREDS}")
    )
  )
except NotImplementedError:
  raise RuntimeError("Invalid credentials.")

ubuntu = lp.distributions["ubuntu"]
series = ubuntu.getSeries(name_or_version=os.environ['RELEASE'])

mir_team = lp.people["mir-team"]
rc_ppa = mir_team.getPPAByName(name="rc")
release_ppa = mir_team.getPPAByName(name="release")

release_ppa.copyPackage(source_name="mir",
                        version=sys.argv[1],
                        from_archive=rc_ppa,
                        from_series=series.name,
                        from_pocket="Release",
                        to_series=series.name,
                        to_pocket="Release",
                        include_binaries=True)

EOF
      break
    fi
    PARENT=$(( ${PARENT} + 1 ))
  done

  # upload to dev PPA
  TARGET_PPA=ppa:mir-team/dev
  # determine the dev version string
  if [[ "$( git describe --match="*-dev" )" =~ ^${VERSION_RE}-dev-${SUFFIX_RE} ]]; then
    MIR_VERSION="${BASH_REMATCH[1]}~dev${BASH_REMATCH[5]}-${BASH_REMATCH[6]}"
  else
    echo "ERROR: could not parse git describe output" >&2
    exit 5
  fi
fi

PPA_VERSION=${MIR_VERSION}-0ubuntu${UBUNTU_VERSION}

echo "Setting version to:"
echo "  ${PPA_VERSION}"

# unrelease the latest entry to retain release changelog
awk --include=inplace 'BEGIN { getline; print $1, $2, "UNRELEASED;", $4 }; { print }' debian/changelog

debchange \
  --newversion ${PPA_VERSION} \
  --force-bad-version \
  "Automatic build of revision ${GIT_REVISION}"

debchange \
  --release \
  --distribution ${UBUNTU_SERIES} \
  "" # required for debchange to not open an editor

dpkg-buildpackage \
    -I".git" \
    -I"build" \
    -i"^.git|^build" \
    -d -S

dput ${TARGET_PPA} ../mir_${PPA_VERSION}_source.changes
