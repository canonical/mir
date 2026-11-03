# Knowledge Base

- [.kb/agents.md](.kb/agents.md) - Structure and rules of the knowledge base. You MUST read this file.
- [src/server/.kb/mir-server.md](src/server/.kb/mir-server.md) - Server startup, shutdown, and ownership.
- [src/platforms/.kb/kms-display-invariants.md](src/platforms/.kb/kms-display-invariants.md) - KMS invariants shared by atomic-kms and gbm-kms (FB lifetime, cursor, compatibility, quirks).
- [src/platforms/atomic-kms/.kb/atomic-kms-platform.md](src/platforms/atomic-kms/.kb/atomic-kms-platform.md) - Atomic-kms specific commit model, probing, and format handling.
- [src/platforms/gbm-kms/.kb/gbm-kms-platform.md](src/platforms/gbm-kms/.kb/gbm-kms-platform.md) - GBM-KMS specific page-flip protocol, output state machine, and probing.
- [src/server/.kb/mir-glib-main-loop.md](src/server/.kb/mir-glib-main-loop.md) - GLib main loop dispatch, teardown guarantees, and alarm semantics.
- [src/server/.kb/mir-shm-backing.md](src/server/.kb/mir-shm-backing.md) - SHM backing store SIGBUS handling and size guarantees.
- [src/server/.kb/mir-signal-and-termination.md](src/server/.kb/mir-signal-and-termination.md) - Fatal-signal interception, termination exception flow, and extension policy.
- [src/server/compositor/.kb/mir-compositor.md](src/server/compositor/.kb/mir-compositor.md) - Compositor threading, buffer arbitration, bypass, and occlusion.
- [src/server/console/.kb/console-services.md](src/server/console/.kb/console-services.md) - Console providers, VT switching, logind, and session lock.

Existing [Copilot instructions](.github/copilot-instructions.md) and [path instructions](.github/instructions/)
remain authoritative for their existing concerns. [Sphinx documentation](doc/sphinx/) remains authoritative for published docs.
