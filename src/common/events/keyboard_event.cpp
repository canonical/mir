/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"

MirKeyboardEvent::MirKeyboardEvent()
{
    event.initInput();
    event.getInput().initKey();
}

MirKeyboardAction MirKeyboardEvent::action() const
{
    return static_cast<MirKeyboardAction>(event.asReader().getInput().getKey().getAction());
}

void MirKeyboardEvent::set_action(MirKeyboardAction action)
{
    event.getInput().getKey().setAction(static_cast<mir::capnp::KeyboardEvent::Action>(action));
}

int32_t MirKeyboardEvent::key_code() const
{
    return event.asReader().getInput().getKey().getKeyCode();
}

void MirKeyboardEvent::set_key_code(int32_t key_code)
{
    event.getInput().getKey().setKeyCode(key_code);
}

int32_t MirKeyboardEvent::scan_code() const
{
    return event.asReader().getInput().getKey().getScanCode();
}

void MirKeyboardEvent::set_scan_code(int32_t scan_code)
{
    event.getInput().getKey().setScanCode(scan_code);
}

char const* MirKeyboardEvent::text() const
{
    return event.asReader().getInput().getKey().getText().cStr();
}

void MirKeyboardEvent::set_text(char const* str)
{
    event.getInput().getKey().setText(str);
}
