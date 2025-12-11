# Release Notes

<!--

## Mir X.YY.Z

- New upstream release X.YY.Z

  - ABI summary:
    - miral ABI unchanged at <version>
    - mircommon ABI unchanged at <version>
    - mircore ABI bumped to <version>
    - miroil ABI unchanged at <version>
    - mirplatform ABI bumped to <version>
    - mirserver ABI unchanged at <version>
    - mirwayland ABI bumped to <version>
    - mirplatformgraphics ABI unchanged at <version>
    - mirinputplatform ABI bumped to <version>

  - Enhancements:
    - Pull request title (#<PR number>)

  - Bugs fixed:
    - Issue title (#<Issue number>)

  - Documentation:
    - Pull request title (#<PR number>)

-->

## Mir 2.25.0

- New upstream release 2.25.0

  - ABI summary:
    - miral ABI unchanged at 7
    - mircommon ABI unchanged at 11
    - mircore ABI unchanged at 2
    - miroil ABI unchanged at 8
    - mirplatform ABI bumped to 34
    - mirserver ABI bumped to 66
    - mirwayland ABI unchanged at 5
    - mirplatformgraphics ABI unchanged at 23
    - mirinputplatform ABI unchanged at 10

  - Enhancements:
    - Implementation of an evdev platform in Rust (#4336)
    - cmake: extend/fix RustLibrary.cmake (#4528)
    - Bootstrap new release process (and bump to 2.24.0~dev) (#4519)
    - CI: add "Start Release" workflow
    - Tools: support -rc and -dev tags
    - Modify `XCursorLoader::image` to check the given name first (#4522)
    - Don't give up if specified cursor theme is not found (#4520)
    - Renderer: Ensure current EGL context when destroying GL resources (#4460)

  - Bugs fixed:
    - Fix magnifier filter not consuming events when magnifications is greater/less than the max/min (#4523)
    - Don't use `assert()` (#4517)
    - Destroy toplevel handles on application switcher destruction (#4518)
    - Fix various TICS warnings in platforms (#4460 and related)

  - Documentation:
    - Add Release Notes skeleton
