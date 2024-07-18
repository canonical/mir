---
discourse: 4911,5164,5603,6756,8037
---

# Tutorial 2
This tutorial will guide you through using Mir demos to see what Mir is capable
of, and the usage of Mir libraries to build your own Wayland compositor.

## Mir Demos
Mir comes with a set of demos in its [example
directory](https://github.com/canonical/mir/tree/main/examples) to showcase its
abilities. This section will cover the installation and usage of these demos.


### Installation
Mir demos are available on Ubuntu as prebuilt binaries. For distros that don't
have prebuilt binaries, examples can be built from source.

<details>
<summary> Installing Mir Demos on Ubuntu </summary>

You can run the following command to install Mir demos and the drivers they
use.
```sh
sudo apt install mir-demos mir-graphics-drivers-desktop
```
</details>

<details>
<summary>Building Mir Demos on Other Distros</summary>

For Linux distributions that don't currently package Mir demos you need to
build them yourself. (See [Getting Involved in
Mir](how-to/getting_involved_in_mir)).
</details>

### Running
The main script you'll want to play around with is `miral-app`. It runs a shell
with some eyecandy by default, but you can run it in kiosk mode if that fits
your usecase more.

For the uninitiated: The shell mode runs a floating window manager just like
GNOME and such where you can move windows and maximize or minimize them. Kiosk
mode on the other hand assumes you want one (or more) applications in
fullscreen mode all the time.

To run in shell mode, just run:
```sh
miral-app
```

for kiosk mode:
```sh
miral-app -kiosk
```

<!--- Likely to get axed, opinions welcome -->
#### Under The Hood
In reality, `miral-app` is just a thin wrapper around `miral-shell` and
`miral-kiosk` that sets things up so they can run properly. You can even run
those two binaries on their own if you set things up. Do note that the
underlying binary might support more options than `miral-app`, so when you pass
an option that `miral-app` doesn't understand, it'll pass that and any
following options onto that binary. 

For example, if you run:
```sh
miral-app --window-manager tiling
```
`miral-app` doesn't support this flag, so it's sent down to `miral-shell` which
interprets it. If you add the `-kiosk` option, you'll get an error since the
kiosk binary doesn't support that flag.

## Mir Libraries
At this point, you're probably taken away by Mir's capabilities and want to use
it and the supporting libraries to build your own Wayland compositor. This
section will guide you through the installation and basic usage of Mir.

### Installation

### Usage
