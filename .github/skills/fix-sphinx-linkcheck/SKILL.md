---
name: fix-sphinx-linkcheck
description: "Fix Sphinx linkcheck failures in Mir docs: disambiguate ambiguous internal links, replace stale external anchors, add known 403-only URLs to linkcheck_ignore, and verify with doc-clean-doc, doc-html, doc-linkcheck targets."
---

# Fix Sphinx Linkcheck

Use this workflow when CI reports Sphinx linkcheck failures in the Mir docs.

Reference for destination resolution and disambiguation:
https://myst-parser.readthedocs.io/en/latest/syntax/cross-referencing.html#default-destination-resolution

## Steps

1. Identify failing links from `doc-linkcheck` output.
2. For ambiguous internal references (for example multiple matches for one label), disambiguate with Sphinx's standard-domain label role.
   - Preferred: ``{ref}`my-label` ``.
   - Example: use ``{ref}`release-notes` `` instead of doc/path-style links.
3. For missing anchors, replace with a currently working URL/anchor from upstream docs.
4. For stable, known 403 responses from external sites, add a focused regex to `doc/sphinx/conf.py` `linkcheck_ignore`.
5. Verify in sequence:

```bash
cmake -B ~/build .
cmake --build ~/build --target doc-clean-doc
cmake --build ~/build --target doc-html
cmake --build ~/build --target doc-linkcheck
```

## Rules

- Keep edits minimal and local to the reported failures.
- Prefer ``{ref}`label` `` links for internal docs.
- Prefer explicit targets created with `(my-label)=` over implicit heading anchors.
- Keep ignore regexes narrowly scoped to avoid masking real failures.
