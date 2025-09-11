(how-to-enable-remote-desktop)=

# How to Enable Remote Desktop
Mir supports remote desktops via the VNC protocol. To demo this, you'll use
`wayvnc` - a VNC server. 

1. Install `wayvnc`:
    ```sh
    sudo apt install wayvnc
    ```

2. Install your preferred [VNC client](https://help.ubuntu.com/community/VNC/Clients). 

3. Start the compositor with all extensions enabled:
   ```sh
   miral-app --add-wayland-extensions all
   ```
   **Note**: Due to security reasons, some Wayland extensions needed by on-screen
    keyboards are disabled by default. In this how-to, we override this setting by
    passing `--add-wayland-extensions all` when launching `miral-app`. passing a
    `--add-wayland-extensions all` flag when launching an example application.

4. In the shell, open the terminal and run `wayvnc`:
   ```sh
   wayvnc
   ```
   A `wayvnc` server will start and will listen to `localhost`. 


5. Run your VNC client and connect to `localhost`. You will see the exact same
   view in both the Mir compositor and the VNC viewer.
