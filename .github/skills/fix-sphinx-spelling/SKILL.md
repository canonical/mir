---
name: fix-sphinx-spelling
description: 'Fix Sphinx spelling failures in Mir docs: correct true misspellings in docs content, add project-specific false positives to .custom_wordlist.txt, and verify with doc-spelling.'
---

# Fix Sphinx Spelling

Use this workflow when CI reports Sphinx spelling failures in the Mir docs.

## Steps

1. Run `doc-spelling` and collect the reported words and file/line locations.
1. Classify each reported word:
   - Real typo in prose: fix the text in-place.
   - Intentional technical/project term, acronym, symbol, or command: add it to `doc/sphinx/.custom_wordlist.txt`.
1. Keep dictionary entries minimal:
   - Add only exact words that are true false positives.
   - Preserve alphabetical ordering of `.custom_wordlist.txt`.
1. Re-run spelling checks until clean:

```bash
cmake -B ~/build .
cmake --build ~/build --target doc-spelling
```

## Rules

- Prefer fixing real typos over adding dictionary entries.
- Keep edits minimal and local to reported failures.
- Do not add broad patterns or unrelated words to `.custom_wordlist.txt`.
- Maintain alphabetical order in `.custom_wordlist.txt` when adding new entries.
- If a term can be written in standard form without losing meaning (for example `pkg-config`), prefer that.
