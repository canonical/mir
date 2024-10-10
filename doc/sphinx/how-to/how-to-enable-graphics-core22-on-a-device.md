# How to enable graphics-core22 on a device
This document will guide you through the process of assembling a 
`graphics-core22` snap. With this snap, your device will be ready
to run graphical snaps like `ubuntu-frame` or any other *Mir*-based
compositor in a snapped environment.

To this end, we will accomplish the following in order:
- How to assemble a `graphics-core22` provider snap
- How to test a `graphics-core22` provider snap
- How to test the confinement of your *Mir*-based snap that relies on the provider snap

## Prerequisites
Before we get started, let's make sure that we have everything that we require.

### 1. Development Board
For starters, make sure that you have some sort of development board on hand.

### 2. GPU Support Check
Next, make sure that this board runs a `snapd`-enabled Ubuntu image with
a kernel that includes GPU support. To do this, we'll run a series of checks.

First, you should ensure that *at least* `/dev/dri/card0` exists, but you 
may have more depending on your setup. You may check this from the command line.

Next, install the [graphics-test-tools](https://snapcraft.io/graphics-test-tools) snap:

```shell
sudo snap install graphics-test-tools --channel 22/stable
```

This snap will provide you with `graphics-test-tools.drm-info`. Run it and you
should see something like the following:
```
Node: /dev/dri/card1
├───Driver: amdgpu (AMD GPU) version 3.54.0 (20150101)
│   ├─── ...
├───Device: PCI 1002:747e Advanced Micro Devices, Inc. [AMD/ATI] Device 747e
│   └───Available nodes: primary, render
├───Framebuffer size
│   ├───Width: [0, 16384]
│   └───Height: [0, 16384]
├───Connectors
|   |   ...
│   ├───Connector 3
│   ├───Object ID: 101
│   ├───Type: HDMI-A
│   ├───Status: connected
│   ├───Physical size: 530x300 mm
│   ├───Subpixel: unknown
│   ├───Encoders: {3}
│   ├───Modes
│   │   ├───1920x1080@60.00 preferred driver phsync pvsync 
│   │   ├───1920x1080@60.00 driver phsync pvsync 16:9 
│   │   ├───...
|   |   ...
├───Encoders
│   ├───Encoder 0
│   │   ├───Object ID: 82
│   │   ├───Type: TMDS
│   │   ├───CRTCS: {0, 1, 2, 3}
│   │   └───Clones: {0}
|   |   ...
├───CRTCs
│   ├───CRTC 0
│   │   ├───Object ID: 72
│   │   ├───Legacy info
│   │   │   ├───Mode: 1920x1080@60.00 preferred driver phsync pvsync 
│   │   │   └───Gamma size: 256
|   |   ...
└───Planes
    ├───Plane 0
    │   ├───Object ID: 40
    │   ├───CRTCs: {3}
    │   ├───Legacy info
    │   │   ├───FB ID: 0
    |   ...
```
You want to make sure that you have:
- 1 Node
- 1 or more Connector(s)
- 1 or more CRTC(s)
- 1 or more Plane(s)

Finally, If you have a display connected, then one of your `Connector`s should read `Status: connected`. This `Connector` will also list many `Modes` that
the  display can use.

#### Troubleshooting
If `drm-info` does *not* produce results similar to those above, then this
indicates that your kernel **is NOT yet ready** to support the GPU. This
means that kernel enablement is required before Ubuntu Frame can be enabled.

### 3. Driver Support Check
Finally, we will need to check that you have the proper userspace drivers
installed on the system, namely:
- `EGL` (`libEGL.so.1`)
- `GLES` (`libGLESv2.so.2`) 
- `GBM` (`libgbm.so.1`)

While *Mir* can be enabled on platforms that do not support `GBM/KMS`, this
requires a paltform implementation to be written in *Mir*. This work is out
of the scope of this guide.

## How to assemble a `graphics-core22` provider snap
With the prerequisites out of the way, it is time to assemble a `graphics-core22` snap.

The snap itself must provide the libraries described in 
[this document](https://mir-server.io/docs/the-graphics-core22-snap-interface#heading--lists--supported-apis).
Unless you are incredibly space-constrained or in an unusual circumstance, it is a good idea to
provide **all** of these libraries. Even if your board doesn’t support these libraries, a failure to include
them will result in applications that would otherwise fall back to a supported API failing to start.

`libEGL`, `libGLESv2` and `libgbm` should be provided by the vendor's drivers.
`libvulkan` *may* be provided by the vendor as well.
The remaining packages should be available in the Ubuntu archive.

Most of these libraries do **NOT** need to be in a fixed location in the snap.
However, the following paths **MUST** be provided in a fixed location by your snap:

1. `libdrm`:
    ```yaml
    parts:
      drm:
        # DRM userspace
        ...
        organize:
          # Expected at /libdrm by the `graphics-core22` interface
          usr/share/libdrm: libdrm
        ...
    ```


2. `drirc.d` (optional, if your vendor drivers supply `drirc` configuration):
    ```yaml
    parts:
      dri:
        # Userspace drivers
        ...
        organize:
          # Expected at /drirc.d by the `graphics-core22` interface
          usr/share/drirc.d: drirc.d
        ...
    ```


3. `X11`:
    ```yaml
    parts:
      x11:
        ...
        organize:
          # Expected at /X11 by the `graphics-core22` interface
          usr/share/X11: X11
        ...
    ```

The remainder of the paths will be established using the required script at the location
`bin/graphics-core22-provider-wrapper`. This script will be invoked with 
`graphics-core22-provider-wrapper $EXECUTABLE $ARGS...`. This script does the 
following:
1. It exports any environment variables that are required for the binary 
to find and use the vendor drivers
2. It then executes the provided `$EXECUTABLE` with `$ARGS…`.

An example of a script that provides `mesa-core22` is as follows:

```sh
# Dervied from:
# https://github.com/canonical/mesa-core22/blob/main/scripts/bin/graphics-core22-provider-wrapper.in


#!/bin/bash
set -euo pipefail


# Find the parent directory of the wrapper script. This will be
# the path of the provider snap in the client. Your files with
# be found at a path relative to this path.
SELF="$( cd -- "$(dirname "$0")/.." ; pwd -P )/usr"


# The arch triplet(s) for this driver (eg: arm64-linux-gnu, i386-linux-gnu, etc)
ARCH_TRIPLETS=( x86_64-linux-gnu )


# VDPAU_DRIVER_PATH only supports a single path, rely on LD_LIBRARY_PATH instead
for arch in ${ARCH_TRIPLETS[@]}; do
  LD_LIBRARY_PATH=${LD_LIBRARY_PATH:+$LD_LIBRARY_PATH:}${SELF}/lib/${arch}:${SELF}/lib/${arch}/vdpau
  LIBGL_DRIVERS_PATH=${LIBGL_DRIVERS_PATH:+$LIBGL_DRIVERS_PATH:}${SELF}/lib/${arch}/dri/
  LIBVA_DRIVERS_PATH=${LIBVA_DRIVERS_PATH:+$LIBVA_DRIVERS_PATH:}${SELF}/lib/${arch}/dri/
done

__EGL_VENDOR_LIBRARY_DIRS=${__EGL_VENDOR_LIBRARY_DIRS:+$__EGL_VENDOR_LIBRARY_DIRS:}${SELF}/share/glvnd/egl_vendor.d
__EGL_EXTERNAL_PLATFORM_CONFIG_DIRS=${__EGL_EXTERNAL_PLATFORM_CONFIG_DIRS:+$__EGL_EXTERNAL_PLATFORM_CONFIG_DIRS:}${SELF}/share/egl/egl_external_platform.d
VK_LAYER_PATH=${VK_LAYER_PATH:+$VK_LAYER_PATH:}${SELF}/share/vulkan/implicit_layer.d/:${SELF}/share/vulkan/explicit_layer.d/
XDG_DATA_DIRS=${XDG_DATA_DIRS:+$XDG_DATA_DIRS:}${SELF}/share


# These are in the default LD_LIBRARY_PATH, but in case the snap dropped it inadvertently
if [ -d "/var/lib/snapd/lib/gl" ] && [[ ! ${LD_LIBRARY_PATH} =~ (^|:)/var/lib/snapd/lib/gl(:|$) ]]; then
  LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/var/lib/snapd/lib/gl
fi

if [ -d "/var/lib/snapd/lib/glvnd/egl_vendor.d" ]; then
  # This needs to be prepended, as glvnd goes depth-first on these
  __EGL_VENDOR_LIBRARY_DIRS=/var/lib/snapd/lib/glvnd/egl_vendor.d:${__EGL_VENDOR_LIBRARY_DIRS}
fi

if [ -d "/var/lib/snapd/lib/vulkan/icd.d" ]; then
  XDG_DATA_DIRS=${XDG_DATA_DIRS}:/var/lib/snapd/lib
fi

if [ -d "/var/lib/snapd/lib/gl/vdpau" ]; then
  LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/var/lib/snapd/lib/gl/vdpau
fi

export LD_LIBRARY_PATH
# Mesa-specific environment variable pointing to directory containing $VENDOR_dri.so files
export LIBGL_DRIVERS_PATH
export LIBVA_DRIVERS_PATH
# glvnd-specific environment variable pointing to directory containing ICD descriptor files
export __EGL_VENDOR_LIBRARY_DIRS
# EGL external platform interface specific environment
export __EGL_EXTERNAL_PLATFORM_CONFIG_DIRS
export VK_LAYER_PATH
export XDG_DATA_DIRS

exec "$@"
```

This script has exported the proper paths via environment variables so that
any snap that connects to our provider snap can properly resolve these
libraries.

### Multiarch Support
If the vendor drivers provide libraries for multiple sub-architectures (for example, 32-bit and 64-bit ARM) your provider snap can supply both. In this case, you will need to include all provided arch triples in the `ARCH_TRIPLETS` array in the above script (for example: `ARCH_TRIPLETS=( armhf-linux-gnu arm64-linux-gnu )`), and will need to set a few more environment variables in the `graphics-core22-provider-wrapper`.
An example from `mesa-core22` (which provides both `x86_64-linux-gnu` and `i386-linux-gnu`) adds these lines:

```sh
# /bin/graphics-core22-provider-wrapper

...
if [ "$SNAP_ARCH" == "amd64" ]; then
  GCONV_PATH=${GCONV_PATH:+$GCONV_PATH:}${SELF}/lib/i386-linux-gnu/gconv
fi

if [ -d "/var/lib/snapd/lib/gl32" ] && [[ ! ${LD_LIBRARY_PATH} =~ (^|:)/var/lib/snapd/lib/gl32(:|$) ]]; then
  LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/var/lib/snapd/lib/gl32
fi

if [ -d "/var/lib/snapd/lib/gl32/vdpau" ]; then
  LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/var/lib/snapd/lib/gl32/vdpau
fi

...

[ -z ${GCONV_PATH+x} ] || export GCONV_PATH
```

The `snapcraft.yaml` file will also need to be updated to point at the 32 bit paths.
You may find an example of using `i386` in 
[mesa-core22](https://github.com/canonical/mesa-core22/blob/9d64e64fb06f7052f9e6f8a8899cac763f5fad7e/snap/snapcraft.yaml#L176).


### How to supply the vendor libraries
The following example supplies drivers from `libmali`. Using a parameter for
`make`, the libraries will be installed to `$SNAPCRAFT_PART_INSTALL/usr/lib`.
These libraries do not need to be in a particular path.


```yaml
parts:
  ...
  vendor-drivers:
  plugin: make
  source: libmali
  make-parameters:
    - LIBDIR=$SNAPCRAFT_PART_INSTALL/usr/lib/$CRAFT_ARCH_TRIPLET_BUILD_FOR
  prime:
    - usr/lib
  ...

...
slots:
 graphics-core22:
   interface: content
   read:
     - [$SNAP]
```


### How to supply the remaining libraries
As mentioned before, you should be able to get the remaining libraries from
the Ubuntu archive. The part below can supply these libraries, and they may be
organized into any directory, in this case they are in the same path as would
be found in a classic Ubuntu system. Here is an example from the `mesa-core22`
[snapcraft.yaml](https://github.com/canonical/mesa-core22/blob/main/snap/snapcraft.yaml).


```yaml
parts:
  drm:
    # DRM userspace
    #   o libdrm.so.2
    plugin: nil
    stage-packages:
      - libdrm2
      - libdrm-common
    organize:
      # Expected at /libdrm by the `graphics-core22` interface
      usr/share/libdrm: libdrm
    prime:
      - usr/lib
      - usr/share/doc/*/copyright
      - libdrm

  va:
    # Video Acceleration API
    #   o libva.so.2
    plugin: nil
    stage-packages:
      - libva2
      - libva-drm2
      - libva-x11-2
      - libva-wayland2
    prime:
      - usr/lib
      - usr/share/doc/*/copyright

  dri:
    # Userspace drivers
    plugin: nil
    stage-packages:
      - libgl1-mesa-dri
      - va-driver-all
      - vdpau-driver-all
      - libvdpau-va-gl1
      - mesa-vulkan-drivers
      - libglx-mesa0
    organize:
      # Expected at /drirc.d by the `graphics-core22` interface
      usr/share/drirc.d: drirc.d
    prime:
      - usr/lib
      - usr/share/doc/*/copyright
      - usr/share/vulkan
      - drirc.d
    override-stage: |
      # Strip any absolute paths out of the Vulkan ICD driver manifests;
      # the driver `.so` files will not be in a fixed location, so must use
      # relative paths and rely on `LD_LIBRARY_PATH` (set by the graphics-core22-proivder-wrapper script)
      # to find the driver `.so`s
      sed -i 's@/usr/lib/[a-z0-9_-]\+/@@' ${CRAFT_PART_INSTALL}/usr/share/vulkan/*/*.json
      craftctl default
      craftctl set version=$( apt-cache policy libgl1-mesa-dri | sed -rne 's/^\s+Candidate:\s+([^-]*)-.+$/\1/p' )

  x11:
    # X11 support (not much cost to having this)
    #   o libGLX.so.0
    plugin: nil
    stage-packages:
      - libglx0
      - libx11-xcb1
      - libxau6
      - libxcb-dri2-0
      - libxcb-dri3-0
      - libxcb-present0
      - libxcb-sync1
      - libxcb-xfixes0
      - libxcb1
      - libxdamage1
      - libxdmcp6
      - libxshmfence1
    organize:
      # Expected at /X11 by the `graphics-core22` interface
      usr/share/X11: X11
    prime:
      - usr/lib
      - usr/share/doc/*/copyright
      - X11

  wayland:
    # Wayland support (not much cost to having this)
    plugin: nil
    stage-packages:
      - libwayland-client0
      - libwayland-cursor0
      - libwayland-egl1
      - libwayland-server0
      - libnvidia-egl-wayland1
    prime:
      - usr/lib
      - usr/share/doc/*/copyright
      - usr/share/egl/egl_external_platform.d
```


When you're ready, build your snap using `snapcraft`.


### Troubleshooting


#### Vendor drivers require libraries newer than core22 provides
Some vendor driver binaries might be built against newer libraries than are provided in the
Ubuntu 22.04 repositories. Particularly, the core `libwayland` libraries may introduce new
features since 22.04 that vendor binaries might require. This will manifest as missing
symbol errors at runtime.


Generally, new version requirements for libraries can be handled by adding a snapcraft
part building the required library version. An example of this for `libwayland` is:


```yaml
parts:
 ...
 wayland:
   plugin: meson
   meson-parameters:
     - -Ddocumentation=false
     - -Dtests=false
     - --prefix=$SNAPCRAFT_PART_INSTALL/usr
   build-packages:
     - libxml2-dev
     - libffi-dev
     - libexpat1-dev
     - pkg-config
   override-build: |
     snapcraftctl build
   source: https://gitlab.freedesktop.org/wayland/wayland.git
   source-type: git
   source-tag: 1.21.0
   prime:
     - usr/lib
```


## How to test a `graphics-core22` provider snap
Now we have our `graphics-core22` provider snap built. However, we don't yet know if it works
properly. Testing this snap is our next task.

Assuming that you have already installed the `graphics-test-tools` snap,
connect the snap that you've built to `graphics-test-tools`:

```shell
sudo snap disconnect graphics-test-tools:graphics-core22
sudo snap connect graphics-test-tools:graphics-core22 <your-snap>:graphics-core22
```

With that installed, we can begin testing.

### Use `eglinfo` to test
This is the most basic test that can be performed, and is a good indication of baseline GPU
driver setup. This should list _at least_ a GBM platform with the expected vendor and 
`OpenGL_ES` client API. An abbreviated example (on a `mesa-core22` system):

```
$ graphics-test-tools.eglinfo
EGL client extensions string:
	EGL_EXT_device_base EGL_EXT_device_enumeration EGL_EXT_device_query
	EGL_EXT_platform_base EGL_KHR_client_get_all_proc_addresses
	EGL_EXT_client_extensions EGL_KHR_debug EGL_EXT_platform_device
	EGL_EXT_platform_wayland EGL_KHR_platform_wayland
	EGL_EXT_platform_x11 EGL_KHR_platform_x11 EGL_EXT_platform_xcb
	EGL_MESA_platform_gbm EGL_KHR_platform_gbm
	EGL_MESA_platform_surfaceless


GBM platform:
EGL API version: 1.5
EGL vendor string: Mesa Project
EGL version string: 1.5
EGL client APIs: OpenGL OpenGL_ES
EGL driver name: iris
EGL extensions string:
	EGL_ANDROID_blob_cache EGL_ANDROID_native_fence_sync
	EGL_EXT_buffer_age EGL_EXT_create_context_robustness
	EGL_EXT_image_dma_buf_import EGL_EXT_image_dma_buf_import_modifiers
	EGL_IMG_context_priority EGL_KHR_cl_event2 EGL_KHR_config_attribs
	EGL_KHR_context_flush_control EGL_KHR_create_context
	EGL_KHR_create_context_no_error EGL_KHR_fence_sync
	EGL_KHR_get_all_proc_addresses EGL_KHR_gl_colorspace
	EGL_KHR_gl_renderbuffer_image EGL_KHR_gl_texture_2D_image
	EGL_KHR_gl_texture_3D_image EGL_KHR_gl_texture_cubemap_image
	EGL_KHR_image EGL_KHR_image_base EGL_KHR_image_pixmap
	EGL_KHR_no_config_context EGL_KHR_reusable_sync
	EGL_KHR_surfaceless_context EGL_EXT_pixel_format_float
	EGL_KHR_wait_sync EGL_MESA_configless_context EGL_MESA_drm_image
	EGL_MESA_image_dma_buf_export EGL_MESA_query_driver
	EGL_WL_bind_wayland_display
Configurations:
 	bf lv colorbuffer dp st  ms	vis   cav bi  renderable  supported
  id sz  l  r  g  b  a th cl ns b	id   eat nd gl es es2 vg surfaces
---------------------------------------------------------------------
0x01 32  0 10 10 10  2  0  0  0 0 0x30335241--     	y  y  y 	win
0x02 32  0 10 10 10  2 16  0  0 0 0x30335241--     	y  y  y 	win
0x03 32  0 10 10 10  2 24  0  0 0 0x30335241--     	y  y  y 	win
0x04 32  0 10 10 10  2 24  8  0 0 0x30335241--     	y  y  y 	win
0x05 32  0 10 10 10  2  0  0  2 1 0x30335241--     	y  y  y 	win
… lots more configurations, then other platform details …
```

If `eglinfo` does **NOT** list a GBM platform, or generates errors then you want to look
at  [possible complications](#possible-complications). If `eglinfo` does list a GBM platform
then we can proceed to testing `ubuntu-frame`.

### Test if `ubuntu-frame` works
First, install `ubuntu-frame`:
```shell
sudo snap install ubuntu-frame --devmode
```

Next, disconnect it from the default graphics interface:
```shell
sudo snap disconnect ubuntu-frame:graphics-core22
```

Then, connect it to your new provider snap:
```shell
sudo snap connect ubuntu-frame:graphics-core22 <your-snap>:graphics-core22
```

Finally, run `ubuntu-frame`:
```shell
ubuntu-frame
```

If everything previously has gone correctly this should result in `ubuntu-frame` coming
up on the connected outputs. If not, the log messages will hopefully help identify issues.

## How to test the confinement of your *Mir*-based snap
Once you have your `graphics-core22` provider snap setup, and you are confident that it works
well, you'll want to make sure that you can run the snap that *depends on* your provider snap
in a confined mode. Since we already have `ubuntu-frame` setup, we will use it as an example.
However, you should be able to run these tests on any *Mir*-based snap.

First, let's uninstall `ubuntu-frame` if it is installed:

```shell
sudo snap remove ubuntu-frame
```

Next, we can install `ubuntu-frame` without the `--devmode` flag and
connect it to the `graphics-core22` slot of your provider snap:
```shell
sudo snap install ubuntu-frame
sudo snap disconnect ubuntu-frame:graphics-core22
sudo snap connect ubuntu-frame:graphics-core22 <your-snap>:graphics-core22
```

Finally, we will run `ubuntu-frame` like before:
```shell
ubuntu-frame
```

If everything works, then `ubuntu-frame` is ready. If not, we'll have to troubleshoot.

### Troubleshooting
When bringing up a confined ubuntu-frame snap on a new board with new drivers there are two
separate access control mechanisms:
- AppArmor
- The devices cgroup

#### AppArmor
The first is relatively easy to debug. When AppArmor would deny access to a resource, it outputs a nice
message in `dmesg`:

```
apparmor="DENIED" operation="open" profile="snap.ubuntu-frame.ubuntu-frame" name="/run/udev/data/c189:1" <other stuff>
```

You can find the AppArmor profile at `/var/lib/snapd/apparmor/profiles/snap.ubuntu-frame.ubuntu-frame`.
Make modifications to the profile using your favorite text editor. Finally, apply the change to replace
the default confinement:

```shell
sudo apparmor_parser -r /var/lib/snapd/apparmor/profiles/snap.ubuntu-frame.ubuntu-frame
```

**Warning**: Please note that this profile is regenerated automatically by many `snapd` actions.
As such, the modifications that you just made are *temporary*. To make them permanent,
you will need to upstream the fix against `snapd`.

#### The devices cgroup
The cgroup confinement is less easy to debug. The kernel emits no logs when the devices cgroup
denies access to a device node. The device node on the filesystem will appear to have the correct
permissions (and, for example, you will be able to touch it), but calls to `open()` will fail
with `EPERM`. If everything seems to be set up correctly, and there are no errors in `dmesg`,
but `Mir` is failing it’s likely that the devices cgroup confinement is to blame.

Additionally, it is more difficult to test as the cgroup is only set up during snap run startup,
and so the knobs we need to twiddle only exist while the snap is running.

We can get around this by running:
```shell
snap run --shell ubuntu-frame
```
This command will get snapd to do all the initialisation and drop us into a shell.
Once the shell exists, there will be two(?!) cgroup folders found under `/sys/fs/cgroup/devices`:
1. /sys/fs/cgroup/devices/system.slice/snap.ubuntu-frame.ubuntu-frame.${UUID}.scope, and
2. /sys/fs/cgroup/devices/snap.ubuntu-frame.ubuntu-frame

It is (2) that we’re after. You can check that you’ve got the right directory, because the
`devices.list` file will contain a bunch of lines like:

```
c 5:1 rwm
c 5:2 rwm
c 136:* rwm
c 137:* rwm
c 138:* rwm
```

You now need to give the cgroup permission to access the necessary devices, for which you need 
the `major:minor` of the device nodes. You can find that with ls:
```
$ ls -la /sys/fs/cgroup/devices/snap.ubuntu-frame.ubuntu-frame
crw-rw-rw- 1 root root 10, 60 Jan 10 04:56 /dev/mali0
```

Here we see that the `/dev/mali0` device node has `major:minor` equal to `10:60`.

Now we can enable access to the relevant device node, via:
```shell
echo "c 10:60 rw" | sudo tee /sys/fs/cgroup/devices/snap.ubuntu-frame.ubuntu-frame/devices.allow
```

And now we can go back to the shell, and try running `ubuntu-frame`:

```shell
$SNAP/usr/local/bin/frame
```
