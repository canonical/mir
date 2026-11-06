# Preface

This file explains how Mir backs Wayland SHM pools. It covers the SIGBUS guard for over-claimed pool sizes and the mapping and resize model in [`ShmBacking`](../shm_backing.cpp).

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Act only in synchronous BUS_ADRERR context

The SIGBUS handler takes an ordinary mutex and relies on kernel-sent, synchronous BUS_ADRERR faults. It assumes no thread touches protected SHM memory while holding a lock that the handler needs (such as `current_access`). This is a reentrancy assumption, not async-signal safety. Never access SHM pixels while holding such a lock, and do not add locks or allocation to the handler. Forward any other SIGBUS to the previous handler untouched.

## §2 - Never shrink

`ShmBacking::resize()` rejects any size smaller than the current mapping. A backing store only ever grows.


# Concepts

## SIGBUS protection

A client can claim a larger SHM pool than it created. [`ShmBacking::lock_range()`](../shm_backing.cpp) wraps such a mapping with an `AccessProtector` unless `size_is_trustworthy` is true. On a BUS_ADRERR fault in the protected range, [`sigbus_handler`](../shm_backing.cpp) attempts to replace the range with an anonymous zero `MAP_FIXED` mapping so the access completes. On success, the protector records the fault and `Mapping::access_fault()` reports it.

Recovery is not guaranteed. `within_protected_region()` rejects a fault on the range's first byte because it uses `>` when `protect_access_to()` documents `[addr, addr+len)`. The `MAP_FIXED` remap fails when the range start (`mapped_address + offset`) is not page-aligned. Unless another live protector recovers it, such a fault falls through to the previous SIGBUS handler. The system installs the handler on every `protect_access_to()`, but saves the previous handler only when the installed handler is not already ours.

## Mapping lifetime

`current_mapping` is an `std::atomic<std::shared_ptr<CurrentMapping const>>`. Each `CurrentMapping` munmaps itself on destruction and pins the address for readers. `get_range()` bounds-checks with `start > size || size - start < len` to avoid integer overflow.

## Size guarantees

[`backing_size_is_guaranteed_at_least()`](../shm_backing.cpp) reports whether the kernel guarantees the file cannot shrink (F_SEAL_SHRINK) and its current size covers the mapping. `resize()` stores that result as `size_is_trustworthy`, which lets later mappings skip the SIGBUS protector entirely.
