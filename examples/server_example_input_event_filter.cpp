/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_input_event_filter.h"

#include "mir/server.h"
#include "mir/input/composite_event_filter.h"

#include <linux/input.h>

namespace me = mir::examples;

///\example server_example_input_event_filter.cpp
/// Demonstrate a custom input by making Ctrl+Alt+BkSp stop the server

me::QuitFilter::QuitFilter(std::function<void()> const& quit_action)
    : quit_action{quit_action}
{
}

bool me::QuitFilter::handle(MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
        return false;
    MirInputEvent const* input_event = mir_event_get_input_event(&event);
    if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
        return false;
    MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(input_event);
    if (mir_keyboard_event_action(kev) != mir_keyboard_action_down)
        return false;
    MirInputEventModifiers mods = mir_keyboard_event_modifiers(kev);
    if (!(mods & mir_input_event_modifier_alt) || !(mods & mir_input_event_modifier_ctrl))
        return false;
    if (mir_keyboard_event_scan_code(kev) == KEY_BACKSPACE)
    {
        quit_action();
        return true;
    }

    return false;
}

auto me::make_quit_filter_for(mir::Server& server)
-> std::shared_ptr<mir::input::EventFilter>
{
    auto const quit_filter = std::make_shared<me::QuitFilter>([&]{ server.stop(); });

    server.add_init_callback([quit_filter, &server]
        { server.the_composite_event_filter()->append(quit_filter); });

    return quit_filter;
}
