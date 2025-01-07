# Getting started with Mir

This tutorial will guide you through Mir's basic functionality. 

By the end of the tutorial, you will install and run a demo application, learn how to use Mir in different environments, and learn about features that Mir provides for Mir-based compositors. 

If you are unfamiliar with Mir, start with this tutorial, and then proceed to a [developer tutorial](write-your-first-wayland-compositor) which will guide you through the process of writing a compositor.


## Installing demo applications
Mir demos are available on Debian derivatives, Fedora, and Alpine. For distros
that don't have pre-built binaries, examples can be built from source.

### On Debian and its derivatives

```sh
sudo apt install mir-demos mir-graphics-drivers-desktop
```

### On Fedora
```sh
sudo dnf install mir-demos
```

### On Alpine
```sh
sudo apk add mir-demos mir
```

### On Arch Linux (AUR)
```
https://aur.archlinux.org/packages/mir
```

## Running applications on X11 or Wayland

We'll work with `miral-app`, a script that handles running a Mir shell with a basic GUI.

Mir allows you to run a graphical shell in *desktop mode* or in *kiosk mode*.

Desktop mode means that application windows that are opened are floating, you can move them around the screen, maximize, or minimize them.
Kiosk mode means that the application is opened in fullscreen mode. You are unable to move or resize it.

To run the script in desktop mode:
```sh
miral-app
```

To run the script in kiosk mode:
```sh
miral-app -kiosk
```

## Running shells natively

Mir compositors support running "natively" by launching them from a virtual terminal or a login screen.

### Launching from a virtual terminal

1. Switch to a virtual terminal by pressing CTRL+ALT+F\<Number\> and log in.

2. Run the script:
```sh
miral-app
```

### Launching from a login screen

Open the window manager list, choose **Mir Shell**, log in, and `miral-shell` will be running fullscreen.

## Using on-screen keyboards
Mir-based compositors support on-screen keyboards. 

**Note**: Due to security reasons, some Wayland extensions needed by on-screen keyboards are disabled by default. In this tutorial, we override this setting by passing `--add-wayland-extensions all` when launching `miral-app`.
passing a `--add-wayland-extensions all` flag when launching an example application.

You can use any Wayland compatible on-screen keyboard but as an example, we'll use `ubuntu-frame-osk`. 

1. Install `ubuntu-frame-osk`:
```sh
sudo snap install ubuntu-frame-osk
sudo snap connect ubuntu-frame-osk:wayland
```

2. Start `miral-app`:
```sh
miral-app --add-wayland-extensions all
```

3. Once the shell loads, start the terminal (with `Ctrl-Alt-Shift-T`) and start your on-screen keyboard:
```sh
ubuntu-frame-osk&
```
An on-screen keyboard will pop up.


## Mixing Wayland and X11 clients

You can run X11 applications inside Mir-based compositors. As an example, let's run `xclock` - an X11 application that displays the time.

1. Enable X11 support in `miral-shell`:
```sh
miral-app --enable-x11 true
```

2. Once the shell loads, run `xclock`:
```sh
xclock
``` 
A window with a clock will pop up.

## Remote desktop
Mir supports remote desktops via the VNC protocol. To demo this, you'll use `wayvnc` - a VNC server. 

1. Install `wayvnc`:

```sh
sudo apt install wayvnc
```

2. Install your preferred [VNC client](https://help.ubuntu.com/community/VNC/Clients). 

3. Start the shell with all extensions enabled:
```sh
miral-app --add-wayland-extensions all
```

4. In the shell, open the terminal and run `wayvnc`:
```sh
wayvnc
```
A `wayvnc` server will start and will listen to `localhost`. 

5. Run your VNC client and connect to `localhost`. You will see the exact same view in both the Mir compositor and the VNC viewer.
