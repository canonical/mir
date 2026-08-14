---
myst:
  html_meta:
    description: How to contribute documentation to Mir, including structure, style, local builds, and pull request checks.
---

(contributing-documentation)=

# How to contribute documentation to Mir

Documentation improvements are a first-class contribution to Mir.
If you find unclear wording, stale instructions, or missing guidance, please open a docs pull request.

## Understand where content belongs

Mir documentation in `doc/sphinx/` follows the [Diátaxis](https://diataxis.fr/) framework:

- `tutorial/` for learning-oriented guides
- `how-to/` for step-by-step task guides
- `explanation/` for concepts and background
- `reference/` for technical detail and specifications
- `configuring/` for end-user configuration
- `contributing/` for contributor workflows

When adding a page, choose the section by user need rather than by topic name.

## Make a focused docs change

1. Keep each pull request focused on one docs outcome (for example: fix one stale guide, or add one missing guide).
1. Prefer concrete examples and copyable commands.
1. Keep language concise and direct.
1. If you move or rename a page, add a redirect in `doc/sphinx/redirects.txt`.

## Build and validate docs locally

From the repository root, use the CMake doc targets:

```sh
cmake -B build
cmake --build build --target doc-html
cmake --build build --target doc-linkcheck
cmake --build build --target doc-spelling
```

If you are iterating on wording and layout, `doc-serve` is useful for live preview.

## Submit a documentation pull request

In your PR description:

- explain who the page/change helps,
- describe what changed,
- list what checks you ran (for example `doc-html` and `doc-linkcheck`).

If your change affects contributor workflows, add links from index pages such as `doc/sphinx/contributing/index.md` or `doc/sphinx/contributing/how-to/index.md`.

## Related resources

- [Getting involved in Mir](getting-involved-in-mir)
- [Mir Hacking Guide](https://github.com/canonical/mir/blob/main/HACKING.md)
- [Mir issue tracker](https://github.com/canonical/mir/issues)
