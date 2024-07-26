# How to Handle Keyboard Input

This how-to guide will show you the basics of handling keyboard input in Mir
based compositors.

Handling keyboard input allows for great flexibility in your compositor. For
example, it's widely known that ALT + F4 closes the currently open application.
Some window managers take this a step further by allowing all navigation
between windows to be done via the keyboard. 

We'll implement a couple of shortcuts:
- CTRL + ALT + T / CTRL + ALT + SHIFT + T: To launch a terminal emulator
- CTRL + ALT + Backspace: To stop the compositor

For ease of reading (and copying), we'll split the changes into three parts:
headers, code, and options.

**Note**: This builds on top of [Write Your First Wayland
Compositor](../write-your-first-wayland-compositor.md)

### Header changes
Here we just include `append_event_filter.h` for the declaration of
`AppendEventFilter` and `toolkit_event` to get definitions for event types and
functions. We import everything from `miral::toolkit` to make the code a bit
easier to read.
```diff
@@ -1,10 +1,15 @@
 #include <miral/runner.h>
+#include <miral/append_event_filter.h>
 #include <miral/minimal_window_manager.h>
 #include <miral/set_window_management_policy.h>
+#include <miral/toolkit_event.h>

 using namespace miral;
+using namespace miral::toolkit;
```

### Code changes
This big block of code can be broken down into three parts: 
1. Declaring the name of the terminal we'll use and an external client launcher
   that we'll use to launch it. 
2. Filtering Mir events until we obtain a keyboard key down event. 
3. Handling the combinations we want.

```diff
+    std::string terminal_cmd{"kgx"};
+    miral::ExternalClientLauncher external_client_launcher;
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
Here we simply add the external client launcher we declared in the previous
subsection, and an event filter that uses the code we specified in the previous
subsection.

```diff
     return runner.run_with(
         {
             set_window_management_policy<MinimalWindowManager>(),
+            external_client_launcher,
+            miral::AppendEventFilter{builtin_keybinds}
         });
 }
```

This can be easily extended to read keybinds from config files or control
various other aspects of the compositor. The only limit here is your
imagination.
