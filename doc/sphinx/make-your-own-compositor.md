---
discourse: 4911,5164,5603,6756,8037
---

# Using Mir Libraries to Build Your Own Wayland Compositor
This tutorial will guide you through writing a simple compositor: the installation of dependencies, building it, and running it. You'll be impressed by how easy it is to create a Wayland compositor using Mir!

As you continue to develop it your compositor has access to all the features of Mir demonstrated in [the demo tutorial](explore-mir-using-demos).

## Assumptions
This tutorial assumes that:
1. The reader is familiar with C++ and CMake.
2. The reader has cmake and a C++ compiler installed

## A barebones Mir compositor
This section will cover the needed dependencies and how to install them, the
minimum code needed for a Mir compositor, how to build this code, and finally
how to run your compositor.

### Dependencies
Before you start coding, you'll need to install `libmiral`, and
`mir-graphics-drivers-desktop` which can be done on different distros as
follows:

Installing dependencies on Debian and its derivatives:
```sh
sudo apt install libmiral-dev mir-graphics-drivers-desktop
```

<details>
<summary> Installing Dependencies on Other Distros </summary>

Installing dependencies on Fedora 
```sh
sudo dnf install mir-devel libxkbcommon
```
Installing dependencies on Alpine 
```sh
sudo apk add mir-dev
```
</details>


### Code
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

This program creates a floating window manager with basic window management
capabilities such as controlling multiple windows, minimizing and maximizing,
and handling mouse input. This is done with the help of MirAL (Mir Abstraction
Layer) which gives you a high level interface to work with Mir.

### Building
To compile this simple program, you can use this `CMakeLists.txt` file:
```cmake
cmake_minimum_required(VERSION 3.5)

project(demo-mir-compositor)

set(CMAKE_CXX_STANDARD 23)

include(FindPkgConfig)
pkg_check_modules(MIRAL miral REQUIRED)
pkg_check_modules(XKBCOMMON xkbcommon REQUIRED)

add_executable(demo-mir-compositor main.cpp)

target_include_directories(demo-mir-compositor PUBLIC SYSTEM ${MIRAL_INCLUDE_DIRS})
target_link_libraries(     demo-mir-compositor               ${MIRAL_LDFLAGS})
target_link_libraries(     demo-mir-compositor               ${XKBCOMMON_LIBRARIES})
```

Then to build:
```sh
cmake -B build
cmake --build build
```

### Running
To run, you can run nested in an X or Wayland session, or from a virtual
terminal, just like the demo applications in [Explore What Mir Is Capable of
Using Mir Demos](explore-mir-using-demos.md). For example, if we were to run
inside an existing Wayland session, we'd run the following command:
```sh
WAYLAND_DISPLAY=wayland-99 ./build/demo-mir-compositor 
```
You'll see a window housing the compositor with a black void filling it. To
fill this void with some content, you can run the following from another
terminal:
```sh
WAYLAND_DISPLAY=wayland-99 bomber
```
You're free to replace `bomber` with any other Wayland compatible application.

## Startup Applications
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
+    auto run_startup_apps = [&](std::vector<std::string> const& apps) 
+    {
+        for(auto const& app : apps)
+        {
+            external_client_launcher.launch(std::vector<std::string>{app}});
+        }
+    };
+
     return runner.run_with(
         {
-            set_window_management_policy<MinimalWindowManager>()
+            set_window_management_policy<MinimalWindowManager>(),
+            external_client_launcher,
+            miral::ConfigurationOption{run_startup_apps, "startup-app", "App to run at startup (can be specified multiple times)"},
         });
 }
```

Now, to start `kgx` and `bomber` on startup, we can do:
```sh
./build/demo-mir-compositor --startup-app kgx --startup-app bomber
```

## Keyboard shortcuts
What if you accidentally close the only open terminal? Or you want to log out?
Keyboard shortcuts are a good option that give you great flexibility without
much coding effort. Let's see how they're implemented in Mir. 

We'll implement a couple of shortcuts:
- CTRL + ALT + T / CTRL + ALT + SHIFT + T: To launch a terminal emulator
- CTRL + ALT + Backspace: To stop the compositor

Since the changes are bigger than usual, we'll split them into three parts:
headers, code, and options.

### Header changes
Here we just include `append_event_filter.h` to be able to filter and react to
certain events and `toolkit_event` to get definitions for event types and
functions. We import everything from `miral::toolkit` to make the code a bit
easier to read.
```diff
@@ -1,10 +1,15 @@
 #include <miral/runner.h>
+#include <miral/append_event_filter.h>
 #include <miral/configuration_option.h>
 #include <miral/external_client.h>
 #include <miral/minimal_window_manager.h>
 #include <miral/set_window_management_policy.h>
+#include <miral/toolkit_event.h>
 
 using namespace miral;
+using namespace miral::toolkit;
```
 
### Code changes
This big block of code can be broken down into two parts: the filtering of
Mir events until we obtain a keyboard key down event, and the handling of the
combinations we want.

```diff
+    std::string terminal_cmd{"kgx"};
+
+    auto const builtin_keybinds = [&](MirEvent const* event)
+        {
+            // Skip non-input events
+            if (mir_event_get_type(event) != mir_event_type_input)
+                return false;
+
+            // Skip non-key input events
+            MirInputEvent const* input_event = mir_event_get_input_event(event);
+            if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
+                return false;
+
+            // Skip anything but down presses
+            MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(input_event);
+            if (mir_keyboard_event_action(kev) != mir_keyboard_action_down)
+                return false;
+
+            // CTRL + ALT must be pressed
+            MirInputEventModifiers mods = mir_keyboard_event_modifiers(kev);
+            if (!(mods & mir_input_event_modifier_alt) || !(mods & mir_input_event_modifier_ctrl))
+                return false;
+
+            switch (mir_keyboard_event_keysym(kev))
+            {
+            // Exit program
+            case XKB_KEY_BackSpace:
+                runner.stop();
+                return true;
+
+            // Launch terminal
+            case XKB_KEY_t:
+            case XKB_KEY_T:
+                external_client_launcher.launch({terminal_cmd});
+                return false;
+
+            // Do nothing
+            default:
+                return false;
+            };
+        };
```

### Options changes
Here we simply add an event filter that uses the code we specified in the previous subsection.

```diff
     return runner.run_with(
         {
             set_window_management_policy<MinimalWindowManager>(),
             external_client_launcher,
             miral::ConfigurationOption{run_startup_apps, "startup-apps", "Colon separated list of startup apps", ""},
+            miral::AppendEventFilter{builtin_keybinds}
         });
 }
```

Now, even if you closed your startup programs by accident, you can use your compositor. No need to restart!
