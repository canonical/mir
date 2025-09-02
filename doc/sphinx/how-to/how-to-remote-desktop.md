# How to Remote Desktop
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
