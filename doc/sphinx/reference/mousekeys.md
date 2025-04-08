Mousekeys
========

Mousekeys is an accessibility feature that allows you to move the mouse pointer,
issue clicks, double clicks, drag, and drop, using only your keyboard.

## Enabling Mousekeys

Mir supports enabling mousekeys at launch time with `--enable-mouse-keys=1`.
`true` and `on` can also be used instead of `1`.

Note that the availability of mousekeys and its options depends on the shell
author opting in to enable mousekeys at the compositor level. If you are a
shell author and wish to enable mousekeys, instructions can be found in
[MouseKeys API](mousekeys-api).

## Using Mousekeys

By default, mousekeys use the numpad to move the mouse pointer. Numlock must be
turned on.

The default keymap allows you to use the following buttons:
- `2`, `4`, `6`, and `8` to move the pointer. Diagonal movement via
  combinations is supported.
- `5` to issue a click of whatever pointer button is chosen. By default, that
  is the left mouse button.
- `+` to issue a double click.
- `/`, `*`, `-` to switch between the left, middle, and right mouse buttons.
  Once you switch to a button, this state is preserved until you switch to
  another button.
- `0` and `.` to start and stop dragging respectively.

## Customizing mousekeys

In addition to basic pointer controls, our mousekeys implementation allows you
to customize the max speed and acceleration curve via launch options and
programmatically. The keymap can only be customized through a programmatic
interface via the MirAL API. Information about this interface can be found at
[MouseKeys API](mousekeys-api).

Maximum pointer speed can be controlled on the X and Y axes separately by
passing `--mousekeys-max-speed-x=<value>` and `--mousekeys-max-speed-y=<value>`.

The acceleration curve is quadratic by default, but you can customize the
constant, linear, and quadratic factors of the curve by passing
`--mouse-keys-acceleration-constant-factor=<value>`,
`--mouse-keys-acceleration-linear-factor=<value>`, and
`--mouse-keys-acceleration-quadratic-factor=<value>` respectively.

For example, if you wish to eliminate acceleration altogether, you can pass
`--mouse-keys-acceleration-linear-factor=0 --mouse-keys-acceleration-quadratic-factor=0`
when launching mir.
