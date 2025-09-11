(ref-introducing-the-miral-api)=
# Introducing the Miral API

When using Mir to develop a compositor the principle library API you will be
using is MirAL - the "Mir Abstraction Layer". This provides a stable ABI onto
the most useful functionality Mir provides.

## Anatomy of the "Hello World" Compositor 

The "Hello World" program written using MirAL is described in {ref}`write-your-first-wayland-compositor`.

This example uses three components of MirAL:

Component | Description
-- | --
{class}`miral::MirRunner`| This allows you to configure and run a Wayland compositor
{func}`miral::set_window_management_policy`| This integrates your chosen window management policy
{class}`miral::MinimalWindowManager`| This is one of the supplied window management policies

There are a lot of other components provided, but the {class}`miral::MirRunner`,
is the heart of any compositor.

Other compositors will use more MirAL components to do more, but the overall
structure will remain the same.

## Some highlights of the MirAL API

### miral::MirRunner

This class, {class}`miral::MirRunner`, is the heart of any compositor: 

* you set up and start the Wayland server using {func}`miral::MirRunner::run_with`; and,
* stop it using {func}`miral::MirRunner::stop`.

The {func}`miral::MirRunner::run_with` allows you to tailor your compositor to
your specific requirements: You customise any components you wish to incorporate
(or write new ones) and pass them to {func}`miral::MirRunner::run_with`.

In addition to starting and stopping your Wayland compositor, {class}`miral::MirRunner`
provides a hooks for co-ordinating other processing you need by running code:
* when the server has started: {func}`miral::MirRunner::add_start_callback`;
* before the server stops: {func}`miral::MirRunner::add_stop_callback`;
* when a signal is received: {func}`miral::MirRunner::register_signal_handler`; and
* when a file descriptor becomes readable: {func}`miral::MirRunner::register_fd_handler`.

### miral::WindowManagementPolicy

This is an important way to be able to customise your compositor. Changing the 
way that windows are managed affects the way users interact with your compositor.

The "Hello World" example uses {class}`miral::MinimalWindowManager` which is an
implementation of the {class}`miral::WindowManagementPolicy` interface. This 
interface is how you can specify your chosen window management approach.

Three of our downstream projects implement their own window management policies
that take very different approaches to window management. Each of them provides
a custom implementation of the {class}`miral::WindowManagementPolicy` interface.

* [Ubuntu Frame](https://github.com/canonical/ubuntu-frame) provides a fullscreen experience
* [Miriway](https://github.com/Miriway/Miriway) offers traditional "floating windows" with support for workspaces 
* [miracle-wm](https://github.com/miracle-wm-org/miracle-wm) features a tiling window manager

The {class}`miral::WindowManagementPolicy` interface has two important types of 
function:

* `handle_...` functions expect the policy to take action in response; and,
* `advise_...` functions notify the policy that something is happening.

### miral::WindowManagerTools

When implementing a {class}`miral::WindowManagementPolicy` you will need
the functions provided by {class}`miral::WindowManagerTools`. This lets you
direct the handling of windows. It is supplied to your window management policy
constructor for later use.

### Other MirAL components

Most of the remaining MirAL components are designed to be passed into 
{func}`miral::MirRunner::run_with`. They offer a range of capabilities including:
* Adding your own configuration options: {class}`miral::ConfigurationOption`;
* Supporting X11 applications: {class}`miral::X11Support`;
* Launching apps: : {class}`miral::ExternalClientLauncher`;
* Controlling the keyboard layout: {class}`miral::Keymap`;
* Controlling the available Wayland extension protocols: {class}`miral::WaylandExtensions`; and,
* Selecting server-side or client-side decorations: {class}`miral::Decorations`.

## Further reading

You can explore the full API in the - [API reference](/api/library_root)
