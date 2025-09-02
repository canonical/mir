# Energy Efficiency

This document aims to explore in depth the energy efficiency considerations 
around Mir based compositors.

Note that Mir is a C++ library for building compositors, not a product itself.
The following applies to all Mir based compositors, but it is theoretically 
possible to defeat the available efficiencies in such products.

## GPU acceleration

Mir based compositors default to hardware compositing on any available GPU. Mir 
includes a "graphics platform" abstraction that allows efficient implementation
on different software or graphics stacks.

## Display timeout

Mir provides an `idle-timeout` configuration option for setting the time (in 
seconds) the compositor will remain idle before turning off the display.

## Compositor optimisations

### Occlusion

Where client windows are "occluded" when they do not contribute to the content
of the display. For example, being offscreen or hidden by another window.

Mir does not trigger recomposition when the content of an occluded window
updates. It also drastically "throttles" the repainting of the occluded windows.

### Hardware composition

Depending on the capabilities of the GPU composition of visual elements (such as
cursors and application windows) can be handled without direct intervention of
the CPU or the explicit provision of a full frame buffer.

Historically, Mir development has focused on hardware with limited capabilities
for hardware composition. However, both the range of hardware for which Mir is
considered and the capabilities of "low end" hardware are increasing. As a 
result, Mir has work in progress to exploit additional features of hardware composition 
options where they are available.

Mir currently uses hardware composition for cursors
(where supported by the GPU hardware) and can, in some usecases, enable
"composition bypass" for fullscreen clients.

### Damage tracking

Damage tracking involves passing information about which parts of buffers have
been updated. That can avoid the need to regenerate parts of the display that
have not changed.

Mir has not implemented this optimisation yet.

## Metrics

We are unable to provide meaningful metrics for power consumption attributable
to Mir as actual power consumption is dependent on these variables:

* the CPU, GPU and memory characteristics of the system
* the number, size and configuration of the displays
* the number of application windows being composited
* the rate at which buffers are submitted to the application windows
* the Mir configuration deployed in the product
