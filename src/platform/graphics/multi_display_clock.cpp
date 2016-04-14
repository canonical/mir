/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/graphics/multi_display_clock.h"

namespace mir { namespace graphics {

void MultiDisplayClock::add_child_clock(std::weak_ptr<DisplayClock> w)
{
    Lock lock(frame_mutex);
    if (auto dc = w.lock())
    {
        void const* id = dc.get();
        children[id] = Child{std::move(w), {}, {}};
        synchronize(lock);
        dc->set_frame_callback( std::bind(&MultiDisplayClock::on_child_frame,
                                         this, id,
                                         std::placeholders::_1) );
    }
}

void MultiDisplayClock::synchronize(Lock const&)
{
    baseline = last_multi_frame;
    for (auto& child : children)
        child.second.baseline = child.second.last_frame;
}

void MultiDisplayClock::on_child_frame(void const* child_id,
                                       Frame const& child_frame)
{
    FrameCallback cb;
    Frame cb_frame;

    {
        Lock lock(frame_mutex);
        auto found = children.find(child_id);
        if (found != children.end())
        {
            auto& child{found->second};
            child.last_frame = child_frame;
            auto child_delta = child_frame.msc - child.baseline.msc;
            auto virtual_delta = last_multi_frame.msc - baseline.msc;
            if (child_delta > virtual_delta)
            {
                last_multi_frame.msc = baseline.msc + child_delta;
                last_multi_frame.ust = child_frame.ust;
                cb = frame_callback;
                cb_frame = last_multi_frame;
            }
        }
    }

    if (cb)
        cb(cb_frame);
}

}} // namespace mir::graphics
