## Using Mir Libraries to Build Your Own Wayland Compositor
At this point, you're probably taken away by Mir's capabilities and want to use
it and the supporting libraries to build your own Wayland compositor. This
section will guide you through the installation and basic usage of Mir.

### A barebones Mir compositor

The following code block is the bare minimum you need to run a Mir compositor:
```cpp
#include <miral/runner.h>
#include <miral/minimal_window_manager.h>
#include <miral/set_window_management_policy.h>

using namespace miral;

int main(int argc, char const* argv[])
{
    MirRunner runner{argc, argv};

    return runner.run_with(
        {
            set_window_management_policy<MinimalWindowManager>()
        });
}
```

You can se it's barely longer than the usual C++ "Hello, world!" example. This
code should give you a minimal compositor with basic window management
capabilities such as controlling multiple windows, minimizing and maximizing,
handling input, etc... This is done with the help of MirAL (Mir Abstraction
Layer) which gives you a high level interface to work with Mir!

To compile, you'll need to install `libmiral`, and
`mir-graphics-drivers-desktop` which can be done on Ubuntu (and Debian
derivatives) with the following command:
```sh
sudo apt install libmiral-dev mir-graphics-drivers-desktop
```

**Note**: `mir-graphics-drivers-desktop` is only needed for Debian based distros. 

To compile this simple program:
```sh
g++ main.cpp -I/usr/include/mircore -I/usr/include/miral -lmiral -o demo-mir-compositor
```

To run, you can run nested in an X or Wayland session, or from a virtual
terminal, just like the demo applications in [Explore What Mir Is Capable of
Using Mir Demos](explore-mir-using-demos.md). For example, if we were to run
inside an existing Wayland session, we'd run the following command:
```sh
WAYLAND_DISPLAY=wayland-99 ./demo-mir-compositor 
```
You'll see a window housing the compositor with a black void filling it. To
fill this void with some content, you can run the following from another
terminal:
```sh
WAYLAND_DISPLAY=wayland-99 bomber
```
You're free to replace `bomber` with any other Wayland compatible application.

### Startup Applications
When starting a compositor or display server, you usually want to start a few
other applications. For example you might want to start a background viewer, a
terminal, and a status bar. Doing so manually like what we've do would be
immensely annoying, so let's add an option for users to pass a list of startup
apps!

It's up to you how you want your command line interface will be. Let's say
we'll take a list of colon separated program names, then we'll launch each of
those individually. 

We'll start by creating an instance of `miral::ExternalClientLauncher`, this
will provide a way to launch programs by name. Then, we'll create a function
that will take the colon separated string and launch each application
separately. Finally, we just need to pass the external client launcher and our
configuration option to {func}`run_with` so that they're registered with the
compositor.

```diff
 #include <miral/runner.h>
+#include <miral/configuration_option.h>
+#include <miral/external_client.h>
 #include <miral/minimal_window_manager.h>
 #include <miral/set_window_management_policy.h>
 
@@ -9,8 +10,22 @@ int main(int argc, char const* argv[])
 {
     MirRunner runner{argc, argv};
 
+    miral::ExternalClientLauncher external_client_launcher;
+    
+    auto run_startup_apps = [&](std::string const& apps) 
+    {
+        for(auto i = std::begin(apps); i != std::end(apps);)
+        {
+            auto const j = std::find(i, std::end(apps), ':');
+            external_client_launcher.launch(std::vector<std::string>{std::string{i, j}});
+            if((i = j) != std::end(apps)) ++i;
+        }
+    };
+
     return runner.run_with(
         {
-            set_window_management_policy<MinimalWindowManager>()
+            set_window_management_policy<MinimalWindowManager>(),
+            external_client_launcher,
+            miral::ConfigurationOption{run_startup_apps, "startup-apps", "Colon separated list of startup apps", ""},
         });
 }
```

Now, to start `kgx` and `bomber` on startup, we can do:
```sh
miral-shell --startup-apps kgx:bomber
```
