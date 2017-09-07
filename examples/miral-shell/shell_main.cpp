/*
 * Copyright © 2016-2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * under the terms of the GNU General Public License version 2 or 3 as as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "tiling_window_manager.h"
#include "floating_window_manager.h"
#include "titlebar_config.h"
#include "spinner/splash.h"

#include <miral/display_configuration_option.h>
#include <miral/runner.h>
#include <miral/window_management_options.h>
#include <miral/append_event_filter.h>
#include <miral/debug_extension.h>
#include <miral/internal_client.h>
#include <miral/command_line_option.h>
#include <miral/cursor_theme.h>
#include <miral/keymap.h>

#include <linux/input.h>

int main(int argc, char const* argv[])
{
    using namespace miral;

    std::function<void()> shutdown_hook{[]{}};

    SpinnerSplash spinner;
    InternalClientLauncher launcher;
    ActiveOutputsMonitor outputs_monitor;
    WindowManagerOptions window_managers
        {
            add_window_manager_policy<FloatingWindowManagerPolicy>("floating", spinner, launcher, shutdown_hook),
            add_window_manager_policy<TilingWindowManagerPolicy>("tiling", spinner, launcher, outputs_monitor),
        };

    MirRunner runner{argc, argv};

    runner.add_stop_callback([&] { shutdown_hook(); });

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

            if (mir_keyboard_event_scan_code(kev) != KEY_BACKSPACE)
                return false;

            runner.stop();
            return true;
        };

    Keymap config_keymap;
    DebugExtension debug_extensions;

    return runner.run_with(
        {
            CommandLineOption{[&](std::string const& ) { },
                              "desktop_file_hint", "Ignored for Unity8 compatibility", "miral-shell.desktop"},
            CursorTheme{"DMZ-White"},
            window_managers,
            display_configuration_options,
            launcher,
            outputs_monitor,
            config_keymap,
            debug_extensions,
            AppendEventFilter{quit_on_ctrl_alt_bksp},
            StartupInternalClient{"Intro", spinner},
            pre_init(CommandLineOption{[&](std::string const& typeface) { ::titlebar::font_file(typeface); },
                              "shell-titlebar-font", "font file to use for titlebars", ::titlebar::font_file()})
        });
}
