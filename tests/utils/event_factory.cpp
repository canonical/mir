/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */
#include "mir_test/event_factory.h"

namespace mis = mir::input::synthesis;

mis::KeyParameters::KeyParameters() :
    device_id(0)
{
}

mis::KeyParameters& mis::KeyParameters::from_device(int new_device_id)
{
    device_id = new_device_id;
    return *this;
}

mis::KeyParameters& mis::KeyParameters::of_scancode(int new_scancode)
{
    scancode = new_scancode;
    return *this;
}

mis::KeyParameters& mis::KeyParameters::with_action(mis::KeyEventAction new_action)
{
    action = new_action;
    return *this;
}

mis::KeyParameters mis::a_key_down_event()
{
    return mis::KeyParameters().with_action(mis::KeyEventAction::Down);
}
