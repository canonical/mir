---
name: code-review
description: Review a Mir pull request or local diff for defects that general-purpose review misses, including ABI bookkeeping, Wayland protocol semantics, resource lifetime, and test validity.
---

# Code Review

Use this skill when asked to review a Mir pull request, or a staged, unstaged, or
branch diff.

## Establish the change

1. Read the applicable `.github/instructions/` files, the surrounding code,
   callers, and tests. Current code takes precedence over any earlier review
   comment.
1. Check that the title and description match the diff and test steps.

## Check these first

General-purpose review tends to miss the following. Check each one that applies
before reporting anything else.

- **ABI.** New or removed symbols need the correct `symbols.map` stanza and
  `debian/*.symbols` entry. A removal still needs a version bump. Check the
  current minor-release branch before adding symbols, and prefer a conversion
  operator over an ABI break. Follow
  `.github/instructions/abi-symbols.instructions.md`.
- **Wayland.** Report errors on the resource that defines the rule, not the
  resource of the method being executed. Trace resource lifetime when a peer
  resource can be destroyed first. Check version negotiation against older
  clients. Restrict a rule rather than dropping it. Make multi-step state changes
  atomic. Follow `.github/instructions/wayland-protocol.instructions.md`.
- **Tests.** Assert the property the test name claims, not a weaker one. Confirm
  the test fails without the fix. Avoid timeouts that are too short for CI.
- **Lifetime.** Check for use-after-free when a peer resource dies first, leaks on
  teardown, a null pointer where other paths check for it, and pointers to stack
  storage.
- **Concurrency.** Check that a lock covers the whole multi-step update, and that
  a key collision is not silently ignored.
- **Errors.** Check that an exception is not constructed and discarded, and that
  `noexcept` and hot paths do not allocate.
- **Build.** Check for duplicate link entries, unused includes after a rewrite,
  and toolchain assumptions in workflows.

## Report

- Verify each finding against the current code before reporting it.
- Give a file and line, the triggering scenario, and the observable consequence.
- Order findings by severity, and separate blocking defects from suggestions.
- If you find nothing, say so and name what you did not check.
