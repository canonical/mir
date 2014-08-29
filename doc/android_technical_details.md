Android Technical Details {#android_technical_details}
===============================

Mir Usage of Android Drivers
----------------------------

Mir relies on the libhybris library to use the Android drivers. This allows the
drivers to use the bionic libc while Mir itself uses the standard GNU libc.

Mir Display Modes
-----------------

When you're using Mir to drive the display of the Android device, Mir has
a default way to display, as well as a backup mode that it can try if the
default mode isn't working.

 *  Default Mode
    The default display mode uses the HWC HAL module from the Android drivers.
    Depending on the version and device, the default mode may also use the
    framebuffer HAL module. These modules are used to determine the display
    information, to synchronize with vsync, and to post to the display. The HWC
    is not used at this time to provide overlay support.
 *  Backup Mode
    The backup mode is used when the primary display mode is unavailable (due
    to system problems, missing shared libraries, or similar conditions)
    The backup mode will only use the framebuffer HAL module. This module is a
    bit more limited than the HWC module, and the driver support might be a bit
    less thoroughly tested. It still should give you a display though.

Mir Device Support
------------------

In theory, all devices with that use the normal Android drivers abstractions
should run Mir. Currently, we support HWC (hardware composer) version 1.0 and
later. The deprecated FB HAL module from android's libhardware should also work.
If you are attempting to get mir to work on a new device, check out
\subpage android_new_device_bringup "this page" for suggestions on
troubleshooting. You can also 
<a href="https://bugs.launchpad.net/mir">file a bug,</a> being very specific
about the chipset, GPU, and driver versions that your phone has. 
