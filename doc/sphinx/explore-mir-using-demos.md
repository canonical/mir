---
discourse: 4911,5164,5603,6756,8037
---

# Explore What Mir Is Capable of Using Mir Demos
Mir comes with a set of demos in its [example
directory](https://github.com/canonical/mir/tree/main/examples) to showcase its
abilities. This section will cover the installation and usage of these demos.


## Installation
Mir demos are available on Debian derivatives, Fedora, and Alpine. For distros
that don't have prebuilt binaries, examples can be built from source.

<details>
<summary> Installing Mir Demos on Debian and its derivatives </summary>

```sh
sudo apt install mir-demos mir-graphics-drivers-desktop
```
</details>

<details>
<summary> Installing Mir Demos on Fedora </summary>

```sh
sudo dnf install mir-demos mir
```
</details>

<details>
<summary> Installing Mir Demos on Alpine </summary>

```sh
sudo apk add mir-demos mir
```
</details>


## Running
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
### Under The Hood
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

## Running Natively
The previous section showed how you can run Mir demos under an X11 or Wayland
session. But Mir compositors can also run "natively" by launching them from a
virtual terminal or a greeter.

### Launching from a virtual terminal
To switch to a virtual terminal, you can press CTRL+ALT+F<Number>. You can then
log in and run:
```sh 
miral-shell
```

### Launching from a greeter
You can also launch `miral-shell` from your greeter by opening the window
manager list (bottom right cog on Ubuntu) and choosing "Miral Shell". Once you
log in, you'll see `miral-shell` running fullscreen!

## Using On-screen Keyboards
Mir based compositors can easily support on-screen keyboards. Note that due to
security reasons, the Wayland extensions needed are disabled by default.

For the purpose of demonstation, we'll use `ubuntu-frame-osk`, but you're free
to use any Wayland compatible on-screen keyboard.

You can install `ubuntu-frame-osk` by running:
```sh
sudo snap install ubuntu-frame-osk
```

If you get errors later on related to connecting to a Wayland compositor, you should run:
```sh
sudo snap connect ubuntu-frame-osk:wayland
```

To test your favourite on-screen keyboard, you can start `miral-shell` as follows:
```sh
miral-shell --add-wayland-extensions all
```

Once `miral-shell` loads, you can start the terminal, then run:
```sh
ubuntu-frame-osk
```
And an on-screen keyboard should pop up!


## Mixing Wayland and X11 Clients
You can easily run X11 applications inside Mir based compositors. For example,
to enable x11 support in `miral-shell`, you can run the following command:

```sh
miral-shell --enable-x11 true
```

When it loads, you can start a terminal and run `xclock` or any other X11
application side to side with Wayland applications!

## Remote Desktop
Mir also supports remote desktops via the VNC protocol. To demo this, we'll use
`wayvnc`. You can install it by running:

```sh
sudo apt install wayvnc
```

Then, you can run `miral-shell` with all extensions like so:
```sh
miral-shell --add-wayland-extensions all
```

From inside, you can run the terminal and run `wayvnc`:
```sh
wayvnc
```

This should start `wayvnc` and make it listen to `localhost`. To connect, run
your favourite VNC viewer and connect to `localhost`. You should see the exact
same view in both Mir compositor and the VNC viewer.

## Nesting Wayland Compositors
