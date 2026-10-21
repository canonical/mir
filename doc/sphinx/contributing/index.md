(contributing-index)=

# Contributing to Mir

Mir is a C++ project with Rust components that is developed, tested, and shipped across
multiple platforms and packaging formats. This documentation helps contributors
understand the codebase, build and validate changes, and participate in the project.

You do not need to be a compositor developer to contribute: documentation improvements,
useful issue reports, and testing releases on real hardware all help improve Mir. New
contributors can start with the
[good first issues](https://github.com/canonical/mir/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22Good%20first%20issue%22)
on GitHub.

## In this documentation

### Get involved and contribute

- {ref}`Get involved in Mir <howto-contribute>` to set up a development
  environment and learn how to contribute.
- {ref}`Contribute documentation <contributing-documentation>`.
- {ref}`Report issues and request features <contributing-reporting-issues>`.

### Develop and maintain Mir

- Learn about Mir's {ref}`architecture <mir-architecture>`,
  {ref}`libraries and module structure <mir-libraries>`, and
  {ref}`input platform APIs <mir-input-platform>`.
- Work with {ref}`graphics support for snaps <how-to-enable-graphics-for-snaps-on-a-device>`,
  {ref}`debug Mir snaps <how-to-debug-mir-snaps>`,
  {ref}`update Wayland Explorer <how-to-update-wayland-explorer>`, and
  {ref}`update symbol maps <how-to-update-symbols-maps>`.

### Test, release, and look up development practices

- {ref}`Test Mir for a release <how-to-test-mir-for-a-release>` or validate a graphical
  snap environment with {ref}`checkbox-mir <how-to-use-checkbox-mir>`.
- {ref}`Measure code coverage <how-to-measure-coverage>` and
  {ref}`release a new Mir version <how-to-release-a-new-mir-version>`.
- Read the {ref}`C++ style guide <cppguide>`,
  {ref}`continuous integration reference <mir-continuous-integration>`, and
  {ref}`DSO versioning guide <dso-versioning-guide>`.

## How this documentation is organised

The {ref}`how-to guides <contributing-how-to-index>` cover tasks such as contributing,
development, testing, and releasing. {ref}`Explanations <contributing-explanation-index>`
describe the project's architecture and APIs, while the {ref}`reference <contributing-reference-index>` documents development practices and tooling.

```{toctree}
---
hidden:
---
how-to/index
explanation/index
reference/index
```
