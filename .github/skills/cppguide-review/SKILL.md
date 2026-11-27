---
name: cppguide-review
description: Review C++ pull request changes for actionable violations of Mir's C++ style guide. Use when reviewing changes to .h or .cpp files, especially GitHub pull requests; optionally propose a cppguide improvement when PR discussion justifies a reusable exception.
---

# Review Mir C++ Guide Compliance

Review only changed C++ code for concrete violations of
[`doc/sphinx/contributing/reference/cppguide.md`](../../../doc/sphinx/contributing/reference/cppguide.md).
This workflow is intended for advisory pull request review, not as a CI gate.

## Review procedure

1. Identify the pull request diff and limit the review to changed `.h` and `.cpp` lines.
   Read enough surrounding code to understand local conventions and the change's context.
1. Read the relevant sections of the C++ guide. The guide is authoritative; do not treat
   personal preferences, generic C++ advice, or rules from other instruction files as guide
   violations unless the guide itself supports the finding.
1. Check each potential issue against the guide and the surrounding code. The guide's
   [exceptions for existing non-conformant code](../../../doc/sphinx/contributing/reference/cppguide.md#existing-non-conformant-code)
   allow local consistency to take precedence when a change is made in legacy code. Do not
   request unrelated cleanup or flag unchanged pre-existing code.
1. Report a violation only when it is specific, actionable, and clearly supported by a guide
   rule. Place the comment on the smallest relevant changed line range. Cite the guide section
   by its heading and link when possible. Explain the mismatch and give a concise correction.
1. Do not review general correctness, security, performance, ABI, or test coverage as part of
   this skill unless a C++ guide rule directly applies.
1. If there are no actionable violations, say so briefly and do not invent review comments to
   fill space.

## Pull request discussion and guide proposals

When the PR description or discussion available to you explains why a change intentionally
deviates from the guide, consider whether the explanation reveals a reusable gap in the guide.
Only propose a guide addition when the rationale is explicit and the proposed rule could help
future contributors beyond this one change. Do not infer missing discussion or rationale.

Present any proposal separately from violations as a non-blocking **C++ guide suggestion**.
Include the relevant existing guide section, concise draft wording, and why the rule generalizes.
Do not edit the guide, treat the justified deviation as a violation, or suggest encoding a
one-off exception as a general rule.

## Finding format

Each review comment should:

- identify one issue;
- explain why the changed code conflicts with a named C++ guide section;
- state the likely consequence only when the guide or code context supports it; and
- offer a focused correction.

Keep comments neutral and concise. Do not report speculative concerns or stylistic preferences.
