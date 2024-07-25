# Explore What Mir Is Capable of Using Mir Demos
Mir comes with a set of demos in the [example
directory](https://github.com/canonical/mir/tree/main/examples) to showcase its
abilities. This section will cover the installation and usage of these demos.


## Installation
Mir demos are available on Debian derivatives, Fedora, and Alpine. For distros
that don't have prebuilt binaries, examples can be built from source.

To install Mir demos on Debian and its derivatives:

```sh
sudo apt install mir-demos mir-graphics-drivers-desktop
```

<details>
<summary> Installing Demos on Other Distros </summary>

Installing Mir demos on Fedora 
```sh
sudo dnf install mir-demos
```

 Installing Mir demos on Alpine 
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

**Note**: By default, Mir does not enable Wayland extensions that normal
applications should not be using. For demonstration purposes we will override
this and allow all supported extensions in some of the following examples by
passing `--add-wayland-extensions all` when running the example.

## Running Natively
The previous section showed how you can run Mir demos under an X11 or Wayland
session. But Mir compositors can also run "natively" by launching them from a
virtual terminal or a greeter.

### Launching from a virtual terminal
To switch to a virtual terminal, you can press CTRL+ALT+F\<Number\>. You can then
log in and run:
```sh 
miral-app
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
sudo snap connect ubuntu-frame-osk:wayland
```

To test your favourite on-screen keyboard, you can start `miral-app` as
follows:
```sh
miral-app --add-wayland-extensions all
```

Once the shell loads, you can start the terminal, then run:
```sh
ubuntu-frame-osk&
```
And an on-screen keyboard should pop up!


## Mixing Wayland and X11 Clients
You can easily run X11 applications inside Mir based compositors. For example,
to enable x11 support in `miral-shell`, you can run the following command:

```sh
miral-app --enable-x11 true
```

When it loads, you can start a terminal and run `xclock` or any other X11
application alongside Wayland applications!

## Remote Desktop
Mir also supports remote desktops via the VNC protocol. To demo this, we'll use
`wayvnc`. You can install it by running:
```sh
sudo apt install wayvnc
```

Start the shell with all extensions enabled:
```sh
miral-app --add-wayland-extensions all
```

From inside, you can run the terminal and run `wayvnc`:
```sh
wayvnc
```

This should start `wayvnc` and make it listen to `localhost`. To connect, run
your favourite VNC viewer and connect to `localhost`. You should see the exact
same view in both Mir compositor and the VNC viewer. For example:
```sh
gvncviewer localhost
```
