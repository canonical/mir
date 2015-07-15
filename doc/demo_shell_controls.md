Demo Shell Controls {#demo_shell_controls}
===================

Mir's demo shell (`mir_proving_server`) is a basic environment for testing
Mir server features as well as running natively ported apps/toolkits. It is
still primitive and requires some explaining to make proper use of, so read
on.

Running Demo Shell
------------------

Remember to always run `mir_proving_server` as root on PC (not required on
Android), as this is required for input device support (open bug
https://bugs.launchpad.net/mir/+bug/1286252);

    sudo mir_proving_server

And if you're not already on the VT you wish to use, that needs to be
specified:

    sudo mir_proving_server --vt 1

There are plenty more options available if you run:

    mir_proving_server --help

Controls
--------

All controls have a keyboard/mouse combination and where possible also have
touch alternatives for phones/tablets:

 - Quit the shell (shut down the Mir server): *Ctrl-Alt-Backspace*
 - Switch back to X: *Ctrl-Alt-F7*
 - Switch virtual terminals (VTs): *Ctrl-Alt-(F1-F12)*
 - Switch apps: *Alt-Tab* or *4-finger swipe left/right*
 - Switch windows of the current app: *Alt-grave*
 - Ask an app to close: *Alt-F4* (it may refuse or not understand)
 - Move window: *Alt-leftmousebutton* or *3-finger drag*
 - Resize window: *Alt-middlemousebutton* or *3-finger pinch/zoom*
 - Toggle negative rendering: *Super-N*
 - Toggle high-contrast rendering: *Super-C*
 - Sleep/wake all displays: *Alt-P* or *Android power button*
 - Rotate the focussed monitor: *Ctrl-Alt-(Left/Right/Up/Down)* or
   *Volume up/down while touching the screen*
 - Change display mode of the focussed monitor: *Ctrl-Alt-(=/-)*
 - Reset display mode to default, on focussed monitor: *Ctrl-Alt-0*
 - Adjust window opacity/alpha: *Alt-mousewheel*
 - Zoom in/out: *Super-mousewheel*

*Super* means the Windows key on a PC, or the search key on a Chromebook.

Want more? Log your requests at: https://bugs.launchpad.net/mir/+filebug
