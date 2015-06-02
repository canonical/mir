Linux Kernel Requirements for Mir
=================================

To run Mir with the default `mesa-kms` platform you need a linux kernel with at
least:

Modules: i915, radeon and nouveau, to support the broadest range of common
desktop/laptop hardware.

Version: Kernel version 3.11.0 or later, at least for radeon and nouveau to
support fullscreen bypass correctly. Intel (i915) is known to work properly
with even older kernels.

Additional patches: None at this time.
