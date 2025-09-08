# Using Mir to build a Desktop Environment

A Desktop Environment comprises a lot more than a compositor. A Desktop
Environment typically has launchers, panels, integration with the system 
greeter and other services.

Mir can help you build a compositor, but these other elements need to come
from elsewhere. And they need to be integrated with your compositor.

In the following, example implementations are taken from downstream projects:

Project | Description
   --   |   --
[Miriway](https://github.com/Miriway/Miriway)|Miriway is a generic compositor using Mir that can be customised to serve the needs of a range of desktop environments
[Ubuntu Frame](https://github.com/canonical/ubuntu-frame)|A simple fullscreen shell used for kiosks, industrial displays, digital signage, smart mirrors, etc.

## Wayland extensions

The communication between the compositor and other programs is substantially
through Wayland extension protocols. Each extension protocol addresses a 
specific interaction: drawing a window, cut-and-paste, panel placement, 
and so on.  

### Bespoke Wayland extensions

It may be necessary to implement Wayland extensions that are not directly
supported by Mir. These could be extensions that are specific to your Desktop
Environment, or ones that cannot be integrated directly into Mir. (See: [How to integrate a custom Wayland protocol](../how-to/how-to-integrate-a-custom-wayland-protocol.md))

### Privileged Wayland extensions

Mir does not automatically make all Wayland extensions available to all
Wayland clients. Some protocol extensions are "privileged" and only 
intended for clients that meet some trust criteria.

In your compositor you can enable access to the privileged Wayland 
extensions using the `miral::WaylandExtensions` APIs. There is an example
of this in _Miriway_.

```c++
    // Change the default Wayland extensions to:
    //  1. to enable screen capture; and,
    //  2. to allow pointer confinement (used by, for example, games)
    // Because we prioritise `user_preference()` these can be disabled by the configuration
    WaylandExtensions extensions;
    for (auto const& protocol : {
        WaylandExtensions::zwlr_screencopy_manager_v1,
        WaylandExtensions::zxdg_output_manager_v1,
        "zwp_idle_inhibit_manager_v1",
        "zwp_pointer_constraints_v1",
        "zwp_relative_pointer_manager_v1"})
    {
        extensions.conditionally_enable(protocol, [&](WaylandExtensions::EnableInfo const& info)
            {
                return info.user_preference().value_or(true);
            });
    }
...
    // Protocols we're reserving for shell components_option
    for (auto const& protocol : {
        WaylandExtensions::zwlr_layer_shell_v1,
        WaylandExtensions::zwlr_foreign_toplevel_manager_v1})
    {
        child_control.enable_for_shell(extensions, protocol);
    }

    ConfigurationOption shell_extension{
        [&](std::vector<std::string> const& protocols)
        { for (auto const& protocol : protocols) child_control.enable_for_shell(extensions, protocol); },
    "shell-add-wayland-extension",
    "Additional Wayland extension to allow shell processes (may be specified multiple times)"};

```

### Identifying clients to "qualify" them to use privileged extensions

It is up to you how you identify the clients with access to Privileged Wayland 
extensions. But there is prior art:

In Ubuntu Frame, AppArmor is queried to identify the snap that contains the 
client and checks that against a list of trusted snaps:

```c++
    else if (aa_getpeercon(app_fd, &label_cstr, &mode_cstr) < 0)
    {
        ...
```

In Miriway, trusted "shell components" have to be `fork()/exec()`d 
by Miriway, and are identified by PID. There's a `shell-component` 
configuration option to allow these to be specified by the Desktop
Environment using Miriway.