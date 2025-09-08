# Using Mir to build a Desktop Environment

A Desktop Environment comprises a lot more than a compositor. A Desktop
Environment typically has launchers, panels, integration with the system 
greeter and other services.

Mir can help you build a compositor, but these other elements need to come
from elsewhere. And they need to be integrated with your compositor.

In the following, example implementations are taken from downstream projects:

Project | Description
   --   |   --
[miracle-wm](https://github.com/miracle-wm-org/miracle-wm)|a Wayland compositor based on Mir. It features a tiling window manager at its core, very much in the style of i3 and sway
[Ubuntu Frame](https://github.com/canonical/ubuntu-frame)|A simple fullscreen shell used for kiosks, industrial displays, digital signage, smart mirrors, etc.
[Miriway](https://github.com/Miriway/Miriway)|Miriway is a generic compositor using Mir that can be customised to serve the needs of a range of desktop environments

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
of this in miracle-wm.

```c++
    WaylandExtensions wayland_extensions = WaylandExtensions {}
                                               .enable(WaylandExtensions::zwlr_layer_shell_v1)
                                               .enable(WaylandExtensions::zwlr_foreign_toplevel_manager_v1)
                                               .enable(WaylandExtensions::zxdg_output_manager_v1)
                                               .enable(WaylandExtensions::zwp_virtual_keyboard_manager_v1)
                                               .enable(WaylandExtensions::zwlr_virtual_pointer_manager_v1)
                                               .enable(WaylandExtensions::zwp_input_method_manager_v2)
                                               .enable(WaylandExtensions::zwlr_screencopy_manager_v1)
                                               .enable(WaylandExtensions::ext_session_lock_manager_v1);
```

### Identifying clients to "qualify" them to use privileged extensions

It is up to you how you identify the clients with access to Privileged Wayland 
extensions. But there is prior art:

#### In Ubuntu Frame
In Ubuntu Frame, AppArmor is queried to identify the snap that contains the 
client and checks that against a list of trusted snaps:

```c++
    else if (aa_getpeercon(app_fd, &label_cstr, &mode_cstr) < 0)
    {
        ...
```

#### In Miriway
In Miriway, trusted "shell components" have to be `fork()/exec()`d 
by Miriway, and are identified by PID, this is handled by 
`miriway::ChildControl`:  

```c++
void miriway::ChildControl::enable_for_shell(WaylandExtensions& extensions, std::string const& protocol)
{
    extensions.conditionally_enable(protocol, self->enable_for_shell_pids);
}
```

In addition, There's a `shell-component` configuration option to allow 
these programs to be specified by a Desktop Environment using Miriway.
