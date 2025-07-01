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

#include <miral/append_keyboard_event_filter.h>
#include <miral/command_line_option.h>
#include <miral/display_configuration_option.h>
#include <miral/runner.h>
#include <miral/toolkit_event.h>

#include <linux/input.h>

#include <atomic>

int main(int argc, char const* argv[])
{
    using namespace miral;
    using namespace miral::toolkit;

    MirRunner runner{argc, argv};

    std::atomic<bool> allow_quit{true};

    ConfigurationOption disable_quit{
        [&](bool disable_quit) { allow_quit = !disable_quit; },
        "disable-quit",
        "Disable Ctrl-Alt-Shift-BkSp to quit"};

    AppendKeyboardEventFilter quit_on_ctrl_alt_bksp{[&](MirKeyboardEvent const* kev)
        {
            if (!allow_quit)
                return false;

            if (mir_keyboard_event_action(kev) != mir_keyboard_action_down)
                return false;

            MirInputEventModifiers mods = mir_keyboard_event_modifiers(kev);
            if (!(mods & mir_input_event_modifier_alt) ||
                !(mods & mir_input_event_modifier_ctrl)||
                !(mods & mir_input_event_modifier_shift))
                return false;

            switch (mir_keyboard_event_scan_code(kev))
            {
            case KEY_BACKSPACE:
                runner.stop();
                return true;

            default:
                return false;
            };
        }};

    return runner.run_with({quit_on_ctrl_alt_bksp, disable_quit, display_configuration_options });
}
