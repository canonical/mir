Demo Server {#demo_server}
===========

Mir's demo server (`mir_demo_server`) is an example of using the Mir to
build a server. It uses only functionality supported by the public Mir API.

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

The following operations are supported:

 - Quit (shut down the Mir server): *Ctrl-Alt-Backspace*
 - Switch back to X: *Ctrl-Alt-F7*
 - Switch virtual terminals (VTs): *Ctrl-Alt-(F1-F12)*
 - Switch apps: *Alt-Tab*, tap or click on the corresponding app
 - Close app: *Alt-F4*
 - Switch window within app: *Alt-`*, tap or click on the window
 - Close surface: *Ctrl-F4*
 - Move window: *Alt-leftmousebutton* drag
 - Resize window: *Alt-middle_button* drag
 - Maximize/restore current window: Alt-F11
 - Maximize/restore current window: Shift-F11
 - Maximize/restore current window: Ctrl-F11

For those writing client code request to set the surface attribute
`mir_surface_attrib_state` are honoured:
 - `mir_surface_state_restored`: restores the window 
 - `mir_surface_state_maximized`: maximizes size
 - `mir_surface_state_vertmaximized`: maximizes height
 - `mir_surface_state_horizmaximized`: maximizes width

For a quick demo try:

    sudo DISPLAY= mir_demo_server --vt 1 --launch bin/mir_demo_client_egltriangle\
     --test-client bin/mir_demo_client_multiwin --test-timeout 60

(Remember to unwrap the line)

### Tiling Window Manager

One option that needs elaboration is "--window-manager tiling".

This starts a (rather primitive) tiling window manager. It tracks the available
displays and splits the available workspace into "tiles" (one per client).

For a quick demo try:

    sudo DISPLAY= mir_demo_server --vt 1 --launch bin/mir_demo_client_egltriangle\
     --test-client bin/mir_demo_client_multiwin --test-timeout 60\
     --window-manager tiling

(Remember to unwrap the line)

Want more? Log your requests at: https://bugs.launchpad.net/mir/+filebug
