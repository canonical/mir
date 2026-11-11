# Preface

This file covers the console and session-management services in `src/server/console/`.
It explains how Mir selects a provider, and how the Linux VT and logind backends manage
VT ownership, DRM master, and input devices.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - VT switch acknowledgement

- The `SIGUSR1` handler must either ACK the switch through `VT_RELDISP` or die, otherwise the kernel waits forever. `scope_exit` guarantees the ACK, and the release path defaults to allowing the switch so an exception still lets the user escape.

## §2 - VT mode restore

- `restore_vt` reapplies `VT_SETMODE` only when the saved mode was `VT_AUTO`. `VT_PROCESS` is bound to the calling process, so restoring it will rebind the signals to us and corrupt the previous owner's state.


# Concepts

## Provider selection

- [`the_console_services()`](../default_configuration.cpp) picks the provider once and caches it in `console_services` (explicitly noted as not thread-safe).
- `--vt` is valid only with `--console-provider=vt`. Every other provider (including `auto`) throws `AbnormalExit` when `vt != 0`.
- `auto` skips logind when `DISPLAY` is set (X can already own the VT). It then tries logind → Linux VT → `MinimalConsoleServices`, catching failures at each step.

## Linux VT

- [`LinuxVirtualTerminal`](../linux_virtual_terminal.h) declares `prev_active_vt` before `vt_fd` because `open_vt()` runs during `vt_fd` construction and writes it.
- `open_vt()` activates a requested VT only after leaving its process group and calling `setsid()` (guarded by an "already session leader" check). `find_active_vt_number()` probes `/dev/tty` then `/dev/tty0`, read then write.
- `restore()` suspends all active devices before handing the VT back. It sets `prev_active_vt = 0` so a later `restore()` (for example, from the destructor) does not switch back to the previous VT again.
- `register_switch_handlers()` installs `VT_PROCESS` mode with `SIGUSR1` for both acquire and release.

## Devices

- In [`DeviceList`/`Device`](../linux_virtual_terminal.cpp), devices self-register on construction and remove themselves on destruction. The list holds raw `Device*` under `Synchronised`, and `Device` holds a `weak_ptr` back. `DeviceListIterable` keeps the lock for the whole iteration.
- The system claims DRM master at construction, drops it on suspend, and reclaims it on activate (`drmSetMaster` is idempotent). Evdev suspend uses `EVIOCREVOKE` (best-effort, logged) and closes the FD.

## logind

- [`LogindConsoleServices`](../logind_console_services.cpp) is a `scene::SessionLockObserver`, registered through `register_interest`. Logind `lock` and `unlock` signals call `session_lock->lock()` and `unlock()`. In addition, `on_lock` and `on_unlock` set the logind locked hint. The system seeds `active` from the session state starting with `"active"`.
- GDBus proxies use the thread-default main context at creation. Therefore, proxy creation and asynchronous main-loop calls are wrapped in `run_with_context_as_thread_default`. Synchronous calls (`TakeControl`/`SetType`, `ReleaseControl`, and non-running-loop acquire/release paths) are made directly.
- GDBus signals cannot carry FDs, which causes the `MIR_GDBUS_SIGNALS_SUPPORT_FDS` split. The system uses either a `resume-device` handler or a manual `g_dbus_connection_add_filter` that extracts the FD from the message unix-fd list.
- The system ignores `PauseDevice("gone")` for DRM (major 226) because logind sends it spuriously after the second `TakeDevice` during platform construction. For non-DRM devices, the system removes the device and calls `ReleaseDevice` despite the "gone" promise.
- `acquired_devices` is keyed by `dev_t`, and a duplicate acquire throws. Release is asynchronous when the main loop is running, and synchronous otherwise.
