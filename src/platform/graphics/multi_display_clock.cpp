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

Frame MultiDisplayClock::last_frame() const
{
    Lock lock(mutex);
    return last_multi_frame;
}

void MultiDisplayClock::set_frame_callback(FrameCallback const& cb)
{
    Lock lock(mutex);
    callback = cb;
}

void MultiDisplayClock::hook_child_clock(Lock const&,
                                         DisplayClock& child_clock, int idx)
{
    child_clock.set_frame_callback( std::bind(&MultiDisplayClock::on_child_frame,
                                         this, idx, std::placeholders::_1) );
}

void MultiDisplayClock::add_child_clock(std::weak_ptr<DisplayClock> w)
{
    Lock lock(mutex);
    if (auto dc = w.lock())
    {
        children.emplace_back(Child{std::move(w), {}});
        synchronize(lock);
        hook_child_clock(lock, *dc, children.size()-1);
    }
}

void MultiDisplayClock::synchronize(Lock const&)
{
    baseline = last_multi_frame;
    for (auto& child : children)
    {
        if (auto child_clock = child.clock.lock())
            child.baseline = child_clock->last_frame();
    }
}

void MultiDisplayClock::on_child_frame(int child_index, Frame const& child_frame)
{
    FrameCallback cb;
    Frame cb_frame;

    {
        Lock lock(mutex);
        auto& child = children.at(child_index);
        auto child_delta = child_frame.msc - child.baseline.msc;
        auto virtual_delta = last_multi_frame.msc - baseline.msc;
        if (child_delta > virtual_delta)
        {
            last_multi_frame.msc = baseline.msc + child_delta;
            last_multi_frame.ust = child_frame.ust;
            cb = callback;
            cb_frame = last_multi_frame;
        }
        if (auto child_clock = child.clock.lock())
            hook_child_clock(lock, *child_clock, child_index);
    }

    if (cb)
        cb(cb_frame);
}

}} // namespace mir::graphics
