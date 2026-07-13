---
applyTo: '**/symbols.map,src/**/CMakeLists.txt,CMakeLists.txt,debian/*.symbols,debian/control,tools/update_package_abis.sh'
description: ABI stability and symbol-map management for Mir libraries.
---

# ABI stability & symbol management

MirAL provides a stable ABI. Any changes to the public API must be reflected in symbol maps
and Debian symbol files.

- Each library has a `symbols.map` (e.g., `src/miral/symbols.map`) with versioned stanzas (e.g., `MIRAL_5.0 { ... }`).
- Corresponding Debian symbol files (e.g., `debian/libmiral7.symbols`) must be updated in lockstep.
- MirAL headers (`include/miral/miral/`) must NOT expose implementation details: no templates with logic in headers, no inline functions that depend on internal types. Use the pImpl pattern.
- Do not put "deducing this" or other constructs that require definitions in headers — MirAL headers must remain ABI-stable.

## When and how to bump ABIs

### miral / miroil — adding a new symbol (non-breaking)

1. Add the new method/class to the public header and implementation
1. If not already bumped this release cycle, bump `MIRAL_VERSION_MINOR` in `src/CMakeLists.txt` (or `MIROIL_VERSION_MINOR` in `src/miroil/CMakeLists.txt`)
1. `cmake --build <BUILD_DIR> --target generate-miral-symbols-map` (or `generate-miroil-symbols-map`)
1. Verify with `git diff src/miral/symbols.map`
1. `cmake --build <BUILD_DIR> --target regenerate-miral-debian-symbols` (or `regenerate-miroil-debian-symbols`)

No `*_ABI` bump is required for additive changes.

### miral / miroil — removing or changing a symbol (ABI break)

1. Make the destructive change
1. Bump `MIRAL_VERSION_MAJOR` in `src/CMakeLists.txt`, reset `MIRAL_VERSION_MINOR` and `MIRAL_VERSION_PATCH` to 0
1. Bump `MIRAL_ABI` in `src/miral/CMakeLists.txt`
1. Run `generate-miral-symbols-map` then `regenerate-miral-debian-symbols`
1. Run `./tools/update_package_abis.sh` to update Debian package names (control file, `.install` file renames) and RPM ABI macros

### mirserver — any symbol change (additive or breaking)

mirserver has no stable ABI promise. **Every** symbol change requires bumping `MIRSERVER_ABI`:

1. Make the change
1. Bump `MIRSERVER_ABI` in `src/server/CMakeLists.txt`
1. Bump the minor number in `project(Mir VERSION X.Y.Z)` in the **top-level** `CMakeLists.txt` (the stanza name `MIR_SERVER_INTERNAL_X.Y` derives from this)
1. `cmake --build <BUILD_DIR> --target generate-mirserver-symbols-map`
1. Run `./tools/update_package_abis.sh` (handles `debian/control`, `.install` file renames, and RPM ABI macros)

**Note**: there is no `regenerate-mirserver-debian-symbols` target — mirserver debian packaging is managed by `update_package_abis.sh`.

### General rules

- All version bumps are once-per-release-cycle — check if someone already bumped before you do.
- CI (`symbols-check.yml`) catches added/removed symbols but does **not** detect changed signatures — those must be moved to a new stanza manually.
- `tools/update_package_abis.sh` handles `debian/control`, `.install` file renames, and RPM ABI macros for all libraries. It does not modify `symbols.map` or CMake variables.
- ABI version variables are defined per-library as `set(LIB_ABI N)` (e.g., `MIRAL_ABI`, `MIRCORE_ABI`, `MIRWAYLAND_ABI`, `MIRSERVER_ABI`). Bump these when breaking ABI.

See [How to Update Symbol Maps](../../doc/sphinx/contributing/how-to/update-symbols-map.md) and
[DSO Versioning Guide](../../doc/sphinx/contributing/reference/dso-versioning-guide.md) for full
details.
