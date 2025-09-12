# Introducing the MirAL API

## The main() program

The main() program from miral-shell looks like this:

```{literalinclude} ../../../examples/miral-shell/shell_main.cpp
```

This shell is providing {class}`FloatingWindowManagerPolicy`, {class}`TilingWindowManagerPolicy`
and {class}`SpinnerSplash`. The rest is from MirAL.

If you look for the corresponding code in lp:qtmir and lp:mir you’ll find it
less clear, more verbose and scattered over multiple files.

A shell has to provide a window management policy (miral-shell provides two:
{class}`FloatingWindowManagerPolicy` and {class}`TilingWindowManagerPolicy`). A window management
policy needs to implement the {class}`miral::WindowManagementPolicy` interface for
handling a set of window management events.

The way these events are handled determines the behavior of the shell.

The {class}`miral::WindowManagerTools` interface provides the principle methods for
a window management policy to control Mir.
