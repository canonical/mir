(how-to-debug-mir-snaps)=

# How To Debug Mir Snaps

Getting a useful stack trace or debugging session with snaps requires some set up, as they don't contain debugging information.
This document describes the steps required to supply the debugger with the necessary data.

## Enable Debuginfod

First, we'll make sure you have Debuginfod enabled.
It's a service that hosts the debug info stripped from Ubuntu packages, which debuggers can retrieve by matching the `build-id` embedded in the executables and libraries.
In modern Ubuntu it's installed by default, but enable it in GDB by default with:

```shell
sudo apt install libdebuginfod1t64
# If not already there, this will be added to your environment on next login
export DEBUGINFOD_URLS=https://debuginfod.ubuntu.com
echo "set debuginfod enabled on" >> ~/.gdbinfo
```

To learn more, head to {ref}`server:about-debuginfod` on Ubuntu Server documentation pages.

## Install The Snap

Next, install the snap you want to debug. You have two options:

### Mir CI Snaps

If debugging an issue in a Mir Pull Request, you can install the snap from [Mir Continuous Integration pipelines](https://github.com/canonical/mir/blob/main/.github/workflows/snap.yml#L72-L76).
These are published in the store for as long as the pull request is active - install with, for example:

```shell
snap install mir-test-tools --channel "edge/mir-pr<number>"
```

These are Debug builds, so you can go straight to {ref}`running-mir-snaps-under-gdb` below.

### PPA-based Snaps

The snaps available outside of the `mir-pr<number>` branches come from PPA builds, which strip debug info into separate packages, making it a bit more involved to help GDB find them.

1. best create a folder in which you'll hold the debug files and point GDB at it

   ```
   DEBUG_DIR=$HOME/.local/debug
   echo "set debug-file-directory $DEBUG_DIR/usr/lib/debug:/usr/lib/debug/" >> ~/.gdbinit
   mkdir --parents $DEBUG_DIR
   cd $DEBUG_DIR
   ```

1. next, add the PPA source for the `base` of the snap you're looking at:

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

1. then configure the sources' priority down, so they don't get considered for installation

   ```shell
   tee <<EOF > /etc/apt/preferences.d/mir-debug
   Package: *
   Pin: release n=$RELEASE, o=LP-PPA-mir-team-$PPA
   Pin-Priority: -50
   EOF
   ```

1. and download all the dbgsym packages into a folder of choice

   ```shell
   apt download {lib,}mir*-dbgsym/noble
   ```

1. finally, unpack the downloaded packages

   ```shell
   for pkg in *deb; do dpkg-deb --extract $pkg .; done
   ```

(running-mir-snaps-under-gdb)=

## Running Mir Snaps Under GDB

Next, to get into a GDB session, run the snap with `snap run --gdbserver <snap>` and follow the steps to open a `gdb` prompt.
To avoid breaking out, ignore the `SIGSTOP`s being sent on Mir startup:

```text
handle SIGSTOP nostop nopass
break miral::MirRunner::run_with
continue
# when it breaks out, look at the printed path and point at your local source
set substitute-path /usr/src/mir-<version>/ .
```

You can find out more about debugging snaps in general in Snapcraft's {ref}`snapcraft:how-to-debug-with-gdb`.
