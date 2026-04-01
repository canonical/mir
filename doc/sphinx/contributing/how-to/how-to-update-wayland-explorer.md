---
html:
  meta:
    description: How to update the Wayland Explorer's view of protocols supported by a Mir server.
---

# How to update Wayland Explorer for Mir

Wayland Explorer is a popular website that provides a user-friendly view of the protocols that each compositor or compositor library supports.
You can find it here: https://wayland.app/protocols/.

## Updating for Mir

1. Install `wlprobe`, which is required to get a JSON file of the available protocols:

   ```sh
   cargo install wlprobe
   ```

1. Run `mir_demo_server` with `wlprobe` as its client:

   ```sh
   env -u WAYLAND_DISPLAY mir_demo_server --test-client wlprobe --timeout 0
   ```

1. Finally, go to https://github.com/vially/wayland-explorer/edit/main/src/data/compositors/mir.json,
   replace the file content with the printed JSON, adding a `"version": "<MAJOR.MINOR>"` value, e.g.:

   ```json
   {
     "version": "2.26",
     "generationTimestamp": "...",
     "globals": ["..."]
   }
   ```

   And open a pull request.
