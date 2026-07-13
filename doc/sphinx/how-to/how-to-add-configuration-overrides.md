---
myst:
  html_meta:
    description: How to add reloadable, drop-in configuration overrides to a Mir compositor.
---

(how-to-add-configuration-overrides)=

# How to add configuration overrides in Mir

This guide shows how to load a base configuration file and drop-in override
files, with automatic reload on change, using {class}`miral::ConfigFile` and
`miral::live_config::IniFileWithOverrides`.

Requires MirAL 5.9 for the override-aware `ConfigFile` constructor;
`IniFileWithOverrides` is available since MirAL 5.8.

**Note**: [Write Your First Wayland Compositor](../tutorial/write-your-first-wayland-compositor.md)
is a prerequisite for this how-to.

______________________________________________________________________

## Choosing an approach

`ConfigFile` provides two constructors, each paired with a `live_config::Store`
implementation that knows how to parse the stream(s) it is handed:

- **Single file (`ConfigFile::Loader` + `live_config::IniFile`)** —
  `ConfigFile` calls your loader functor with an open stream of the base
  file, and `IniFile` parses it as INI. Use this when you want INI parsing
  but no drop-in overrides.
- **Base + drop-in overrides (`ConfigFile::OverrideLoader` +
  `live_config::IniFileWithOverrides`)** — `ConfigFile` collects the base
  file plus any `<name>.d/*<ext>` overrides across XDG roots and delivers
  them through an `OverridesList`; `IniFileWithOverrides` parses and merges
  them in one transaction. Use this for layered, distributor-friendly
  configuration.

For non-INI formats, pair the `Loader` or `OverrideLoader` constructors with your own parser:

```cpp
miral::ConfigFile config_file{
    runner,
    "myapp.conf",
    miral::ConfigFile::Mode::reload_on_change,
    [](std::istream& stream, std::filesystem::path const& path)
    {
        // parse stream...
    }};

// or

miral::ConfigFile config_file{
    runner,
    "myapp.conf",
    miral::ConfigFile::Mode::reload_on_change,
    [](miral::live_config::OverridesList const& changes)
    {
        // ...
    },
    ".conf"};
```

The rest of this guide uses the `OverrideLoader` + `IniFileWithOverrides`
approach.

## Set up the configuration store

First, add the required headers:

```{literalinclude} ./configuration-overrides/main.cpp
---
language: cpp
start-after: '[doc:config-override:includes]'
end-before: '[doc:config-override:includes-end]'
---
```

Create an `IniFileWithOverrides`, register typed attributes, and attach an
`on_done` callback that fires once per load:

```{literalinclude} ./configuration-overrides/main.cpp
---
language: cpp
start-after: '[doc:config-override:store]'
end-before: '[doc:config-override:store-end]'
dedent: 4
---
```

Each attribute handler receives the registered {class}`miral::live_config::Key` and an
`std::optional` value. The value is `std::nullopt` when the key is absent from all files;
`add_string_attribute` also accepts a preset default (here `"black"`) used in that case.

## Attach `ConfigFile`

Construct `ConfigFile` with the override-aware constructor, passing the store's `load`
method as the callback and a file extension to filter override files:

```{literalinclude} ./configuration-overrides/main.cpp
---
language: cpp
start-after: '[doc:config-override:config-file]'
end-before: '[doc:config-override:config-file-end]'
dedent: 4
---
```

Keep `config_file` alive for the compositor lifetime — destroying it stops reload
monitoring.

## Where files live

Given `"demo-compositor.conf"` as the base filename, `ConfigFile` searches these
directories in priority order:

1. `$XDG_CONFIG_HOME` (defaults to `$HOME/.config`)
1. Entries in `$XDG_CONFIG_DIRS` (defaults to `/etc/xdg`)

The base file is loaded from the highest-priority root that contains it. Override
files are collected from `demo-compositor.conf.d/` under **every** root, filtered by
the extension passed to `ConfigFile` (`.conf`). Override files in higher-priority roots
shadow same-named files from lower-priority roots.

Example layout:

```{code-block} text
~/.config/demo-compositor.conf           # base (user)
~/.config/demo-compositor.conf.d/
    90-user.conf                         # user override
/etc/xdg/demo-compositor.conf            # base (system, used if user base absent)
/etc/xdg/demo-compositor.conf.d/
    10-site.conf                         # system override
```

Override files are applied in lexicographic order of their basename. With the layout
above, `10-site.conf` is applied before `90-user.conf`.

A sample `demo-compositor.conf`:

```{literalinclude} ./configuration-overrides/test.sh
---
language: bash
start-after: '[doc:config-override:example-conf]'
end-before: '[doc:config-override:example-conf-end]'
---
```

A sample `demo-compositor.conf.d/90-user.conf` that overrides the background and adds
a workspace:

```{literalinclude} ./configuration-overrides/test.sh
---
language: bash
start-after: '[doc:config-override:example-override]'
end-before: '[doc:config-override:example-override-end]'
---
```

After merging: `display_background` is `blue` (last-writer-wins); `display_workspaces`
is `home,work,gaming` (array values accumulate across files). See
{ref}`configuration-overrides` for the full merge rules.

## Build and try it out

Create a new directory for the project and add these files:

`CMakeLists.txt`:

```{literalinclude} ./configuration-overrides/CMakeLists.txt
---
language: cmake
---
```

`main.cpp`:

```{literalinclude} ./configuration-overrides/main.cpp
---
language: cpp
---
```

Build:

```{literalinclude} ./configuration-overrides/test.sh
---
language: bash
start-after: '[doc:config-override:build]'
end-before: '[doc:config-override:build-end]'
---
```

Set up config files:

```bash
mkdir -p ~/.config/demo-compositor.conf.d

cat > ~/.config/demo-compositor.conf <<'EOF'
display_background=black
display_workspaces=home
display_workspaces=work
EOF

cat > ~/.config/demo-compositor.conf.d/90-user.conf <<'EOF'
display_background=blue
display_workspaces=gaming
EOF
```

Run the compositor:

```bash
WAYLAND_DISPLAY=wayland-99 ./demo-config-override &
```

You should see lines like:

```
config: display_background=blue
config: display_workspaces=home,work,gaming
config: loaded
```

To test reload: edit `~/.config/demo-compositor.conf.d/90-user.conf` and the
compositor will re-read and re-merge all files automatically.

## See also

- {ref}`configuration-overrides` — merge semantics, XDG resolution, and reload model
