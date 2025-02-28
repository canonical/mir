/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miral/minimal_window_manager.h"
#include "tiling_window_manager.h"
#include "floating_window_manager.h"
#include "wallpaper_config.h"
#include "spinner/splash.h"

#include <miral/display_configuration_option.h>
#include <miral/external_client.h>
#include <miral/runner.h>
#include <miral/window_management_options.h>
#include <miral/append_event_filter.h>
#include <miral/internal_client.h>
#include <miral/command_line_option.h>
#include <miral/cursor_theme.h>
#include <miral/decorations.h>
#include <miral/keymap.h>
#include <miral/toolkit_event.h>
#include <miral/x11_support.h>
#include <miral/wayland_extensions.h>
#include <miral/enable_mousekeys.h>

#include <xkbcommon/xkbcommon-keysyms.h>

#include <cstring>

namespace
{
struct ConfigureDecorations
{
    miral::Decorations const decorations{[]
        {
            if (auto const strategy = getenv("MIRAL_SHELL_DECORATIONS"))
            {
                if (strcmp(strategy, "always-ssd") == 0) return miral::Decorations::always_ssd();
                if (strcmp(strategy, "prefer-ssd") == 0) return miral::Decorations::prefer_ssd();
                if (strcmp(strategy, "always-csd") == 0) return miral::Decorations::always_csd();
            }
            return miral::Decorations::prefer_csd();
        }()};

    void operator()(mir::Server& s) const
    {
        decorations(s);
    }
};
}

int main(int argc, char const* argv[])
{
    using namespace miral;
    using namespace miral::toolkit;

    std::function<void()> shutdown_hook{[]{}};

    SpinnerSplash spinner;
    InternalClientLauncher launcher;
    auto focus_stealing_prevention = FocusStealing::allow;
    WindowManagerOptions window_managers
        {
            add_window_manager_policy<FloatingWindowManagerPolicy>("floating", spinner, launcher, shutdown_hook, focus_stealing_prevention),
            add_window_manager_policy<TilingWindowManagerPolicy>("tiling", spinner, launcher),
        };

    MirRunner runner{argc, argv};

    runner.add_stop_callback([&] { shutdown_hook(); });

    ExternalClientLauncher external_client_launcher;

    std::string terminal_cmd{"miral-terminal"};

    auto const quit_on_ctrl_alt_bksp = [&](MirEvent const* event)
        {
            if (mir_event_get_type(event) != mir_event_type_input)
                return false;

            MirInputEvent const* input_event = mir_event_get_input_event(event);
            if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
                return false;

            MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(input_event);
            if (mir_keyboard_event_action(kev) != mir_keyboard_action_down)
                return false;

            MirInputEventModifiers mods = mir_keyboard_event_modifiers(kev);
            if (!(mods & mir_input_event_modifier_alt) || !(mods & mir_input_event_modifier_ctrl))
                return false;

            switch (mir_keyboard_event_keysym(kev))
            {
            case XKB_KEY_BackSpace:
                runner.stop();
                return true;

            case XKB_KEY_t:
            case XKB_KEY_T:
                external_client_launcher.launch({terminal_cmd});
                return true;

            case XKB_KEY_x:
            case XKB_KEY_X:
                external_client_launcher.launch_using_x11({"xterm"});
                return true;

            default:
                return false;
            };
        };

    Keymap config_keymap;

    auto run_startup_apps = [&](std::string const& apps)
    {
      for (auto i = begin(apps); i != end(apps); )
      {
          auto const j = find(i, end(apps), ':');
          external_client_launcher.launch(std::vector<std::string>{std::string{i, j}});
          if ((i = j) != end(apps)) ++i;
      }
    };

    auto const to_focus_stealing = [](bool focus_stealing_prevention)
    {
        if (focus_stealing_prevention)
            return FocusStealing::prevent;
        else
            return FocusStealing::allow;
    };

    return runner.run_with(
        {
            CursorTheme{"default:DMZ-White"},
            WaylandExtensions{},
            X11Support{},
            ConfigureDecorations{},
            pre_init(ConfigurationOption{[&](bool is_set)
                    { focus_stealing_prevention = to_focus_stealing(is_set); },
                    "focus-stealing-prevention", "prevent focus stealing", false}),
            window_managers,
            display_configuration_options,
            external_client_launcher,
            launcher,
            config_keymap,
            AppendEventFilter{quit_on_ctrl_alt_bksp},
            StartupInternalClient{spinner},
            ConfigurationOption{run_startup_apps, "startup-apps", "Colon separated list of startup apps", ""},
            pre_init(ConfigurationOption{[&](std::string const& typeface) { ::wallpaper::font_file(typeface); },
                                         "shell-wallpaper-font", "font file to use for wallpaper", ::wallpaper::font_file()}),
            ConfigurationOption{[&](std::string const& cmd) { terminal_cmd = cmd; },
                                "shell-terminal-emulator", "terminal emulator to use", terminal_cmd},
            miral::EnableMouseKeys{},
        });
}
