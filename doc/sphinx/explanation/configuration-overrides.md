---
myst:
  html_meta:
    description: How Mir's ConfigFile resolves XDG directories, orders override files, and merges INI values.
---

(configuration-overrides)=

# Configuration overrides in Mir

`miral::ConfigFile` and `miral::live_config::IniFileWithOverrides` together implement a
layered configuration system: a base file plus zero or more drop-in override files, loaded
in a single transaction and optionally reloaded on change.

This pattern lets distributors, site administrators, and users each supply a fragment
without modifying each other's files.

## File discovery and XDG roots

`ConfigFile` accepts either a bare filename or a path.

**Bare filename** (e.g., `"myapp.conf"`): `ConfigFile` searches the following roots in
priority order and uses the highest-priority root that contains the file:

1. `$XDG_CONFIG_HOME` (falls back to `$HOME/.config`)
1. Each entry in `$XDG_CONFIG_DIRS` (falls back to `/etc/xdg`)

**Path with a parent directory** (e.g., `/etc/myapp/myapp.conf`): the parent directory
is used as the first root; `$XDG_CONFIG_DIRS` entries are still appended.

Override files are always collected from `<base-filename>.d/` subdirectories under
**every** root, not just the root that provided the base file. Only files whose extension
matches the filter passed to `ConfigFile` are included.

## Override ordering and precedence

1. The base file is loaded first.
1. Override files are collected from all roots and deduplicated by basename: if two roots
   provide a file with the same basename, the higher-priority root's copy is used.
1. The resulting set is sorted lexicographically by basename and applied in
   that order. Taking whole number prefixes into account.

Using numeric prefixes (e.g., `10-site.conf`, `90-user.conf`) is a conventional
way to control ordering predictably. Our sorting works such that `5-foo.conf` \<
`10-bar.conf`. Pure lexicographic sorting would place `10-bar.conf` before
`5-foo.conf`.

When files are added or removed at runtime, priority is re-evaluated from scratch on the
next reload.

## Merge semantics

`IniFileWithOverrides` applies these rules across the ordered list of files:

| Key type                                  | Rule                                                                                                                                                                                                                           |
| ----------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Scalar (`int`, `bool`, `float`, `string`) | Last file to set the key wins.                                                                                                                                                                                                 |
| Array (`ints`, `floats`, `strings`)       | Values accumulate across files.                                                                                                                                                                                                |
| Array with empty assignment (`key=`)      | Clears all previously accumulated values; subsequent entries in the same or later files start fresh.                                                                                                                           |
| Key absent from all files                 | Handler receives the preset value (if registered with one), otherwise `std::nullopt`.                                                                                                                                          |
| Invalid scalar value                      | Handler receives the preset value (if registered with one), otherwise `std::nullopt`. A later invalid value therefore supersedes any earlier valid value.                                                                      |
| Invalid array entry                       | The invalid entry is ignored (a warning is logged); valid entries in the same file are kept. If **all** entries in a transaction are invalid, the handler receives the preset value (if registered), otherwise `std::nullopt`. |

All attribute handlers fire before `on_done`, and both fire exactly once per `load()`
call.

## Reload model

With `ConfigFile::Mode::reload_on_change`, `ConfigFile` monitors the relevant directories
via inotify. On any change, it re-evaluates the full file list and calls the
`OverrideLoader` callback with an `OverridesList` classifying each file as:

- **unchanged** — present in both old and new state, content unchanged
- **fresh** — new file (added or first seen)
- **modified** — present before and after, content changed
- **dropped** — was present, no longer found

## Relationship between `ConfigFile` and the live-config store

`ConfigFile` handles **discovery and monitoring**: it locates files using XDG rules,
assembles an `OverridesList`, and fires a callback on change.

`IniFileWithOverrides` (or `IniFile` for single-file use) handles **parsing and merging**:
it takes an `OverridesList`, parses each file as INI, and dispatches typed attribute
handlers.

The two are composed explicitly:

```cpp
miral::live_config::IniFileWithOverrides config;

miral::ConfigFile config_file{
    runner,
    "myapp.conf",
    miral::ConfigFile::Mode::reload_on_change,
    [&config](miral::live_config::OverridesList const& changes) { config.load(changes); },
    ".conf"};
```

Use `IniFile` instead of `IniFileWithOverrides` when you have a single config file with no
drop-in overrides (`ConfigFile` with the single-file `Loader` constructor).

## See also

- {ref}`how-to-add-configuration-overrides`
