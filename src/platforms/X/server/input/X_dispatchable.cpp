/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "X_dispatchable.h"
#include "mir/events/event_private.h"
#include "../debug.h"

#include <chrono>
#include <X11/Xlib.h>
#include <linux/input.h>

namespace mi = mir::input;
namespace mix = mi::X;
namespace md = mir::dispatch;

extern ::Display *x_dpy;

mix::XDispatchable::XDispatchable(int raw_fd)
    : fd(raw_fd), sink(nullptr)
{
    CALLED
}

mir::Fd mix::XDispatchable::watch_fd() const
{
    CALLED
    return fd;
}

bool mix::XDispatchable::dispatch(md::FdEvents /*events*/)
{
    CALLED
    XEvent xev;

    XNextEvent(x_dpy, &xev);

    if (sink)
    {
        if(xev.type == KeyPress)
        {
        	MirEvent event;

            mir::log_info("Key pressed");

            event.key.type = mir_event_type_key;
            event.key.source_id = 0;
            event.key.action = mir_key_action_up;
            event.key.modifiers = mir_input_event_modifier_none;
            event.key.key_code = XKB_KEY_q;
            event.key.scan_code = KEY_Q;
            event.key.repeat_count = 0;

            event.key.event_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                       std::chrono::system_clock::now().time_since_epoch()).count();

            sink->handle_input(event);
        }
    }
    else
    {
        mir::log_info("input event detected with no sink to handle it");
    }
    return true;
}

md::FdEvents mix::XDispatchable::relevant_events() const
{
    CALLED
    return md::FdEvent::readable;
}

void mix::XDispatchable::set_input_sink(mi::InputSink *input_sink)
{
    CALLED

    sink = input_sink;
}

void mix::XDispatchable::unset_input_sink()
{
    CALLED

    sink = nullptr;
}
