# Performance

This document aims to explore in depth the performance considerations around Mir
based compositors.

Note that Mir is a C++ library for building compositors, not a product itself.
The following applies to all Mir based compositors, but it is theoretically
possible to defeat the available performance in such products.

Where possible, Mir makes use of the GPU for compositing as this is faster than
compositing on the CPU.

All measures of performance are affected by the following:

- the number, size and configuration of the displays
- the number of application windows being composited
- the rate at which buffers are submitted to the application windows

## Memory usage

Memory usage varies a lot depending on:

- the GPU characteristics of the system

The range being from around 200MB for a simple compositor (Frame) running on a
single display with a single application to 7GB for a "desktop capable"
compositor (Miriway) running on multiple displays, hybrid graphics and dozens of
applications.

## CPU usage

CPU usage is generally low but varies a lot depending on:

- the CPU and GPU characteristics of the system

Where using the GPU for compositing is not possible, Mir may make use of Mesa's
software renderer or copy buffers between GPU and display driver. This has a
cost in CPU use. In particular, CPU usage
[has been reported](https://github.com/canonical/mir/issues/3230) as
being significantly higher when using DisplayLink's evdi drivers to handle
external displays via a "docking station".

## GPU usage

GPU usage is generally low but varies a lot depending on:

- the GPU characteristics of the system

## Frame rate

Mir normally has no difficulty keeping up with applications but frame rate can
vary a lot depending on:

- the CPU, GPU and memory characteristics of the system

Part of our test automation is tracking the results of running some test
applications (e.g. `glmark2-wayland`) on low-end hardware on which we know Frame
has been deployed. These results can be as low as 30fps, but on "normal desktop
hardware" figures in the 10s of thousand fps are reported.
