# Mir documentation

Mir is an open-source C++ library for building custom Wayland compositors. It provides
reusable components for graphics, input, window management, and application
communication, so developers can start with a working compositor and customize the
parts that are specific to their product instead of building everything from scratch.

Mir is used in graphical environments ranging from embedded and kiosk displays to
desktop compositors. Some highlights include Ubuntu Frame, Lomiri, Miracle-WM, and Miriway.

## In this documentation

### Get started and build a compositor

- {ref}`Getting started with Mir <tutorial-getting-started>` introduces Mir's
  capabilities; {ref}`write your first Wayland compositor <write-your-first-wayland-compositor>`
  walks through a minimal compositor.
- Add behavior with guides to {ref}`keyboard input <how-to-handle-keyboard-input>`,
  {ref}`startup applications <how-to-specify-startup-apps>`,
  {ref}`custom Wayland protocols <how-to-integrate-a-custom-wayland-protocol>`,
  {ref}`configuration overrides <how-to-add-configuration-overrides>`, and
  {ref}`client/server-side decoration preferences <specifying-csd-ssd-preference>`.
- Explore {ref}`compositor libraries <consumer-libraries>`,
  {ref}`desktop environments <exp-desktop-environment>`,
  {ref}`window positioning <exp-window-positions-under-wayland>`,
  {ref}`compositors built with Mir <compositors-built-with-mir>`, and the
  {ref}`MirAL API introduction <ref-introducing-the-miral-api>`.
- Browse the full {doc}`API reference <api/library_root>` and
  {ref}`Linux kernel requirements <kernel-requirements>`.

### Run and configure a Mir compositor

- Configure devices and services: {ref}`calibrate a touchscreen <how-to-calibrate-a-touchscreen-device>`,
  {ref}`enable remote desktop <how-to-enable-remote-desktop>`,
  {ref}`enable screencasting <how-to-enable-screencasting>`, and
  {ref}`use on-screen keyboards <how-to-use-on-screen-keyboards>`.
- Look up {ref}`Mir configuration options <options-reference>`,
  {ref}`component reports <component-reports>`, and
  {ref}`configuration overrides <configuration-overrides>`.

### Understand Mir and its platforms

- Understand {ref}`platform support <exp-mir-graphics-support>`,
  {ref}`graphics driver quirks <mir-driver-quirks>`,
  {ref}`security <mir-security>`, {ref}`performance <mir-performance>`,
  {ref}`energy efficiency <mir-energy-efficiency>`,
  {ref}`accessibility <mir-accessibility-methods>`, and
  {ref}`XDG portal support <exp-xdg-portals-support>`.

### Develop and contribute to Mir

- Learn about Mir's {ref}`architecture <mir-architecture>`,
  {ref}`libraries and modules <mir-libraries>`, and
  {ref}`input platform APIs <mir-input-platform>`.
- Build, debug, and validate: {ref}`enable graphics for snaps <how-to-enable-graphics-for-snaps-on-a-device>`,
  {ref}`debug Mir snaps <how-to-debug-mir-snaps>`,
  {ref}`measure coverage <how-to-measure-coverage>`,
  {ref}`test a release <how-to-test-mir-for-a-release>`, and
  {ref}`validate a snap graphical environment <how-to-use-checkbox-mir>`.
- Project maintenance: {ref}`contribute documentation <contributing-documentation>`,
  {ref}`report issues and request features <contributing-reporting-issues>`,
  {ref}`update Wayland Explorer <how-to-update-wayland-explorer>`,
  {ref}`update symbol maps <how-to-update-symbols-maps>`, and
  {ref}`release a new Mir version <how-to-release-a-new-mir-version>`.
- Developer references: {ref}`continuous integration <mir-continuous-integration>`,
  the {ref}`C++ style guide <cppguide>`, and the
  {ref}`DSO versioning guide <dso-versioning-guide>`.

## How this documentation is organised

The documentation follows the [Diátaxis framework](https://diataxis.fr/). The
{ref}`tutorials <tutorial-index>` guide you through learning Mir;
{ref}`how-to guides <how-to-index>` help you complete specific compositor-development
tasks; {ref}`reference <reference-index>` provides API and system details; and
{ref}`explanations <explanation-index>` discuss concepts and design. Separate
{ref}`configuration documentation <configuring-index>` is aimed at people running
Mir-based compositors, while the
{ref}`contributor documentation <contributing-index>` covers developing and maintaining
Mir itself.

## Project and community

Mir is an open-source project in the Ubuntu family, supported commercially by Canonical.
It welcomes community projects, contributions, suggestions, fixes, and constructive feedback.

### Get involved

- {ref}`Contribute to Mir <howto-contribute>`
- [Get support](https://discourse.ubuntu.com/c/project/mir/15)
- [Join the Matrix chat](https://matrix.to/#/#mir-server:matrix.org)
- [Code of conduct](https://ubuntu.com/community/docs/ethos/code-of-conduct)

### Releases and contact

- {doc}`release-notes/index`
- {doc}`roadmap`
- [Mir project website and contact](https://canonical.com/mir)

```{toctree}
---
hidden:
---
tutorial/index
how-to/index
explanation/index
reference/index
configuring/index
contributing/index
release-notes/index
roadmap
```
