# How to specify startup applications

When starting a compositor or display server, you usually want to start a few
other applications. For example you might want to start a background viewer, a
terminal, and a status bar. Doing so manually like what we've do would be
immensely annoying, so let's add an option for users to specify startup apps.

**Note**: [Write Your First Wayland
Compositor](../tutorial/write-your-first-wayland-compositor.md) is a prerequisite for
this how-to.

______________________________________________________________________

We'll start by creating an instance of `miral::ExternalClientLauncher`, this
will provide a way to launch programs by name. Then, we'll create a function
that will take the list of startup apps and launch them. Finally, we just need
to pass the external client launcher and our configuration option to
{func}`run_with` so that they're registered with the compositor.

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
+            external_client_launcher.launch(std::vector<std::string>{app});
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
