# Knowledge Base

IMPORTANT: Read the `.kb/agents.md` file.

- [.kb/agents.md](.kb/agents.md) - Structure and rules of the knowledge base. You MUST read this file.
- [src/server/.kb/mir-server.md](src/server/.kb/mir-server.md) - Server startup, shutdown, and ownership.
- [src/server/.kb/mir-glib-main-loop.md](src/server/.kb/mir-glib-main-loop.md) - GLib main loop dispatch, teardown guarantees, and alarm semantics.
- [src/server/.kb/mir-shm-backing.md](src/server/.kb/mir-shm-backing.md) - SHM backing store SIGBUS handling and size guarantees.
- [src/server/.kb/mir-signal-and-termination.md](src/server/.kb/mir-signal-and-termination.md) - Fatal-signal interception and termination exception flow.
- [src/server/compositor/.kb/mir-compositor.md](src/server/compositor/.kb/mir-compositor.md) - Compositor threading, buffer arbitration, bypass, and occlusion.
- [src/server/console/.kb/console-services.md](src/server/console/.kb/console-services.md) - Console providers, VT switching, logind, and session lock.

Bug references of the form `LP: #1237332` point to Launchpad bugs, at
`https://bugs.launchpad.net/bugs/1237332`. Mir's development originally took place on Launchpad, so
older comments and knowledge-base entries cite bugs there rather than on GitHub.

Existing [Copilot instructions](.github/copilot-instructions.md) and [path instructions](.github/instructions/)
remain authoritative for their existing concerns. [Sphinx documentation](doc/sphinx/) remains authoritative for published docs.
