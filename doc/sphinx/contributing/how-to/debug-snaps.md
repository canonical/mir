(how-to-debug-mir-snaps)=

# How To Debug Mir Snaps

Getting a useful stack trace or debugging session with snaps requires some set up, as they don't contain debugging information.
This document describes the steps required to supply the debugger with the necessary data.

## Debuginfod

The biggest help is provided by Debuginfod, a service that hosts all the debug info stripped from Ubuntu packages.
Debuggers can retrieve those by `build-id` embedded in the executables and libraries on demand.
In modern Ubuntu it's done by default, but to enable it, you just need to export the right environment:

```shell
export DEBUGINFOD_URLS=https://debuginfod.ubuntu.com
```

GDB will then ask you whether you want to use the service, and you can follow its guidance to make this permanent:

```
This GDB supports auto-downloading debuginfo from the following URLs:
  <https://debuginfod.ubuntu.com>
Enable debuginfod for this session? (y or [n]) y
Debuginfod has been enabled.
To make this setting permanent, add 'set debuginfod enabled on' to .gdbinit.
```

To learn more, head to {ref}`server:about-debuginfod` on Ubuntu Server documentation pages.

## Mir CI Snaps

The snaps built for pull requests out of [Mir Continuous Integration pipelines](https://github.com/canonical/mir/blob/main/.github/workflows/snap.yml#L72-L76) are Debug builds,
so you should be set if working with those.

These snaps are published in the `edge/mir-pr<number>` channels for as long as the pull request in question is active.

## PPA-based Snaps

The PPA builds (snaps published outside of branches), on the other hand, strip debug info into separate packages, making it a bit more involved to help GDB find them.

1. create a folder in which you'll hold the debug files and point GDB at it

   ```
   DEBUG_DIR=$HOME/.local/debug
   echo "set debug-file-directory $DEBUG_DIR/usr/lib/debug:/usr/lib/debug/" >> ~/.gdbinit
   mkdir --parents $DEBUG_DIR
   cd $DEBUG_DIR
   ```

1. download the debug packages by either

   - going to the PPA web page and downloading the relevant `-dbgsym` packages, or more conveniently:
   - downloading the packages from the relevant PPA, if you only care about the latest Mir packages
     1. add the PPA source for the `base` of the snap you're looking at:

        ```shell
        PPA=dev
        RELEASE=noble
        sudo add-apt-repository --no-update ppa:mir-team/$PPA
        sed \
          -e "s|^Suites:.*|Suites: $RELEASE|" \
          -e "s|^Components:.*|Components: main/debug|" \
          /etc/apt/sources.list.d/mir-team-ubuntu-$PPA-$( lsb_release -sc ).sources \
          | sudo tee /etc/apt/sources.list.d/mir-team-ubuntu-$PPA-$RELEASE.sources
        sudo apt-get update
        ```

     1. configure the sources' priority down, so they don't get considered for installation

        ```shell
        tee <<EOF > /etc/apt/preferences.d/mir-debug
        Package: *
        Pin: release n=$RELEASE, o=LP-PPA-mir-team-$PPA
        Pin-Priority: -50
        EOF
        ```

     1. download all the dbgsym packages into a folder of choice

        ```shell
        apt download {lib,}mir*-dbgsym/noble
        ```

1. unpack the downloaded packages

   ```shell
   for pkg in *deb; do dpkg-deb --extract $pkg .; done
   ```

## Running Mir Snaps Under GDB

Run the snap with `snap run --gdbserver <snap>` and follow the steps to open a `gdb` session.
To avoid breaking out, ignore the `SIGSTOP`s being sent on startup:

```gdb
handle SIGSTOP nostop nopass
break miral::MirRunner::run_with
continue
# when it breaks out, look at the printed path and point at your local source
set substitute-path /usr/src/mir-<version>/ .
```

Find out more in Snapcraft's {ref}`snapcraft:how-to-debug-with-gdb`.
