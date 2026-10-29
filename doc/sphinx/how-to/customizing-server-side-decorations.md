---
myst:
  html_meta:
    description: How to customize the colors and metrics of Mir's server-side window decorations.
---

(customizing-server-side-decorations)=

# Customizing server-side decorations in Mir

When Mir draws server-side decorations it uses a built-in appearance: a dark
titlebar, a white title, and grey minimize/maximize buttons with a red close
button. {class}`miral::DecorationStyle` lets you change those colors and the
sizes of the decoration's parts.

Requires MirAL 6.2.

**Note**: [Specifying CSD/SSD Preference](specifying-csd-ssd-preference.md) is a
prerequisite for this how-to.

______________________________________________________________________

## Setting a style in code

Create a {class}`miral::DecorationStyle`, describe the appearance you want, and
pass it to `run_with()`:

```diff
@@ -1,6 +1,7 @@
 #include <miral/runner.h>
 #include <miral/configuration_option.h>
 #include <miral/decorations.h>
+#include <miral/decoration_style.h>
 #include <miral/external_client.h>
 #include <miral/minimal_window_manager.h>
 #include <miral/set_window_management_policy.h>
@@ -70,6 +71,11 @@ int main(int argc, char const* argv[])
 {
+    miral::DecorationStyle decoration_style;
+    decoration_style.set_metric(miral::DecorationStyle::Metric::titlebar_height, 32);
+    decoration_style.set_color(
+        miral::DecorationStyle::Element::background,
+        miral::DecorationStyle::State::focused, 0x20, 0x40, 0x80);
+
     return runner.run_with(
         {
             external_client_launcher,
             miral::Decorations::prefer_ssd(),
+            decoration_style,
         });
 }
```

`set_color()` takes the element, the state it is drawn in, and the red, green
and blue channels (plus an optional alpha channel, which defaults to opaque):

| Element             | States                 | Draws                                           |
| ------------------- | ---------------------- | ----------------------------------------------- |
| `background`        | `focused`, `unfocused` | The titlebar and the borders                    |
| `title`             | `focused`, `unfocused` | The window title text                           |
| `button`            | `normal`, `hovered`    | The background of the minimize/maximize buttons |
| `button_icon`       | —                      | The minimize/maximize icons                     |
| `close_button`      | `normal`, `hovered`    | The background of the close button              |
| `close_button_icon` | —                      | The close icon                                  |

`set_metric()` takes a length in logical pixels:

| Metric                       | Sets                                        |
| ---------------------------- | ------------------------------------------- |
| `titlebar_height`            | Visible height of the titlebar              |
| `side_border_width`          | Visible width of the left and right borders |
| `bottom_border_height`       | Visible height of the bottom border         |
| `button_width`               | Width of the titlebar buttons               |
| `padding_between_buttons`    | Gap between the titlebar buttons            |
| `title_font_height`          | Height of the window title text             |
| `resize_corner_input_width`  | Width of the resize handle in a corner      |
| `resize_corner_input_height` | Height of the resize handle in a corner     |

Changes made before the server starts are picked up when it starts; changes made
afterwards are applied to windows that are already decorated.

## Setting a style from a configuration file

Constructing {class}`miral::DecorationStyle` with a
`miral::live_config::Store` registers the same settings as configuration
options, so they can be set — and changed while the compositor is running — from
a configuration file:

```diff
+    miral::live_config::IniFile config_store;
+    miral::DecorationStyle decoration_style{config_store};
+    miral::ConfigFile config_file{
+        runner, "demo-mir-compositor.config", miral::ConfigFile::Mode::reload_on_change,
+        [&config_store](auto& istream, auto& path) { config_store.load_file(istream, path); }};
```

See [How to add configuration overrides](how-to-add-configuration-overrides.md)
for more on wiring up a configuration store.

The options are:

```ini
decorations_titlebar_height=32
decorations_side_border_width=6
decorations_bottom_border_height=6
decorations_button_width=24
decorations_padding_between_buttons=6
decorations_title_font_height=14
decorations_resize_corner_input_width=16
decorations_resize_corner_input_height=16

decorations_color_background_focused=#323232
decorations_color_background_unfocused=#808080
decorations_color_title_focused=#ffffff
decorations_color_title_unfocused=#a0a0a0
decorations_color_button_normal=#606060
decorations_color_button_hovered=#a0a0a0
decorations_color_button_icon=#ffffff
decorations_color_close_button_normal=#a02020
decorations_color_close_button_hovered=#c06060
decorations_color_close_button_icon=#ffffff
```

Colors are written as `#rrggbb` or `#rrggbbaa`. The values shown above are Mir's
defaults: an option that is removed from the file reverts to its default, and a
value that cannot be parsed is ignored with a warning.

`mir_demo_server` is wired up this way, so you can try these options out by
putting them in `~/.config/mir_demo_server.live-config` and editing it while the
server runs.
