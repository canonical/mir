# Graphics Driver Quriks

The gbm-kms and atomic-kms graphics platform modules provide a
`--driver-quirks` option that can control how the module behaves.

The `--driver-quirks` option can be passed multiple times. The value
is a colon separated string of the form `option:specifier:value`. The
`option` part identifies the type of quirk, while the
`specifier:value` identifies what device the quirk applies to.

## Device block list

The `skip` and `allow` quirks can be used to update the block list
used to ignore devices during enumeration and hotplug. This is mainly
useful for ignoring DRM devices exposed by the kernel that do not have
a working DRI driver.

The remainder of the option can take two forms

`driver:name`
: match all devices that use a driver `name`.

`devnode:path`
: match devices based on path.

The `skip` quirk will add an entry to the driver or device node block
lists, and the `allow` quirk will remove an entry from those lists. A
device will be skipped if it's driver or device node appear on the
ignore lists.

Note that matching of the `allow` quirk is only useful for removing
items from the default block list. You can't use it to allow a
specific device if its driver is blocked.

Examples:

- `--driver-quirks=skip:driver:nvidia`
- `--driver-quirks=allow:device:/dev/dri/card0`

## Disable probing for modesetting support

The `disable-kms-probe` quirk can be used to allow use of a device
even if it looks like a modesetting capable driver has not been
attached.

Devices can be specified using the same `driver:name` or
`devnode:path` style specifiers as `allow` or `skip`.

## GBM free buffer support

The `gbm-surface-has-free-buffers` quirk is used to handle devices
where `gbm_surface_has_free_buffers` sometimes incorrectly indicates
that a surface has no free buffers.

The quirk takes a standard `driver:name` or `devnode:path` specifier,
but adds a final `default` or `skip` token.

If the quirk is set to `default`, `gbm_surface_has_free_buffers` will
be called as normal. If the quirk is set to `skip`, Mir will assume
the result is true.

At the moment, the main use of this quirk is to work around Nvidia
drivers spuriously returning `false`. For more information, please see
[#3296](https://github.com/canonical/mir/issues/3296).

## Default quirks

A default set of quirks is configured by the driver that corresponds to:

- `skip:driver:nvidia`
- `skip:driver:ast`
- `skip:driver:simple-framebuffer`
- `disable-kms-probe:driver:virtio_gpu`
- `disable-kms-probe:driver:vc4-drm`
- `disable-kms-probe:driver:v3d`
- `gbm-surface-has-free-buffers:driver:nvidia:skip`
