Introducing the Miral API {#introducing_the_miral_api}
=========================

The main() program
------------------

The main() program from miral-shell looks like this:

\include shell_main.cpp

This shell is providing FloatingWindowManagerPolicy, TilingWindowManagerPolicy
and SpinnerSplash. The rest is from MirAL.

If you look for the corresponding code in lp:qtmir and lp:mir you’ll find it
less clear, more verbose and scattered over multiple files.

A shell has to provide a window management policy (miral-shell provides two: 
FloatingWindowManagerPolicy and TilingWindowManagerPolicy). A window management
policy needs to implement the \ref miral::WindowManagementPolicy interface for
handling a set of window management events.

The way these events are handled determines the behaviour of the shell.

The \ref miral::WindowManagerTools interface provides the principle methods for
a window management policy to control Mir. 
