## How to Use On-Screen Keyboards
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
