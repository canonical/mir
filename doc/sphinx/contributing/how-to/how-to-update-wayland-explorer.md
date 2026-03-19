# How to update Wayland Explorer for Mir

Wayland Explorer is a popular website that provides a user-friendly
view of the protocols that each compositor or compositor library
supports. You can find it here: https://wayland.app/protocols/.

## Updating for Mir

1. First, clone and install `wlprobe`, which is required to get a JSON
   file of the available protocols:

   ```sh
   git clone git@github.com:PolyMeilex/wlprobe.git
   cargo install --path wlprobe
   ```

1. Next, clone Wayland Explorer:

   ```sh
   git clone git@github.com:vially/wayland-explorer.git
   ```

1. Next, run `miral-app` with all available Wayland extensions enbled:

   ```sh
   WAYLAND_DISPLAY=wayland-98 miral-app --add-wayland-extension=all
   ```

1. Then, run `wlprobe` from the `wayland-explorer` directory:

   ```sh
   cd wayland-explorer
   MIR_VERSION=2.26  # Set your Mir version to the current version
   WAYLAND_DISPLAY=wayland-98 ~/.cargo/bin/wlprobe --unique | jq --arg version "$MIR_VERSION" '{generationTimestamp, version:$version, globals}' > src/data/compositors/mir.json
   ```

1. Finally, check the changes and open up a pull request

   ```sh
   git diff src/data/compositors/mir.json
   ```
