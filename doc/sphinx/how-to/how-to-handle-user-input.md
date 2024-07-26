# How to Handle User Input

**Note**: This builds on top of [Write Your First Wayland
Compositor](../write-your-first-wayland-compositor.md)

What if you accidentally close the only open terminal? Or you want to log out?
Keyboard shortcuts are a good option that give you great flexibility without
much coding effort. Let's see how they're implemented in Mir.

We'll implement a couple of shortcuts:
- CTRL + ALT + T / CTRL + ALT + SHIFT + T: To launch a terminal emulator
- CTRL + ALT + Backspace: To stop the compositor

For ease of reading (and copying), we'll split the changes into three parts:
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
Here we simply add an event filter that uses the code we specified in the
previous subsection.

```diff
     return runner.run_with(
         {
             set_window_management_policy<MinimalWindowManager>(),
+            miral::AppendEventFilter{builtin_keybinds}
         });
 }
```

Now, even if you closed your startup programs by accident, you can still use
your compositor. No need to restart!
