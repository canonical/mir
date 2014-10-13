/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include "example_input_event_filter.h"

#include "mir/server.h"

#include <linux/input.h>

namespace me = mir::examples;

me::QuitFilter::QuitFilter(std::function<void()> const& quit_action)
    : quit_action{quit_action}
{
}

bool me::QuitFilter::handle(MirEvent const& event)
{
    if (event.type == mir_event_type_key &&
        event.key.action == mir_key_action_down &&
        (event.key.modifiers & mir_key_modifier_alt) &&
        (event.key.modifiers & mir_key_modifier_ctrl) &&
        event.key.scan_code == KEY_BACKSPACE)
    {
        quit_action();
        return true;
    }

    return false;
}
