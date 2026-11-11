# Knowledge Base

IMPORTANT: Read the `.kb/agents.md` file.

- [.kb/agents.md](.kb/agents.md) - Structure and rules of the knowledge base. You MUST read this file.
- [src/server/.kb/mir-server.md](src/server/.kb/mir-server.md) - Server startup, shutdown, and ownership.
- [src/miral/.kb/miral-basic-window-manager.md](src/miral/.kb/miral-basic-window-manager.md) - BasicWindowManager locking, MRU/focus rules, zones and placement.
- [src/miral/.kb/miral-window-management-model.md](src/miral/.kb/miral-window-management-model.md) - MirAL window-management model: lock contract, Window identity, resize rules.
- [src/platforms/.kb/kms-display-invariants.md](src/platforms/.kb/kms-display-invariants.md) - KMS invariants shared by atomic-kms and gbm-kms (FB lifetime, cursor, compatibility, quirks).
- [src/platforms/atomic-kms/.kb/atomic-kms-platform.md](src/platforms/atomic-kms/.kb/atomic-kms-platform.md) - Atomic-kms specific commit model, probing, and format handling.
- [src/platforms/gbm-kms/.kb/gbm-kms-platform.md](src/platforms/gbm-kms/.kb/gbm-kms-platform.md) - GBM-KMS specific page-flip protocol, output state machine, and probing.
- [src/server/.kb/mir-glib-main-loop.md](src/server/.kb/mir-glib-main-loop.md) - GLib main loop dispatch, teardown guarantees, and alarm semantics.
- [src/server/.kb/mir-shm-backing.md](src/server/.kb/mir-shm-backing.md) - SHM backing store SIGBUS handling and size guarantees.
- [src/server/.kb/mir-signal-and-termination.md](src/server/.kb/mir-signal-and-termination.md) - Fatal-signal interception, termination exception flow, and extension policy.
- [src/server/compositor/.kb/mir-compositor.md](src/server/compositor/.kb/mir-compositor.md) - Compositor threading, buffer arbitration, bypass, and occlusion.
- [src/server/console/.kb/console-services.md](src/server/console/.kb/console-services.md) - Console providers, VT switching, logind, and session lock.
- [src/server/frontend_wayland/.kb/wayland-connector-lifecycle.md](src/server/frontend_wayland/.kb/wayland-connector-lifecycle.md) - WaylandConnector publication order, teardown, and client authorization.
- [src/server/frontend_wayland/.kb/wayland-executor-lifecycle.md](src/server/frontend_wayland/.kb/wayland-executor-lifecycle.md) - WaylandExecutor workqueue ownership and thread affinity.
- [src/server/frontend_wayland/.kb/wayland-surface-and-input.md](src/server/frontend_wayland/.kb/wayland-surface-and-input.md) - wl_surface role/commit rules, input translation, and protocol state machines.
- [src/server/graphics/.kb/graphics-display-multiplexing.md](src/server/graphics/.kb/graphics-display-multiplexing.md) - Composite display configuration, rollback, and observer replay.
- [src/server/graphics/.kb/graphics-platform-selection.md](src/server/graphics/.kb/graphics-platform-selection.md) - Graphics platform probing, selection order, and special cases.
- [src/server/input/.kb/input-device-hub.md](src/server/input/.kb/input-device-hub.md) - Input device hub lifecycle, configuration persistence, and change transactions.
- [src/server/input/.kb/input-dispatch-chain.md](src/server/input/.kb/input-dispatch-chain.md) - Input dispatcher chain composition, key repeat, and timestamps.
- [src/server/input/.kb/seat-input-state.md](src/server/input/.kb/seat-input-state.md) - Seat-wide input state, cursor control, and observer replay.
- [src/server/scene/.kb/scene-idle-and-input-hubs.md](src/server/scene/.kb/scene-idle-and-input-hubs.md) - Idle hub state machine, text-input serials, and clipboard locking.
- [src/server/scene/.kb/scene-sessions-and-display-config.md](src/server/scene/.kb/scene-sessions-and-display-config.md) - Session lifetime/order and display-configuration application semantics.
- [src/server/scene/.kb/scene-surface-stack.md](src/server/scene/.kb/scene-surface-stack.md) - SurfaceStack layering, focus order, occlusion, and locking.

Existing [Copilot instructions](.github/copilot-instructions.md) and [path instructions](.github/instructions/)
remain authoritative for their existing concerns. [Sphinx documentation](doc/sphinx/) remains authoritative for published docs.
