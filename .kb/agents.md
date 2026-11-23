# Preface

This repository follows strict conventions for organizing agent-oriented knowledge documents, and this document is required reading for any agent that wants to read or write these.

The design of this structure has the following key goals:

- **Mechanical** - Encourages agents to read/write the knowledge base as main actors.
- **Generic** - Benefits any agentic workflow, no matter the editor or platform.
- **Distilled** - Avoids the use of verbose task or plan logs that pollute the context window.
- **Hierarchical** - Takes information closer to its context, avoids bloat in a single place.
- **Human** - Preserves information readily available and reviewable in a friendly format.

IMPORTANT: Read the `.kb/agents.md` file.


# Laws

## §1 - Organization

Every directory in this repository may have its own `.kb/` subdirectory with `.kb/*.md` files with dashed lowercase names such as `.kb/special-relativity.md`.

Every filename must be _globally unique_ inside the repository, so section references such as agents.md§1 are possible.

## §2 - Headers

The following are the ONLY top-level headers allowed in `.kb/*.md` files in this repository. Keep the section order identical to that presented below.

- _Preface_ - A brief introduction outlining the scope and relevance of a specific `.kb/*.md` file. This section MUST be at the top of every `.kb/*.md` file so agents can easily grep for it, and the last line of this section MUST be "IMPORTANT: Read the `.kb/agents.md` file." so rules are respected.
- _Laws_ - Essential directives outlining critical constraints, behaviors, and rules. The Laws are defined in _terse_ subsections `## §<N> - <Title>` as in this file, internally referenced as §2 and externally as agents.md§2 (this law).
- _Concepts_ - Structural design, details, references, etc. This is the only section that accepts open-ended subsections.

Keep two empty lines before top headers, and one empty line before subheaders.

## §3 - Process

- Read the _Knowledge Base_ section inside the `AGENTS.md` file at the top of the repository.
- Keep the `.kb/*.md` files up-to-date: add or update content whenever something relevant needs documenting. The `.kb/agents.md` must NOT be edited unless explicitly requested.
- Follow the header conventions in §2. Only the _Preface_ header is required, and the other headers should be omitted if empty or trivial.
- If you find incorrect organization that breaks any of the laws in this document, fix it.
- Never change the subject of a Law - if obsolete, replace its header with `## §<N> - Revoked`.

## §4 - AGENTS.md

The repository's `AGENTS.md` file MUST START with an ordered list of ALL `.kb/*.md` files, in this format:
```
# Knowledge Base

IMPORTANT: Read the `.kb/agents.md` file.

- [.kb/agents.md](.kb/agents.md) - Structure and rules of the knowledge base. You MUST read this file.
- [.kb/filename.md](.kb/filename.md) - Terse summary.
- [subdir/.kb/filename.md](subdir/.kb/filename.md) - Another terse summary.
```


# Concepts

## Changelog

- **2026-11-03** - Added revocations. Moved Headers before Process. Changed sentence pointing to .kb/agents.md, enforced it for AGENTS.md.
- **2026-10-30** - Major overhaul. Added: Laws, Concepts; Removed: Overview, Important, Architecture, Directory, Documents, AGENTS.md in subdirectories.
- **2026-09-24** - Enforced ordering of all headers. Minor tweaks for clarity.
- **2026-06-08** - Public release.

## Version updates

Only when EXPLICITLY requested to update the version of the knowledge base, replace this file with:

https://raw.githubusercontent.com/canonical/knowledge-base/refs/heads/main/.kb/agents.md
