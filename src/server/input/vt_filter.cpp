/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Robert Ancell <robert.ancell@canonical.com>
 */

#include "mir/input/vt_filter.h"
#include "mir_toolkit/event.h"

#include <linux/input.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

namespace
{
void set_active_vt(int vt)
{
    auto console_fd = open("/dev/tty0", O_RDONLY | O_NDELAY);
    ioctl(console_fd, VT_ACTIVATE, vt);
    close(console_fd);
}
}

bool mir::input::VTFilter::handle(MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
        return false;

    auto const input_event = mir_event_get_input_event(&event);

    if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
        return false;

    auto const keyboard_event = mir_input_event_get_keyboard_event(input_event);
    auto const modifier_state  = mir_keyboard_event_modifiers(keyboard_event);

    if (mir_keyboard_event_action(keyboard_event) == mir_keyboard_action_down &&
        (modifier_state & mir_input_event_modifier_alt) &&
        (modifier_state & mir_input_event_modifier_ctrl))
    {
        switch (mir_keyboard_event_scan_code(keyboard_event))
        {
        case KEY_F1:
            set_active_vt(1);
            return true;
        case KEY_F2:
            set_active_vt(2);
            return true;
        case KEY_F3:
            set_active_vt(3);
            return true;
        case KEY_F4:
            set_active_vt(4);
            return true;
        case KEY_F5:
            set_active_vt(5);
            return true;
        case KEY_F6:
            set_active_vt(6);
            return true;
        case KEY_F7:
            set_active_vt(7);
            return true;
        case KEY_F8:
            set_active_vt(8);
            return true;
        case KEY_F9:
            set_active_vt(9);
            return true;
        case KEY_F10:
            set_active_vt(10);
            return true;
        case KEY_F11:
            set_active_vt(11);
            return true;
        case KEY_F12:
            set_active_vt(12);
            return true;
        }
    }

    return false;
}
