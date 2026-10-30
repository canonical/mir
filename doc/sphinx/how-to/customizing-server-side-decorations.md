---
myst:
  html_meta:
    description: How to customize the colors and metrics of Mir's server-side window decorations.
---

(customizing-server-side-decorations)=

# Customizing server-side decorations in Mir

When Mir draws server-side decorations it uses a built-in appearance: a dark
title bar, a white title, and grey minimize/maximize buttons with a red close
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

| Element             | States                           | Draws                                           |
| ------------------- | -------------------------------- | ----------------------------------------------- |
| `background`        | `focused`, `unfocused`           | The title bar and the borders                   |
| `title`             | `focused`, `unfocused`           | The window title text                           |
| `button`            | `normal`, `hovered`, `unfocused` | The background of the minimize/maximize buttons |
| `button_icon`       | `focused`, `unfocused`           | The minimize/maximize icons                     |
| `close_button`      | `normal`, `hovered`, `unfocused` | The background of the close button              |
| `close_button_icon` | `focused`, `unfocused`           | The close icon                                  |

Buttons are drawn in `normal` while the window has keyboard focus and the
pointer is elsewhere, in `hovered` whenever the pointer is over them, and in
`unfocused` while the window does not have keyboard focus.

Button colors that are not set explicitly are derived from the title bar, so
setting `background` and `title` is enough to restyle the buttons too:

- `button` is the `background` color mixed a quarter of the way towards the
  `title` color, or half way while `hovered`
- `button_icon` and `close_button_icon` are drawn in the `title` color
- `close_button` keeps its own color, so that it stays distinct

Setting one of those colors explicitly stops it being derived; unsetting it
again (by removing the option from a configuration file) brings the derived
color back.

`set_metric()` takes a length in logical pixels. Setting `corner_radius` to a
non-zero value rounds the outer corners of the decorations; the corners are
anti-aliased and the pixels outside them are transparent:

| Metric                       | Sets                                        |
| ---------------------------- | ------------------------------------------- |
| `titlebar_height`            | Visible height of the title bar             |
| `side_border_width`          | Visible width of the left and right borders |
| `bottom_border_height`       | Visible height of the bottom border         |
| `button_width`               | Width of the title bar buttons              |
| `padding_between_buttons`    | Gap between the title bar buttons           |
| `title_font_height`          | Height of the window title text             |
| `resize_corner_input_width`  | Width of the resize handle in a corner      |
| `resize_corner_input_height` | Height of the resize handle in a corner     |
| `corner_radius`              | Radius of the rounded outer corners         |

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
ssd_titlebar_height=32
ssd_side_border_width=6
ssd_bottom_border_height=6
ssd_button_width=24
ssd_padding_between_buttons=6
ssd_title_font_height=14
ssd_resize_corner_input_width=16
ssd_resize_corner_input_height=16
ssd_corner_radius=0

ssd_color_background_focused=#323232
ssd_color_background_unfocused=#808080
ssd_color_title_focused=#ffffff
ssd_color_title_unfocused=#a0a0a0
ssd_color_button_normal=#656565
ssd_color_button_hovered=#989898
ssd_color_button_unfocused=#888888
ssd_color_button_icon_focused=#ffffff
ssd_color_button_icon_unfocused=#a0a0a0
ssd_color_close_button_normal=#a02020
ssd_color_close_button_hovered=#c06060
ssd_color_close_button_unfocused=#805050
ssd_color_close_button_icon_focused=#ffffff
ssd_color_close_button_icon_unfocused=#a0a0a0
```

Colors are written as `#rrggbb` or `#rrggbbaa`. The values shown above are Mir's
defaults: an option that is removed from the file reverts to its default (or to
the color derived from the title bar), and a value that cannot be parsed is
ignored with a warning.

`mir_demo_server` is wired up this way, so you can try these options out by
putting them in `~/.config/mir_demo_server.live-config` and editing it while the
server runs.
