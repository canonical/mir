# How to update Wayland Explorer for Mir

Wayland Explorer is a popular website that provides a user-friendly
view of the protocols that each compositor or compositor library
supports. You can find it here: https://wayland.app/protocols/.

## Updating for Mir

1. Install `wlprobe`, which is required to get a JSON file of the
   available protocols:

   ```sh
   cargo install wlprobe
   ```

1. Run `miral-app` with all available Wayland extensions enabled:

   ```sh
   WAYLAND_DISPLAY=wayland-98 miral-app --add-wayland-extension=all
   ```

1. Run `wlprobe` to generate the updated compositor JSON:

   ```sh
   WAYLAND_DISPLAY=wayland-98 ~/.cargo/bin/wlprobe --unique > mir.json
   ```

1. Add a `"version"` field set to the current Mir version to `mir.json`,
   e.g.:

   ```json
   {
     "version": "2.26",
     "generationTimestamp": "...",
     "globals": ["..."]
   }
   ```

1. Finally, go to
   https://github.com/vially/wayland-explorer/edit/main/src/data/compositors/mir.json,
   replace the file content with your updated `mir.json`, and open a
   pull request.
