Android New Device Bringup {#android_new_device_bringup}
===============================

Mir is a library, and is the building block that unity-system-compositor and
unity8 are build upon. If the device is crashing or hanging in a Mir library
when you try to start unity, the information listed below will help the Mir
team diagnose and fix the issues that are seen.

##Mir Tests##
Mir has a test suite available in the package mir-test-tools that checks the
operation of Mir.
Mir also provides a test suite in mir-android-diagnostics that is helpful in
checking how a new device will work with Mir. 

##Mir and libhybris##
Vendor provided drivers are compiled against Android's bionic libc.
Mir runs against glibc. To make the two libc's work together, Mir uses
libhybris.

libhybris has some internal tests (e.g., test_egl), but these are insufficient
tests to see if mir will run on the device. The tests are aimed checking
the operation of hybris on non-Mir graphics stacks. Furthermore, they test a
more limited range of hardware module capabilities (e.g. they don't test HWC
overlay capability) 

###Mir Client Software Rendering###

    mir_android_diagnostics --gtest_filter="AndroidMirDiagnostics.client_can_draw_with_cpu"

This test checks that the CPU can render pixels using software
and checks that the pixels can be read back.

###Mir Client OpenGLES 2.0 Rendering (Android EGLNativeWindowType test)###

    mir_android_diagnostics --gtest_filter="AndroidMirDiagnostics.client_can_draw_with_gpu"

This test checks that the GL api can be used to glClear() a buffer, and checks
that the content is correct after render.

###Mir Display posting (HWC tests)###

    mir_android_diagnostics --gtest_filter="AndroidMirDiagnostics.display_can_post"
    mir_android_diagnostics --gtest_filter="AndroidMirDiagnostics.display_can_post_overlay"

This test checks that the display can post content to the screen. It should
flash the screen briefly and run without error. Since it is important that
screen looks flawless, a visual inspection should also be perfomed using
mir_demo_standalone_render_to_fb and mir_demo_standalone_render_overlays 

###Mir GPU buffer allocations (gralloc tests)###

    mir_android_diagnostics --gtest_filter="AndroidMirDiagnostics.can_allocate_sw_buffer"
    mir_android_diagnostics --gtest_filter="AndroidMirDiagnostics.can_allocate_hw_buffer"

This will test that Mir can access the gralloc module and allocate GPU buffers.

Mir Demos
---------
The Mir team ships certain demos that are useful for developing and improving
Mir, as they operate with less complexity than the full unity stack. These are
available in the 'mir-demos' debian package.

###Visual check of posting to the display using GLES###

    mir_demo_standalone_render_to_fb

This will use the HWC module to drive the display. The program will display an
animation to the screen until the program is stopped with Ctrl-C. This program
forces HWC to display the OpenGLES 2.0 rendered image without using overlays.

The animation should:
 - be a black, white, and purple image of the Mir space station scrolling on a
green background
 - be smooth
 - be checked against visual artifacts (flickering, blockiness, tearing)
 - run indefinitely

###Visual check of posting to the display using Overlays###

    mir_demo_standalone_render_overlays

This will use the HWC module to drive the display by displaying overlays.
The animation should:
 - be a blue square on top of a red square. Both squares will quickly fade to
white, reset to their respective colors, and repeat.
 - be smooth
 - be checked against visual artifacts (flickering, blockiness, tearing)
 - run indefinitely

###Visual check of demo servers###

The demo servers provide a good way to check visually that clients can connect
and display to the screen.
In one terminal, run

    mir_demo_server

and in another terminal, run

    mir_demo_client_egltriangle

You should see:
 - an orange triangle smoothly rotating on a purple background on screen
 - the client terminal reporting an FPS that is close to the vsync rate of the
system.
 - both client and server continuing to run until stopped with Ctrl-C.

There are a variety of demos that should all work with each other. The demo
shell is talked about further in \ref demo_shell_controls

Collecting Additional Android Information From Mir
--------------------------------------------------
Mir has some android specific options for watching HWC interactions. This
option is available with mir_demo_standalone_* and mir_demo_server_*

If you run 

    mir_proving_server --hwc-report log

You will get output similar to this:

    before prepare():
     # | pos {l,t,r,b}         | crop {l,t,r,b}        | transform | blending | 
     0 | {   0,   0, 512, 512} | {   0,   0, 512, 512} | NONE      | NONE     | 
     1 | {  80,  80, 592, 592} | {   0,   0, 512, 512} | NONE      | NONE     | 
     2 | {   0,   0, 768,1280} | {   0,   0, 768,1280} | NONE      | NONE     | 
    after prepare():
     # | Type      | 
     0 | OVERLAY   | 
     1 | OVERLAY   | 
     2 | FB_TARGET | 
    set list():
     # | handle
     0 | 0x2183540
     1 | 0x2183a00
     2 | 0x202ea40
    HWC: vsync signal off
    HWC: display off

This is the list that Mir submits to HWC, the decision of hwc (overlay or GLES),
and the final handles submitted to HWC during the display post. The HWC version
vsync signal and the blanking are also logged.

The "--hwc-report log" option should work with all android-based mir servers
and demo standalone programs. If its more convenient, setting 
MIR_SERVER_HWC_REPORT=log to the environment of the running server will give 
the hwc report on stdout.

###Android platform quirks###
If a driver has a bug that can be fixed, its best to fix the bug in the driver.
However, if workarounds are needed due to source unavailability, they can be
experimented with via quicks that can be activated via the command line.
The list of quirks is printed with the --help flag on a mir_demo_server.

###Note on HWC versions###
The "--hwc-report log" will log the HWC version at the beginning of the report,
 e.g.

    HWC version 1.2

If you see something like:

    HWC version unknown (<version>)

This means that Mir does not support the version of hwc on the device.
As of Feb 2016, Mir supports the legacy FB module, as well as HWC versions
1.0, 1.1, 1.2, 1.3, and 1.4. 

If you run

    mir_demo_standalone_render_overlays --display-report log

You will get the EGL configuration that was selected for the framebuffer EGL
context.

Reporting Problems
-----------------
If any of the above tests crash, hang, or experience a problem, the Mir team
wants to help fix it.
When diagnosing an android problem, these logs are most helpful:
 - The log of the Mir program when it crashes with the "Additional Android
Information".
 - A stacktrace from where Mir has crashed or hung.
 - The logcat from /system/bin/logcat at the time of the problem
 - The kernel log from dmesg at the time of the problem.
 - A video of any visual glitches or corruption
