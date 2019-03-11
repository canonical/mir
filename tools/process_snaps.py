#!/usr/bin/env python3
# coding: utf-8

import json
import logging
import os
import re
import sys

from launchpadlib import errors as lp_errors  # fades
from launchpadlib.credentials import (RequestTokenAuthorizationEngine,
                                      UnencryptedFileCredentialStore)
from launchpadlib.launchpad import Launchpad
import requests  # fades


logger = logging.getLogger("mir.process_snaps")
logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.INFO)

APPLICATION = "mir-ci"
LAUNCHPAD = "production"
RELEASE = "bionic"
TEAM = "mir-team"
SOURCE_NAME = "mir"
SNAPS = {
    "mir-kiosk": {
        "edge": ("dev", "mir-kiosk-edge"),
        "candidate": ("rc", "mir-kiosk-candidate"),
    },
}


PENDING_BUILD = (
    "Needs building",
    "Dependency wait",
    "Currently building",
    "Uploading build",
)

MIR_VERSION_RE = re.compile(r"^(.*)-[^-]+$")
SNAP_VERSION_RE = re.compile(r"^(?:(?P<server>.+)-mir)?"
                             r"(?P<mir>.+?)"
                             r"(?:-snap(?P<snap>.+))?$")

STORE_URL = ("https://api.snapcraft.io/api/v1/snaps"
             "/details/{snap}?channel={channel}")
STORE_HEADERS = {
    "X-Ubuntu-Series": "16",
    "X-Ubuntu-Architecture": "{arch}"
}


def check_store_version(processor, snap, channel):
    logger.debug("Checking version for: %s", processor)
    data = {
        "snap": snap,
        "channel": channel,
        "arch": processor,
    }
    resp = requests.get(
        STORE_URL.format(**data),
        headers={k: v.format(**data) for k, v in STORE_HEADERS.items()}
    )
    logger.debug("Got store response: %s", resp)

    try:
        result = json.loads(resp.content)
    except json.JSONDecodeError:
        logger.error("Could not parse store response: %s",
                     resp.content)
        return

    try:
        return result["version"]
    except KeyError:
        logger.debug("Could not find version for %s (%s): %s",
                     snap, processor, result["error_list"])
    return None


if __name__ == '__main__':
    try:
        lp = Launchpad.login_with(
            APPLICATION,
            LAUNCHPAD,
            version="devel",
            authorization_engine=RequestTokenAuthorizationEngine(LAUNCHPAD,
                                                                 APPLICATION),
            credential_store=UnencryptedFileCredentialStore(
                os.path.expanduser("~/.launchpadlib/credentials"))
        )
    except NotImplementedError:
        raise RuntimeError("Invalid credentials.")

    ubuntu = lp.distributions["ubuntu"]
    logger.debug("Got ubuntu: %s", ubuntu)

    series = ubuntu.getSeries(name_or_version=RELEASE)
    logger.debug("Got series: %s", series)

    team = lp.people[TEAM]
    logger.debug("Got team: %s", team)

    errors = []

    for snap, channels in SNAPS.items():
        for channel, snap_map in channels.items():
            logger.info("Processing channel %s for snap %s…", channel, snap)

            ppa = team.getPPAByName(name=snap_map[0])
            logger.debug("Got ppa: %s", ppa)

            try:
                snap_recipe = lp.snaps.getByName(owner=team, name=snap_map[1])
                logger.debug("Got snap: %s", snap_recipe)
            except lp_errors.NotFound as ex:
                logger.error("Snap not found: %s", snap_map[1])
                errors.append(ex)
                continue

            latest_source = ppa.getPublishedSources(
                source_name=SOURCE_NAME,
                distro_series=series
            )[0]
            logger.debug("Latest source: %s", latest_source.display_name)

            version = (
                MIR_VERSION_RE.match(latest_source.source_package_version)[1]
            )
            logger.debug("Parsed upstream version: %s", version)

            store_versions = {
                processor.name: check_store_version(processor.name,
                                                    snap,
                                                    channel)
                for processor in snap_recipe.processors
            }

            logger.debug("Got store versions: %s", store_versions)

            if all(version.startswith(SNAP_VERSION_RE.match(store_version)
                                      .group("mir"))
                   for store_version in store_versions.values()):
                logger.info("Skipping %s: store versions are current",
                            latest_source.display_name)
                continue

            if latest_source.status != "Published":
                logger.info("Skipping %s: %s",
                            latest_source.display_name, latest_source.status)
                continue

            if any(build.buildstate in PENDING_BUILD
                   for build in latest_source.getBuilds()):
                logger.info("Skipping %s: builds pending…",
                            latest_source.display_name)
                continue

            if any(binary.status != "Published"
                   for binary in latest_source.getPublishedBinaries()):
                logger.info("Skipping %s: binaries pending…",
                            latest_source.display_name)
                continue

            if len(snap_recipe.pending_builds) > 0:
                logger.info("Skipping %s: snap builds pending…",
                            snap_recipe.web_link)
                continue

            logger.info("Triggering for %s…", latest_source.display_name)

            builds = snap_recipe.requestAutoBuilds()
            logger.debug("Triggered builds: %s", snap_recipe.web_link)

    for error in errors:
        logger.debug(error)

    if errors:
        sys.exit(1)
