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
