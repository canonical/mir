# Contributing to Mir

This section provides information for contributors to the Mir project.

Mir is a large C++ project that ships across many platforms and packaging formats,
including snaps. The documents in this subsection are intended to assist developers
who are contributing to Mir. These docs will guide you through the code, help test your
changes, and get your proposals merged, in addition to documenting the project processes.

The Mir development team welcomes new contributors to the project.
If you are interested in making an impact on the Linux desktop, check out the
[good first issues](https://github.com/canonical/mir/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22Good%20first%20issue%22)
label in the Mir project where you can find issues set aside specifically
for newcomers to the project.

You do not need to be a compositor developer to make meaningful contributions.
Improving docs, reporting high-quality issues, and running test plans on real
hardware all help us ship better releases.

## In this documentation

These pages cover the key aspects of working on Mir

````{grid} 1 1 2 2

```{grid-item-card} [How-to guides](contributing-how-to-index)
**How-to guides** - step-by-step guides covering key operations and common tasks
- [](howto-contribute)
```

```{grid-item-card} [Explanation](contributing-explanation-index)
 **Discussion and clarification** of key topics
```

````

````{grid} 1 1 2 2

```{grid-item-card} [Reference](contributing-reference-index)
 **Technical information** - specifications, APIs, architecture
```
````

## Project and community

Mir is a member of the Ubuntu family. It’s an open source project that warmly welcomes community projects, contributions, suggestions, fixes and constructive feedback.

- [Release Notes](https://github.com/canonical/mir/releases)
- [Code of conduct](https://ubuntu.com/community/docs/ethos/code-of-conduct)
- [Get support](https://discourse.ubuntu.com/c/project/mir/15)
- [Join our online chat](https://matrix.to/#/#mir-server:matrix.org)
- {ref}`Contribute <howto-contribute>`

## Contributing beyond core development

### Documentation

Mir documentation lives under `doc/sphinx/` and is built with Sphinx.
If you spot unclear wording, outdated examples, or missing guides, open a pull
request with a focused docs change.

- Start with {ref}`Getting involved in Mir <howto-contribute>`
- [How to contribute documentation to Mir](how-to/how-to-contribute-documentation)
- Build docs locally with the `doc-html` target before opening a PR

### Raising issues and requesting features

Useful issue reports save maintainers and reviewers a lot of time.
When filing an issue, include what you expected, what happened, how to
reproduce it, and relevant logs or hardware details.

- [Open a Mir issue](https://github.com/canonical/mir/issues)
- [How to report Mir issues and request features](how-to/how-to-report-issues)
- [Security reporting policy](https://github.com/canonical/mir/blob/main/SECURITY.md)
- [Discuss ideas first on Discourse](https://discourse.ubuntu.com/c/project/mir/15)

### Testing outside day-to-day development

You can contribute by validating release candidates and snap environments even
if you are not changing Mir source code.

- {ref}`How to test Mir for a release <how-to-test-mir-for-a-release>`
- {ref}`Using checkbox-mir <how-to-use-checkbox-mir>`

Thinking about using Mir for your next project? [Get in touch](https://canonical.com/mir)!

```{toctree}
---
hidden:
---
how-to/index
explanation/index
reference/index
```
