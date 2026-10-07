(how-to-update-symbols-maps)=

# How to update symbols map files

The Mir project is a collection of C++ libraries that a consumer
can use to create a Wayland compositor. In order for consumers
of Mir's libraries to be confident in what they're building against, the
symbols of each library are versioned.

To describe these versions, each user-facing library defines a `symbols.map`
file. These files are comprised of version stanzas which contain the symbols
associated with that version. There are `symbols.map` files for the following
libraries, although only the first three are typically used in practice:

- [miral](#how-to-update-miral-symbols)
- [miroil](#how-to-update-miroil-symbols)
- [mirserver](#how-to-update-mirserver-symbols-map)
- mircore
- mircommon
- mirplatform
- [mirwayland](#how-to-update-mirwayland-symbols)

It is tedious to edit these files. Luckily for us, this versioning business
is (mostly) automated so that we have to think very little about it.

## Goal

When you change the public interface of a user-facing library, the goal is to
leave the tree in a state where:

1. Every new symbol is listed in the library's `symbols.map`, in a stanza for
   the current development version.
2. No released stanza is modified: released stanzas describe the ABI that
   consumers already build against, so they must stay byte-for-byte identical.
3. Any version/ABI numbers the build uses (`MIRAL_VERSION_MINOR`,
   `MIRWAYLAND_ABI`, etc.) are bumped exactly as described below.
4. The `check-*-symbols-map` targets (and the
   [Symbols Check CI](https://github.com/canonical/mir/actions/workflows/symbols-check.yml))
   pass for every library they cover.

If you forget to update the `symbols.map` file, Github CI will complain and
prevent the merge until the issue is addressed. Each time a pull request is
open or updated on Github, this action will be run. If you check the log of
this action, you will see which symbols have been changed so that you can
address the issue.

Note that this CI check only covers `miral`, `miroil`, `mirserver` and
`mircommon`. The other libraries with `symbols.map` files (`mirwayland`,
`mircore` and `mirplatform`) have no `generate-*-symbols-map` target and no
CI coverage, so changes to their interfaces must be made by hand in the same
stanza style. The `mirwayland` section
[below](#how-to-update-mirwayland-symbols) walks through that manual workflow
step by step.

## When are symbols.map files updated

Two different circumstances will prompt a developer to update
these files:

1. **A new symbol is added to a library**

   This means that we have extended the existing interface.
   The new symbols can simply be put in a new stanza with a "bumped"
   minor version. Note that the minor version must only be bumped once
   per release cycle. This means that if I add a new symbol two months
   before a release and you add a symbol 1 day before the release, then
   our new symbols will go in the same stanza. In that situation, I
   would have been the one to create the new stanza.

1. **A symbol is changed or removed** (e.g. a new parameter is
   added to a method call, a class is removed, etc.)

   This is an API break. This means that a consumer of the library
   can no longer safely compile against the new version of that library
   without maybe breaking their build. In this scenario, we are forced
   to create a single new stanza at a brand new version in the `symbols.map`
   file. This new stanza will contain all of the symbols in that library.

**Warning**: Please be aware that this action is not sophisticated enough to
know if you changed the definition of a symbol. If you add or remove a symbol,
the action will inform you. However, if you modify a symbol (e.g. by changing
the parameters that a method takes), you will *not* be informed that the symbol
has changes. Such changes will need to be moved to a new stanza manually.

## Setup

Before we can update the symbols of Mir's libraries, we'll want to set
up our environment so that the our tools can work correctly.

To do this, you will first want to install **clang** (the same packages the
Symbols Check CI installs):

```sh
sudo apt install clang libclang-dev python3-clang
```

Afterwards, you will want to set the following environment variables to point
at the path of `libclang.so` and `libclang.so`'s `lib` directory. Note that
these paths are likely to be the same on most Ubuntu 26.04 setups, but may
vary on other distros:

```sh
export MIR_SYMBOLS_MAP_GENERATOR_CLANG_SO_PATH=$( ls /usr/lib/llvm-*/lib/libclang*.so.1 | tail -n1 )
export MIR_SYMBOLS_MAP_GENERATOR_CLANG_LIBRARY_PATH=$( ls -d /usr/lib/llvm-*/lib | tail -n1 )
```

It is recommended that you put those `export ...` commands in your `.bashrc`
so that you don't have to think about it in the future.

And that's it! Now we are ready to update our symbols automatically.

## How to update miral symbols

### Scenario 1: adding a new symbol

1. Make the additive change to the interface (e.g. by adding a new method
   to an existing class)
1. If not already bumped in this release cycle, bump `MIRAL_VERSION_MINOR`
   in `src/CMakeLists.txt`
1. From the root of the project, run:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target generate-miral-symbols-map
   ```
1. Check that your new symbols is in the `symbols.map` file:
   ```sh
   git diff src/miral/symbols.map
   ```
1. Regenerate Debian symbols:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target regenerate-miral-debian-symbols
   ```

### Scenario 2: removing or changing a symbol

1. Make a destructive change to the interface (e.g. remove a parameter from
   a method).
1. If not already bumped in this release cycle, bump `MIRAL_VERSION_MAJOR`
   in `src/CMakeLists.txt`. Set `MIRAL_VERSION_MINOR` and `MIRAL_VERSION_PATCH`
   to `0`
1. If not already bumped in this release cycle, bump `MIRAL_ABI` in
   `src/miral/CMakeLists.txt`
1. From the root of the project, run:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target generate-miral-symbols-map
   ```
1. Check that your symbols are reflected properly in the `symbols.map` file:
   ```sh
   git diff src/miral/symbols.map
   ```
1. Regenerate the debian symbols:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target regenerate-miral-debian-symbols
   ```

If `MIRAL_ABI` needed to be updated, you should also run:

```sh
./tools/update_package_abis.sh
```

## How to update miroil symbols

### Scenario 1: adding a new symbol

1. Make the additive change to the interface (e.g. by adding a new method
   to an existing class)
1. If not already bumped in this release cycle, bump `MIROIL_VERSION_MINOR`
   in `src/miroil/CMakeLists.txt`
1. From the root of the project, run:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target generate-miroil-symbols-map
   ```
1. Check that your new symbols is in the `symbols.map` file:
   ```sh
   git diff src/miroil/symbols.map
   ```

### Scenario 2: removing or changing a symbol

1. Make a destructive change to the interface (e.g. remove a parameter from
   a method).
1. If not already bumped in this release cycle, bump `MIROIL_ABI`
   in `src/miroil/CMakeLists.txt`. Set `MIROIL_VERSION_MINOR` and `MIROIL_VERSION_PATCH`
   to `0`
1. From the root of the project, run:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target generate-miroil-symbols-map
   ```
1. Check that your symbols are reflected properly in the `symbols.map` file:
   ```sh
   git diff src/miroil/symbols.map
   ```
1. Regenerate the debian symbols:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target regenerate-miroil-debian-symbols
   ```

If `MIROIL_ABI` needed to be updated, you should also run:

```sh
./tools/update_package_abis.sh
```

## How to update mirserver symbols map

### Scenario 1: adding a new symbol

1. Make the additive change to the interface (e.g. by adding a new method
   to an existing class)
1. If not already bumped in this release cycle, bump minor number in `project(VERSION)`
   in `CMakeLists.txt`
1. If not already bumped in this release cycle, bump `MIRSERVER_ABI` in
   `src/server/CMakeLists.txt`
1. From the root of the project, run:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target generate-mirserver-symbols-map
   ```
1. Check that your new symbols is in the `symbols.map` file:
   ```sh
   git diff src/server/symbols.map
   ```

If `MIRSERVER_ABI` needed to be updated, you should also run:

```sh
./tools/update_package_abis.sh
```

### Scenario 2: removing or changing a symbol

1. Make a destructive change to the interface (e.g. remove a parameter from
   a method).
1. If not already bumped in this release cycle, bump `MIR_VERSION_MAJOR`
   in `CMakeLists.txt`. Set `MIR_VERSION_MINOR` and `MIR_VERSION_PATCH`
   to `0`
1. If not already bumped in this release cycle, bump `MIRSERVER_ABI` in
   `src/server/CMakeLists.txt`
1. From the root of the project, run:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target generate-mirserver-symbols-map
   ```
1. Check that your symbols are reflected properly in the `symbols.map` file:
   ```sh
   git diff src/server/symbols.map
   ```

If `MIRSERVER_ABI` needed to be updated, you should also run:

```sh
./tools/update_package_abis.sh
```

## How to update mirwayland symbols

Unlike the libraries above, `mirwayland` has no `generate-mirwayland-symbols-map`
target and is not covered by the Symbols Check CI, so its `symbols.map` file
(`src/wayland/symbols.map`) is updated by hand. There is no generator target
because the generator cannot reproduce this map: most of the exported ABI
(the `mir::wayland::*` protocol classes) is declared in `*_wrapper.h` files
generated into the build directory at build time rather than in
`include/wayland`, so a source-header scan would miss them, and the map
intentionally uses class-level wildcards (e.g. `mir::wayland::Buffer::*;`)
with no `local: *;` stanza, whereas the generator emits per-symbol entries
with `local: *;`. Running it would therefore reformat the released stanzas
instead of updating them. The stanzas are named
`MIRWAYLAND_<major>.<minor>` after the project version (for example
`MIRWAYLAND_2.29`, then `MIRWAYLAND_2.30`), each one inheriting from the
previous stanza.

### Scenario 1: adding a new symbol

1. Make the additive change to the interface (e.g. by adding a new method
   to an existing class in `include/wayland/mir/wayland/`).
1. Open `src/wayland/symbols.map` and, if not already created in this release
   cycle, add a new stanza named after the current development version
   (e.g. `MIRWAYLAND_2.30 { ... } MIRWAYLAND_2.29;`). If the stanza already
   exists, add your symbols to it instead of creating another one.
1. List the new symbols in the new stanza, following the style of the existing
   entries (for example `mir::wayland::WlArrayBase::*;` inside an
   `extern "C++"` block).
1. Check that your new symbols are reflected properly in the `symbols.map` file:
   ```sh
   git diff src/wayland/symbols.map
   ```
1. Build the project and run the relevant tests. Because no automated check
   compares the map against the headers, reviewing the diff carefully is the
   verification: every new public symbol must appear, and no released stanza
   may be modified.

### Scenario 2: removing or changing a symbol

This is an API break. Follow the same manual stanza procedure as above, and
additionally:

1. If not already bumped in this release cycle, bump `MIRWAYLAND_ABI` in
   `src/wayland/CMakeLists.txt`.
1. Run:
   ```sh
   ./tools/update_package_abis.sh
   ```
   (There is no Debian `.symbols` file for `mirwayland`, so no
   `regenerate-*-debian-symbols` step applies.)
