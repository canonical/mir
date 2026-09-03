---
applyTo: include/miral/miral/live_config*.h,include/miral/miral/config_file.h,src/miral/live_config*,src/miral/config_file*
description: MirAL live configuration system types.
---

# Configuration system

MirAL includes a live configuration system under active development. Key types (all in
`include/miral/miral/`):

- **`miral::live_config::Store`** / **`miral::live_config::Key`**: the backend-agnostic interface for registering typed configuration attributes (int, bool, float, string, and array variants) by key, with handlers invoked on load/reload (MirAL 5.5).
- **`miral::live_config::IniFile`**: an INI-file-backed `Store` implementation (MirAL 5.5).
- **`miral::live_config::IniFileWithOverrides`**: an INI `Store` that aggregates a base file plus override (drop-in) files in one transaction — non-array keys are last-writer-wins, array keys accumulate (MirAL 5.8).
- **`miral::ConfigFile`**: locates (via the XDG Base Directory spec) and optionally monitors a config file and its `.d/` overrides for reload-on-change (MirAL 5.1).
