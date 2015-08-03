Android New Device Bringup {#android_new_device_bringup}
===============================

Mir is a library, and is the building block that unity-system-compositor and
unity8 are build upon. If the device is crashing or hanging in a Mir library
when you try to start unity, the information listed below will help the Mir
team diagnose and fix the issues that are seen.

##Mir Tests##
The android platform of Mir has a variety of tests and tools that help the Mir
team troubleshoot new devices. There are thousands of tests, but certain tests
exercise the driver and have proven useful in diagnosing driver problems. These
tests are available in the 'mir-test-tools' debian package.

###Mir Client Software Rendering###

    mir_integration_tests --gtest-filter="AndroidHardwareSanity.client_can_draw_with_cpu"

This test checks that a buffer can travel from the server to the client accross
the interprocess communication channel. It then renders some pixels using
software and the server will check that the pixels are present in the buffer. 

###Mir Client OpenGLES 2.0 Rendering (Android EGLNativeWindowType test)###

    mir_integration_tests --gtest-filter="AndroidHardwareSanity.client_can_draw_with_gpu"

This test checks that a buffer can travel from the server to the client accross
the interprocess communication channel. It then renders some pixels using
OpenGLES 2.0 and the server will check that the pixels are present in the
buffer. 

###Mir Display posting (HWC tests)###

    mir_integration_tests --gtest-filter="AndroidHardwareSanity.display_can_post"
    mir_integration_tests --gtest-filter="AndroidHardwareSanity.display_can_post_overlay"

This test checks that the display can post content to the screen. It should
flash the screen briefly and run without error. Since it is important that
screen looks flawless, a visual inspection should also be perfomed using
mir_demo_standalone_render_to_fb 

###Mir GPU buffer allocations (gralloc tests)###

    mir_integration_tests --gtest-filter="AndroidHardwareSanity.can_allocate_sw_buffer"
    mir_integration_tests --gtest-filter="AndroidHardwareSanity.can_allocate_hw_buffer"

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

    mir_proving_server

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

###Note on HWC versions###
The "--hwc-report log" will log the HWC version at the beginning of the report,
 e.g.

    HWC version 1.2

If you see something like:

    HWC version unknown (<version>)

This means that Mir does not support the version of hwc on the device.
As of Dec 2014, Mir supports the legacy FB module, as well as HWC versions
1.0, 1.1, 1.2, and 1.3. 

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
