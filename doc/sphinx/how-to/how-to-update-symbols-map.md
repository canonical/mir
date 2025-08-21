# How to Update Symbols Map Files
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
- mirwayland

It is tedious to edit these files. Luckily for us, this versioning business
is (mostly) automated so that we have to think very little about it.

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

2. **A symbol is changed or removed** (e.g. a new parameter is
   added to a method call, a class is removed, etc.)

    This is an API break. This means that a consumer of the library
    can no longer safely compile against the new version of that library
    without maybe breaking their build. In this scenario, we are forced
    to create a single new stanza at a brand new version in the `symbols.map`
    file. This new stanza will contain all of the symbols in that library.

If you forget to update the `symbols.map` file, Github CI will complain and
prevent the merge until the issue is addressed. You can find results of this action
[here](https://github.com/canonical/mir/actions/workflows/symbols-check.yml).
Each time a pull request is open or updated on Github, this action will be
run. If you check the log of this action, you will see which symbols have been
changed so that you can address the issue.

**Warning**: Please be aware that this action is not sophisticated enough to
know if you changed the definition of a symbol. If you add or remove a symbol,
the action will inform you. However, if you modify a symbol (e.g. by changing
the parameters that a method takes), you will *not* be informed that the symbol
has changes. Such changes will need to be moved to a new stanza manually.

## Setup
Before we can update the symbols of Mir's libraries, we'll want to set
up our environment so that the our tools can work correctly.

To do this, you will first want to install **clang-19** or newer. Note that it
is very important that you have the most recent version of clang, as that works
the best.

```sh
sudo apt install clang-19
```

Afterwards, you will want to set the following environment variables to point
at the path of `libclang.so` and `libclang.so`'s `lib` directory. Note that
these paths are likely to be the same on most Ubuntu 24.04 setups, but may
vary on other distros:

```sh
export MIR_SYMBOLS_MAP_GENERATOR_CLANG_SO_PATH=/usr/lib/llvm-19/lib/libclang.so.1
export MIR_SYMBOLS_MAP_GENERATOR_CLANG_LIBRARY_PATH=/usr/lib/llvm-19/lib
```

It is recommended that you put those `export ...` commands in your `.bashrc`
so that you don't have to think about it in the future.

One final set up step, you need to install the
corresponding python clang library:
```
sudo apt install python3-clang-19
```

You'll have to run `. tools/symbols_map_generator/venv/bin/activate` before
invoking the symbols map generator in a new terminal session.

And that's it! Now we are ready to update our symbols automatically.

## How to update miral symbols

### Scenario 1: Adding a new symbol
1. Make the additive change to the interface (e.g. by adding a new method
   to an existing class)
2. If not already bumped in this release cycle, bump `MIRAL_VERSION_MINOR`
   in `src/CMakeLists.txt`
3. From the root of the project, run:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target generate-miral-symbols-map
   ```
4. Check that your new symbols is in the `symbols.map` file:
   ```sh
   git diff src/miral/symbols.map
   ```
5. Regenerate Debian symbols:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target regenerate-miral-debian-symbols
   ```

### Scenario 2: Removing or changing a symbol
1. Make a destructive change to the interface (e.g. remove a parameter from
   a method).
2. If not already bumped in this release cycle, bump `MIRAL_VERSION_MAJOR`
   in `src/CMakeLists.txt`. Set `MIRAL_VERSION_MINOR` and `MIRAL_VERSION_PATCH`
   to `0`
3. If not already bumped in this release cycle, bump `MIRAL_ABI` in
   `src/miral/CMakeLists.txt`
4. From the root of the project, run:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target generate-miral-symbols-map
   ```
5. Check that your symbols are reflected properly in the `symbols.map` file:
   ```sh
   git diff src/miral/symbols.map
   ```
6. Regenerate the debian symbols:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target regenerate-miral-debian-symbols
   ```

If `MIRAL_ABI` needed to be updated, you should also run:

```sh
./tools/update_package_abis.sh
```

## How to update miroil symbols

### Scenario 1: Adding a new symbol
1. Make the additive change to the interface (e.g. by adding a new method
   to an existing class)
2. If not already bumped in this release cycle, bump `MIROIL_VERSION_MINOR`
   in `src/miroil/CMakeLists.txt`
3. From the root of the project, run:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target generate-miroil-symbols-map
   ```
4. Check that your new symbols is in the `symbols.map` file:
   ```sh
   git diff src/miroil/symbols.map
   ```

### Scenario 2: Removing or changing a symbol
1. Make a destructive change to the interface (e.g. remove a parameter from
   a method).
2. If not already bumped in this release cycle, bump `MIROIL_ABI`
   in `src/miroil/CMakeLists.txt`. Set `MIROIL_VERSION_MINOR` and `MIROIL_VERSION_PATCH`
   to `0`
3. From the root of the project, run:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target generate-miroil-symbols-map
   ```
4. Check that your symbols are reflected properly in the `symbols.map` file:
   ```sh
   git diff src/miroil/symbols.map
   ```
5. Regenerate the debian symbols:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target regenerate-miroil-debian-symbols
   ```

If `MIROIL_ABI` needed to be updated, you should also run:

```sh
./tools/update_package_abis.sh
```

## How to update mirserver symbols map
### Scenario 1: Adding a new symbol
1. Make the additive change to the interface (e.g. by adding a new method
   to an existing class)
2. If not already bumped in this release cycle, bump `MIR_VERSION_MINOR`
   in `CMakeLists.txt`
3. If not already bumped in this release cycle,  bump `MIRSERVER_ABI` in
   `src/server/CMakeLists.txt`
4. From the root of the project, run:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target generate-mirserver-symbols-map
   ```
5. Check that your new symbols is in the `symbols.map` file:
   ```sh
   git diff src/server/symbols.map
   ```

If `MIRSERVER_ABI` needed to be updated, you should also run:

```sh
./tools/update_package_abis.sh
```

### Scenario 2: Removing or changing a symbol
1. Make a destructive change to the interface (e.g. remove a parameter from
   a method).
2. If not already bumped in this release cycle, bump `MIR_VERSION_MAJOR`
   in `CMakeLists.txt`. Set `MIR_VERSION_MINOR` and `MIR_VERSION_PATCH`
   to `0`
3. If not already bumped in this release cycle,  bump `MIRSERVER_ABI` in
   `src/server/CMakeLists.txt`
4. From the root of the project, run:
   ```sh
   cmake --build <BUILD_DIRECTORY> --target generate-mirserver-symbols-map
   ```
5. Check that your symbols are reflected properly in the `symbols.map` file:
   ```sh
   git diff src/server/symbols.map
   ```

If `MIRSERVER_ABI` needed to be updated, you should also run:

```sh
./tools/update_package_abis.sh
```