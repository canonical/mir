---
name: update-ci-distro-releases
description: Update spread and GitHub Actions distro/runner versions to current supported Ubuntu, Fedora, and Alpine releases; remove EOL stable releases by default, preserve existing development tracks, optionally keep only latest stable per distro, and refresh GitHub runner images.
---

# Update CI Distro Releases

Use this workflow when asked to refresh distro versions in spread and GitHub Actions configuration.

Authoritative sources:

- Ubuntu release support and lifecycle: https://ubuntu.com/about/release-cycle
- Fedora release history (for current and EOL): https://en.wikipedia.org/wiki/Fedora_Linux_release_history
- Alpine releases and support windows: https://alpinelinux.org/releases/
- GitHub Actions runner images and deprecations: https://github.com/actions/runner-images

## Behavior

Default behavior:

1. Remove unsupported/EOL distro releases.
1. Keep all still-supported releases.
1. Add newly supported releases to the version lists/matrices.
1. Preserve existing development images/tracks (for example `edge`, `rawhide`, `devel`)

Optional behavior when explicitly requested:

1. Keep only the latest supported stable release for each distro.
1. Where applicable, replace the default release with the latest supported stable release.

## Files to inspect and update

- `spread.yaml`
- `.github/workflows/spread.yml`
- Any workflow under `.github/workflows/*.yml` with distro version literals or explicit runner image pins.

Typical fields:

- spread backend systems list (`backends.lxd.systems`)
- spread environment release aliases (`RELEASE/...`)
- unversioned spread release aliases (for example `RELEASE/fedora_clang`) aligned to the latest supported stable release for their distro track
- `RELEASE` and `RELEASE/ubuntu` are special-case Ubuntu aliases: update only in latest-only mode, and only to the latest supported Ubuntu LTS codename
- spread GitHub Actions matrix entries (for example `lxd:alpine-...`, `lxd:fedora_...`)
- workflow `runs-on` image pins (for example `ubuntu-24.04`, `ubuntu-22.04`, `ubuntu-latest`)

Ubuntu-specific (especially in latest-only mode):

- stable Ubuntu system image in `backends.lxd.systems` (for example `ubuntu-24.04` -> `ubuntu-26.04`)
- default Ubuntu release aliases (`RELEASE` and `RELEASE/ubuntu`) to the latest supported Ubuntu LTS codename
- remove superseded interim aliases from `RELEASE/...` when latest-only is requested (for example remove `RELEASE/ubuntu_questing` once `RELEASE/ubuntu_resolute` is the selected stable baseline)
- remove matching superseded spread matrix tasks under `.github/workflows/spread.yml` (for example `lxd:...:ubuntu_questing`)

## Update procedure

1. Determine current supported stable releases for Ubuntu, Fedora, and Alpine from the sources above.
1. Choose target versions per distro:
   - Default mode: keep all supported stable releases.
   - Latest-only mode: keep only the newest supported stable release.
1. Update the fields listed above:
   - Add newly supported versions and remove EOL versions.
   - Preserve existing development tracks (`edge`, `rawhide`, `devel`).
   - Keep naming conventions consistent with the existing config.
   - Update unversioned stable release aliases to match the newest supported stable release for their distro track (for example keep `RELEASE/fedora_clang` aligned with the newest supported stable Fedora release kept in `RELEASE/fedora_<version>`).
   - Do not update `RELEASE` or `RELEASE/ubuntu` in default mode.
   - In Ubuntu latest-only mode, set `RELEASE` and `RELEASE/ubuntu` to the latest supported Ubuntu LTS codename, keep only the latest stable Ubuntu image in `backends.lxd.systems`, and remove superseded interim aliases/tasks.
1. If runner images are in scope:
   - Update explicit pinned `runs-on` images only when repo policy or prompt requires it.
   - Only change `runs-on` to labels that are documented as available GitHub-hosted runner images in `actions/runner-images`.
   - Do not infer a new `runs-on` label from distro upgrades in spread config; if no matching GitHub-hosted image exists, keep the current pin (or `ubuntu-latest`) and note the constraint.
   - Leave `ubuntu-latest` unchanged unless explicitly requested.
1. When runner updates are touched, include this user prompt verbatim:
   - `Please verify whether any self-hosted runners also need OS/image updates to match these CI changes.`

## Validation checklist

- No remaining references to removed/EOL distro versions in `spread.yaml` or `.github/workflows/*.yml`.
- Spread matrix entries align with `backends.lxd.systems` and `RELEASE/...` aliases.
- Unversioned stable release aliases (excluding `RELEASE` and `RELEASE/ubuntu`) match the selected latest supported stable release for their distro track.
- Existing development images/tracks are preserved.
- If latest-only mode was requested, exactly one stable version remains per distro.
- If latest-only mode was requested for Ubuntu, `RELEASE` and `RELEASE/ubuntu` match the latest supported Ubuntu LTS codename and superseded interim aliases/tasks are removed.
- YAML syntax remains valid (validate with `yq`).

Recommended validation command:

```bash
yq '.' spread.yaml >/dev/null && yq '.' .github/workflows/spread.yml >/dev/null
```

## Suggested search commands

```bash
rg -n "alpine-|fedora-|ubuntu-|rawhide|edge|devel|runs-on:" spread.yaml .github/workflows/*.yml
```

## Rules

- Prefer minimal, targeted edits.
- Do not silently change release-policy intent; follow default mode unless the prompt explicitly requests latest-only mode.
- Never update workflow `runs-on` to an unverified or unavailable image label.
- If upstream support status is ambiguous, state assumptions explicitly before finalizing.
