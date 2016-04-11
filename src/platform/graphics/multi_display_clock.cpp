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

using namespace mir::graphics;

Frame MultiDisplayClock::last_frame() const
{
    return last_virtual_frame;
}

void MultiDisplayClock::on_next_frame(FrameCallback const& cb)
{
    callback = cb;
}

void MultiDisplayClock::hook_child_clock(DisplayClock& child_clock)
{
    child_clock.on_next_frame( std::bind(&MultiDisplayClock::on_child_frame,
                                         this, children.size(),
                                         std::placeholders::_1) );
}

void MultiDisplayClock::add_child_clock(std::weak_ptr<DisplayClock> w)
{
    if (auto dc = w.lock())
    {
        hook_child_clock(*dc);
        children.emplace_back(Child{std::move(w), {}});
        synchronize();
    }
}

void MultiDisplayClock::synchronize()
{
    baseline = last_frame();
    for (auto& child : children)
    {
        if (auto child_clock = child.clock.lock())
            child.baseline = child_clock->last_frame();
    }
}

void MultiDisplayClock::on_child_frame(int child_index, Frame const& child_frame)
{
    auto& child = children.at(child_index);
    auto child_delta = child_frame.msc - child.baseline.msc;
    auto virtual_delta = last_virtual_frame.msc - baseline.msc;
    if (child_delta > virtual_delta)
    {
        last_virtual_frame.msc = baseline.msc + child_delta;
        last_virtual_frame.ust = child_frame.ust;
        callback(last_virtual_frame);
    }
    if (auto child_clock = child.clock.lock())
        hook_child_clock(*child_clock);
}

