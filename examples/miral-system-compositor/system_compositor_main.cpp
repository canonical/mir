/*
 * Copyright © 2019-2020 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include <miral/append_event_filter.h>
#include <miral/command_line_option.h>
#include <miral/display_configuration_option.h>
#include <miral/runner.h>

#include <linux/input.h>

#include <atomic>

int main(int argc, char const* argv[])
{
    using namespace miral;

    MirRunner runner{argc, argv};

    std::atomic<bool> allow_quit{true};

    CommandLineOption disable_quit{
        [&](bool disable_quit) { allow_quit = !disable_quit; },
        "disable-quit",
        "Disable Ctrl-Alt-Shift-BkSp to quit"};

    AppendEventFilter quit_on_ctrl_alt_bksp{[&](MirEvent const* event)
        {
            if (!allow_quit)
                return false;

            if (mir_event_get_type(event) != mir_event_type_input)
                return false;

            MirInputEvent const* input_event = mir_event_get_input_event(event);
            if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
                return false;

            MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(input_event);
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
