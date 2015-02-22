Demo Server {#demo_server}
===========

Mir's demo server (`mir_demo_server`) is an example of using the Mir to
build a server. In contrast to mir_proving_server (see \ref demo_shell_controls)
it uses only functionality supported by the public Mir API.

Running Demo Server
-------------------

Remember to always run `mir_demo_server` as root on PC (not required on
Android), as this is required for input device support (open bug
https://bugs.launchpad.net/mir/+bug/1286252);

    sudo mir_demo_server

And if you're not already on the VT you wish to use, that needs to be
specified:

    sudo mir_demo_server --vt 1

There are plenty more options available if you run:

    mir_demo_server --help

Tiling Window Manager
---------------------

One option that needs elaboration is "--window-manager tiling".

This starts a (rather primitive) tiling window manager. It tracks the available
displays and splits the available workspace into "tiles" (one per client).

The following operations are supported:

 - Quit (shut down the Mir server): *Ctrl-Alt-Backspace*
 - Switch back to X: *Ctrl-Alt-F7*
 - Switch virtual terminals (VTs): *Ctrl-Alt-(F1-F12)*
 - Switch apps: tap or click on the corresponding tile
 - Move window: *Alt-leftmousebutton* drag
 - Resize window: *Alt-middle_button* drag
 - Maximize/restore current window (to tile size): Alt-F11
 - Maximize/restore current window (to tile height): Shift-F11
 - Maximize/restore current window (to tile width): Ctrl-F11

For those writing client code request to set the surface attribute
`mir_surface_attrib_state` are honoured:
 - `mir_surface_state_restored`: restores the window 
 - `mir_surface_state_maximized`: maximizes to tile size
 - `mir_surface_state_vertmaximized`: maximizes to tile height

For a quick demo try:

    sudo mir_demo_server --vt 1 --display-config sidebyside\
     --window-manager tiling --launch bin/mir_demo_client_egltriangle\
     --test-client bin/mir_demo_client_multiwin --test-timeout 60

(Remember to unwrap the line)

Want more? Log your requests at: https://bugs.launchpad.net/mir/+filebug
