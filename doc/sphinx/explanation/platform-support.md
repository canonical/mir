---
discourse: 13185
---

# Platform support

This document describes the requirements for running Mir.

---

Mir supports graphics and input “platforms” by using modules loadable at runtime. This means that applications built with Mir can run across a range of graphics stacks.

There are Mir platforms supplied with Mir and the potential to develop more. There is also one platform we know of developed by a third party.

The most widely used and tested platform is “mesa-kms” which works with the Mesa collection of open-source graphics drivers. That’s used on Intel-graphics based desktops and a variety of SBCs and other devices.

## Summary list of current Mir graphics platforms

Platform|Status|Description
--|--|--
gbm-kms|Released|Works with any driver providing KMS, `libgbm` and an EGL supporting [EGL_WL_bind_wayland_display](https://registry.khronos.org/EGL/extensions/WL/EGL_WL_bind_wayland_display.txt). The open-source Mesa drivers are the obvious example, but other drivers supporting these interfaces should work - for example, the binary MALI drivers provided by ARM have been tested on some devices.
x11|Released|Provides “Mir-on-X11” primarily for development.
eglstream-kms|Released|Works with proprietary Nvidia drivers.
wayland|Released|Provides “Mir-on-Wayland” both for nested compositors and for development.
android|Released|3rd party (UBports) works with a libhybris container for Android drivers. (Typically for phone hardware with Android-only drivers.)
dispmanx|Released|Works with Broadcom proprietary DispmanX API.

Each of these platforms depends on having the corresponding kernel and userspace drivers installed on the system. On startup Mir will “probe” the system for support of the above and select the best supported platform.

## Choosing hardware and drivers

When configuring a system for use with Mir it is not only necessary to check the hardware and drivers for compatibility with Mir, but also with the application software.

For example, if the application software uses 3D or video acceleration, then that needs to be supported by the application (possibly through a supporting library) on the selected drivers and hardware. It is very possible that the support differs between the open-source drivers and the proprietary ones. Video decoding, in particular, varies a lot between graphics stacks.

There’s a further complication in that Mir expects applications to use Wayland and some features on some drivers don’t work with Wayland. (An example is that the  intel-vaapi-driver for Ubuntu 18.04 doesn’t work with Wayland. That is fixed in more recent versions, but may need to be addressed for core18 snaps.)

Also, there are applications that do not support Wayland directly. These can be supported with Xwayland - a program that translates from X11 to Wayland for the application. But that leads to further limitations as the GL acceleration support through Xwayland depends upon the graphics drivers. It works with Intel on Mesa, but beyond that it is patchy.

## Some hardware examples

### RPi 3

GPU: VideoCore IV

Mesa open-source graphics stack|Proprietary driver
--|--
VC4 - open source kernel/mesa stack.|Dispmanx - Broadcom semi-proprietary stack.
Mir works, and is tested on this.|Mir works, not yet incorporated into CI lab testing
Supports GL.|Requires out-of-tree patches to https://github.com/raspberrypi/userland to enable 3D and video decode clients.
Does not support OMX.<br/>May support some video encode/decode via mmal interface. This is apparently slower than OMX.|Potentially higher performance (particularly OMX for video decode)
As a mesa/gbm-based platform, would expect 3D to work in Xwayland.<br/>Mmal may work in Xwayland.|As a non-mesa/gbm platform would likely not have 3D (or video decode) in Xwayland.

### i.MX6
GPU: Vivante GC something ([varies by model](https://en.wikipedia.org/wiki/I.MX#i.MX_6_series))

Mesa open-source graphics stack|Proprietary driver
--|--
Etnaviv - Full open-source stack, using standard KMS/dma-buf/gbm interfaces. Reverse-engineered.<br/>_As it’s using a full open-source stack, this would be easiest to support._|Vivante - proprietary driver.
Mir would use same platform as on the desktop - gbm-kms.<br/> _We've tested the Boundary Devices i.MX6 on classic Ubuntu. Ubuntu Core would require some enablement work (adding the etnaviv driver)._|Would require writing a Mir platform (this is clearly possible; there are patches for Weston to support Vivante)
Supports 3D (mesa GL) + video decoding (CODA v4l2)|Supports 3D + video decoding
Performance may be an issue (for example, https://github.com/Igalia/meta-webkit/issues/13)|Performance may be better; supported by downstream projects (again, cf: https://github.com/Igalia/meta-webkit/wiki/i.MX6)
Notably - the open source stack should provide 3D acceleration (and potentially video acceleration) in Xwayland.|Support for 3D in Xwayland is unknown; would likely require significant out-of-tree patches.

### i.MX8
GPU: Vivante GC7000
Mesa open-source graphics stack|Proprietary driver
--|--
Etnaviv - Full open-source stack, using standard KMS/dma-buf/gbm interfaces. Reverse-engineered.|Vivante - proprietary driver.
Same comments as etnaviv on i.MX6 apply, but the GPU is newer and the driver support is likewise newer; there may be more missing features/bugs than i.MX6 support.|same comments as i.MX6. <br/>_It’s likely that a Mir Vivante platform would work on both i.MX6 and i.MX8; likewise, there is apparently Weston support (again, in the form of out-of-tree patches)_
Looks like there might not be open-source video decode support (https://community.nxp.com/t5/i-MX-Processors/i-MX8-Linux-video-decode-support/m-p/796955/highlight/true#M123321). Unknown whether we could interface the IMX bits with the etnaviv DRM bits.|Proprietary video decode solution.

## Driver requirements

Different Mir platforms require different features of the underlying driver stack. The features needed to enable a given Mir platform are:

### gbm-kms

The gbm-kms platform requires a DRM device node with KMS support, a `libgbm` implementation for buffer allocation, and an EGL implementation supporting at least [EGL_KHR_platform_gbm](https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_platform_gbm.txt) (or the equivalent [EGL_MESA_platform_gbm](https://registry.khronos.org/EGL/extensions/MESA/EGL_MESA_platform_gbm.txt) extension) and [EGL_WL_bind_wayland_display](https://registry.khronos.org/EGL/extensions/WL/EGL_WL_bind_wayland_display.txt).

Optionally, the EGL implementation can support [EGL_EXT_image_dma_buf_import](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_image_dma_buf_import.txt), [EGL_EXT_image_dma_buf_import_modifiers](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_image_dma_buf_import_modifiers.txt), and use the [linux_dmabuf_unstable_v1](https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/master/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml) Wayland protocol for client buffer transfer. Support for this was added in Mir 2.3. Composite-bypass support depends on this implementation. Some future performance improvements, such as overlay plane usage, are likely to require this support from the driver stack.

### eglstream-kms

The eglstream-kms platform requires a DRM device node with Atomic KMS support and an EGL implementation supporting [EGL_EXT_platform_base](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_platform_base.txt), [EGL_EXT_platform_device](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_platform_device.txt), [EGL_EXT_device_base](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_base.txt), [EGL_EXT_device_drm](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_drm.txt), and [EGL_EXT_output_base](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_output_base.txt).
