# Specifying CSD/SSD preference

Clients can ask the compositor to use server or client side decorations, or request the compositor to choose for them. {class}`miral::Decorations` allows you to customize how the server deals with these requests.

This how-to will show you how to specify the behavior of the compositor regarding these requests.

**Note**: [How to Specify Startup Apps](how-to-specify-startup-apps.md) is a
prerequisite for this how-to.

______________________________________________________________________

To make the compositor prefer server side decorations when the client doesn't
specify a preference, you only need to change two lines:

```diff
@@ -1,6 +1,7 @@
 #include <miral/runner.h>
 #include <miral/configuration_option.h>
+#include <miral/decorations.h>
 #include <miral/external_client.h>
 #include <miral/minimal_window_manager.h>
 #include <miral/set_window_management_policy.h>
@@ -74,5 +75,6 @@ int main(int argc, char const* argv[])
             external_client_launcher,
             miral::ConfigurationOption{run_startup_apps, "startup-app", "App to run at startup (can be specified multiple times)"},
+            miral::Decorations::prefer_ssd(),
         });
 }
```

That's it! You can now build and run your compositor and try running a Wayland
compatible application to see how its decorations change:

```sh
./build/demo-mir-compositor --startup-app kgx --startup-app bomber
```

MirAL also has other strategies: {func}`miral::Decorations::prefer_csd`,
{func}`miral::Decorations::always_ssd`, and
{func}`miral::Decorations::always_csd`. Try playing around with different
strategies and seeing how they behave differently.
