Debug for XMir {#debug_for_xmir}
=================

What is XMir ?
----------------

In order to help with debugging, it's important to understand what XMir is
and what bugs are likely to occur. XMir is not a replacement to X and does
not alter X or compiz window management functionality in the current Ubuntu
stack. XMir is an addition to the system. XMir enables Mir to be
employed as a system compositor under the X stack, in this configuration
Mir composites the greeter and the desktop sessions.

XMir is currently targeted at the Ubuntu 13.10 release as being the default
configuration. It currently supports open source graphics drivers based on
Mesa, Nouveau and Radeon. Once XMir is installed and properly configured,
upon boot or restart of the LightDM daemon, Mir will determine if there
is proper driver support; in the instance where proprietary drivers not
supporting the Mir driver model are detected, LightDM will continue to
boot into the standalone X configuration without error. From a visual
perspective on Ubuntu desktop, XMir and standalone X are identical (note,
currently there is an X cursor present on XMir but this soon be disabled,
making the experience truly identical and indiscernible without checking on
what processes are running, see \ref using_mir_on_pc).

How to toggle between XMir and standalone X ?
-------------------------------------

In order to return to a standalone X configuration, comment out the following
line in /etc/lightdm/lightdm.conf.d/10-unity-system-compositor.conf to look to
look like this:

    [SeatDefaults]
    #type=unity

Simply reboot or restart LightDM.
To reverse the process, simply comment the line back in to enable XMir.

Most common bugs
-------------------------------------

At the moment, the most common bugs are related to failures to boot into XMir.
Due to the robust fallback mechanisms in place, most failures will result in
completing the boot sequence standalone X configuration, when you expected
XMir.

1. First check your /etc/lightdm/lightdm.conf.d/10-unity-system-compositor.conf
   has type=unity properly enabled.

2. When logging a bug, please include the log files contained in the
   /var/log/lightdm/ directory. Note, it is good to inspect the last update to
   these files to see if they were written to in your boot attempt,
   obviously if they were not updated their contents are irrelevant.
   The most important of these files is likely the lightdm.log and
   unity-system-compositor.log. You are more than welcome to inspect these
   files, frequently there will be an error statement in
   unity-system-compositor.log which can be traced back into the source code
   for further debug.

3. If you have the experience of booting to a black screen which seems frozen,
   you may attempt to switch VT's by selecting <Ctrl+Alt+F8> to see if there is
   a UI present. Alternatively, <Ctrl+Alt+F1>, login at the console, copy out
   the log files mentioned above, then toggle back to standalone X as described
   above and rebooting.

Other bugs
-------------------------------------

If you experience a UI lock up or a crash, it would be helpful to double check
for X backtracing which can be found here: https://wiki.ubuntu.com/X/Debugging

Note, it is highly unlikely you will see any visual corruption due to XMir. If
you experience a visual corruption during use, it is most likely that it a bug
that exists in the standalone X Ubuntu configuration. Please attempt to toggle
back to the standalone X configuration and replicate the use case. If you
succeed in only being able to repeat the visual corruption in XMir and not in
standalone X, please file a bug with a detailed description of hardware used,
XMir component versions used and the use case steps. You may check the versions
of key XMir components by the following

    dpkg -s libmirclient3 | grep Version
    dpkg -s libmirserver0 | grep Version
    dpkg -s lightdm | grep Version
    dpkg -s unity-system-compositor | grep Version

