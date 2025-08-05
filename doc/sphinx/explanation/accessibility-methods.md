# Accessibility Methods
Accessibility methods are options which change the behavior of Mir to
accommodate users with disabilities. Mir offers a variety of accessibility
methods which can be broadly categorised into: Pointer accessibility, typing
accessibility, and visual accessibility methods. 

## Pointer Accessibility
Mir includes the following pointer accessibility methods:
1. Mousekeys
2. Simulated secondary click
3. Hover click

### Mousekeys
Mousekeys allows users to control the mouse pointer through their keyboard.
This not only includes movement and primary clicks, but it also extends to
secondary and tertiary clicks, with the ability to dispatch single and double
clicks, as well as holding and releasing mouse buttons.

For a more detailed look at the API and usage of mousekeys, please check out
{cpp:class}`miral::MouseKeysConfig`.

### Simulated Secondary Click
Simulated secondary click allows users to emulate a secondary click by holding
down the primary click button for a short duration. The pointer is allowed to
slightly move from the position the simulated secondary click started from to
further accommodate users. 

Mir additionally provides callbacks that are triggered when the click starts,
when it's cancelled, and when it successfully dispatches a click. These
callbacks can be used to provide custom visual and audio feedback.

For a more detailed look at the API and usage of simulated secondary click,
please check out {cpp:class}`miral::SimulatedSecondaryClick`

### Hover Click
Hover click allows users to dispatch primary clicks by hovering the pointer at
a specific point for a short period of time. After the hover click is
initiated, the pointer is allowed to slightly move.

After a click is successfully dispatched, no further clicks will be initiated
unless the pointer moves a short distance from the position of the last click.

Mir allows you to provide callbacks for when clicks are initiated, cancelled,
or dispatched. They can be used to provide visual and audio feedback.

For a more detailed look at the API and usage of hover click, please check out
{cpp:class}`miral::HoverClick`

## Typing Accessibility
Mir includes the following Typing accessibility methods:
1. Sticky Keys
2. Slow Keys
3. Bounce Keys

### Sticky Keys
Sticky keys allows users to perform key combos that require holding down
modifier keys without having to keep modifier keys pressed down.

Sticky keys can be temporarily disabled if users press two modifiers
simultaneously. This is useful for users who can press some keyboard shortcuts
that require keys next to each other, but not more complicated shortcuts, for
example.

You can provide a callback to trigger when modifier keys are pressed and become
sticky. This can be used to provide feedback to users.

For a more detailed look at the API and usage of sticky keys, please check out
{cpp:class}`miral::StickyKeys`

### Slow Keys
Slow keys allows users with motor disabilities to configure Mir to ignore key
presses lasting less than a specific period.

Mir allows you to provide callbacks which are invoked when a key is depressed,
when a key press is rejected, and when a key press is accepted.

For a more detailed look at the API and usage of slow keys, please check out
{cpp:class}`miral::SlowKeys`

### Bounce Keys
Bounce keys prevents multiple successive presses of the same key issued during
a small window of time from registering multiple times.

You can optionally attach a rejection handler to provide feedback to users on
rejected presses.

For a more detailed look at the API and usage of bounce keys, please check out
{cpp:class}`miral::BounceKeys`

## Visual Accessibility
Mir includes the following visual accessibility methods:
1. Cursor scaling
2. Colour filters
3. Zooming/Magnification

### Cursor Scaling
Cursor scaling allows users to scale the cursor at runtime to any value between
0 and 100. 

For a more detailed look at the API and usage of cursor scaling, please check out
{cpp:class}`miral::CursorScale`

### Colour Filters
Mir provides two colour filters by default, a grayscale filter, and a colour
inversion filter. At the moment, it's not possible to implement your own custom
filter.

For a more detailed look at the API and usage of colour filters, please check out
{cpp:class}`miral::OutputFilter`

### Zooming/Magnification
For users who require temporary magnification, Mir provides a built-in
magnification tool. By default, it magnifies a small area around the cursor,
but the magnification power and magnification area can be modified at runtime.

For a more detailed look at the API and usage of magnification in Mir, please
check out {cpp:class}`miral::Magnifier`
