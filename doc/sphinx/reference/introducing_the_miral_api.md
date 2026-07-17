(ref-introducing-the-miral-api)=

# Introducing the MirAL API

When using Mir to develop a compositor the principal library API you will be
using is MirAL - the "Mir Abstraction Layer". This provides a stable ABI onto
the most useful functionality Mir provides.

## Anatomy of the "Hello World" Compositor

The "Hello World" program written using MirAL is described in {ref}`write-your-first-wayland-compositor`.

This example uses three components of MirAL:

| Component                                   | Description                                               |
| ------------------------------------------- | --------------------------------------------------------- |
| {class}`miral::MirRunner`                   | This allows you to configure and run a Wayland compositor |
| {func}`miral::set_window_management_policy` | This integrates your chosen window management policy      |
| {class}`miral::MinimalWindowManager`        | This is one of the supplied window management policies    |

There are a lot of other components provided, but the {class}`miral::MirRunner`,
is the heart of any compositor.

Other compositors will use more MirAL components to do more, but the overall
structure will remain the same.

## Some highlights of the MirAL API

### miral::MirRunner

This class, {class}`miral::MirRunner`, is the heart of any compositor:

- you set up and start the Wayland server using {func}`miral::MirRunner::run_with`; and,
- stop it using {func}`miral::MirRunner::stop`.

The {func}`miral::MirRunner::run_with` allows you to tailor your compositor to
your specific requirements: You customise any components you wish to incorporate
(or write new ones) and pass them to {func}`miral::MirRunner::run_with`.

In addition to starting and stopping your Wayland compositor, {class}`miral::MirRunner`
provides a hooks for co-ordinating other processing you need by running code:

- when the server has started: {func}`miral::MirRunner::add_start_callback`;
- before the server stops: {func}`miral::MirRunner::add_stop_callback`;
- when a signal is received: {func}`miral::MirRunner::register_signal_handler`; and
- when a file descriptor becomes readable: {func}`miral::MirRunner::register_fd_handler`.

### miral::WindowManagementPolicy

This is an important way to customise your compositor. Changing the
way that windows are managed affects the way users interact with your compositor.

The "Hello World" example uses {class}`miral::MinimalWindowManager` which is an
implementation of the {class}`miral::WindowManagementPolicy` interface. This
interface is how you can specify your chosen window management approach.

Three of our downstream projects implement their own window management policies
that take very different approaches to window management. Each of them provides
a custom implementation of the {class}`miral::WindowManagementPolicy` interface.

- [Ubuntu Frame](https://github.com/canonical/ubuntu-frame) provides a fullscreen experience
- [Miriway](https://github.com/Miriway/Miriway) offers traditional "floating windows" with support for workspaces
- [miracle-wm](https://github.com/miracle-wm-org/miracle-wm) features a tiling window manager

The {class}`miral::WindowManagementPolicy` interface has two important types of
function:

- `handle_...` functions expect the policy to take action in response; and,
- `advise_...` functions notify the policy that something is happening.

### miral::WindowManagerTools

When implementing a {class}`miral::WindowManagementPolicy` you will need
the functions provided by {class}`miral::WindowManagerTools`. This lets you
direct the handling of windows. It is supplied to your window management policy
constructor for later use.

### Other MirAL components

Most of the remaining MirAL components are designed to be passed into
{func}`miral::MirRunner::run_with`. They offer a range of capabilities including:

- Adding your own configuration options: {class}`miral::ConfigurationOption`;
- Supporting X11 applications: {class}`miral::X11Support`;
- Launching apps: : {class}`miral::ExternalClientLauncher`;
- Controlling the keyboard layout: {class}`miral::Keymap`;
- Controlling the available Wayland extension protocols: {class}`miral::WaylandExtensions`; and,
- Selecting server-side or client-side decorations: {class}`miral::Decorations`.

### Custom server-side decorations

MirAL supports **limited buffer-based customization** of server-side decorations via
{class}`miral::CustomDecorations` and the types in {file}`miral/custom_decorations.h`:
{class}`miral::DecorationStrategy`, {class}`miral::DecorationBuffers`, and
{class}`miral::DecorationSurface`.

This is separate from {class}`miral::Decorations`, which controls SSD vs CSD negotiation.
Use both a {class}`miral::Decorations` policy and {class}`miral::CustomDecorations` when
replacing the built-in SSD appearance (for example `prefer_ssd()` or `prefer_csd()`).

See {ref}`custom-server-side-decorations` and
`examples/miral-custom-decorations/miral-custom-decorations.cpp`.

#### Limitations

- Metric methods (`titlebar_height()`, `side_border_width()`, `bottom_border_height()`)
  default to 24/4/4 pixels and control *internal* geometry (distinct from the client-visible
  size from `compute_size_with_decorations()`).
- Rendering methods receive `WindowState`, `InputState`, and `DecorationBuffers` on each
  call and return an optional {class}`miral::DecorationSurface`. Implementations must be
  stateless.
- Allocate pixels with `DecorationBuffers::create_mapping()`; public headers do not expose
  `mir::graphics::Buffer`.
- Buffer pixel dimensions must account for `WindowState::scale()` (HiDPI).
- Only software buffers are supported.
- Button functions are limited to close, maximize, and minimize.
- The renderer strategy is fixed at `MirRunner` construction; it cannot be changed at runtime.

## Further reading

You can explore the full API in the - [API reference](/api/library_root)
