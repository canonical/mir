/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "event_pipe.h"

namespace mf = mir::frontend;

void mf::EventPipe::set_target(std::weak_ptr<EventSink> const& s)
{
    target = s;
}

void mf::EventPipe::handle_event(MirEvent const& e)
{
    // In future, we might put e on a queue and wait for some background
    // thread to push it through to target. But that's not required right now.

    std::shared_ptr<EventSink> p = target.lock();
    if (p)
        p->handle_event(e);
}
