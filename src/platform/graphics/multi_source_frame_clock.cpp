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

#include "mir/graphics/multi_source_frame_clock.h"

namespace mir { namespace graphics {

void MultiSourceFrameClock::add_child_clock(std::weak_ptr<FrameClock> w)
{
    Lock lock(frame_mutex);
    if (auto clock = w.lock())
    {
        ChildId id = clock.get();
        children[id].clock = std::move(w);
        synchronize(lock);
        clock->set_frame_callback(
            std::bind(&MultiSourceFrameClock::on_child_frame,
                      this, id, std::placeholders::_1) );
    }
}

void MultiSourceFrameClock::synchronize(Lock const&)
{
    last_sync = last_multi_frame;

    auto c = children.begin();
    while (c != children.end())
    {
        auto& child = c->second;
        if (child.clock.expired())
        {
            /*
             * Lazy deferred clean-up. We don't need to do this any sooner
             * because a deleted child (which no longer generates callbacks)
             * doesn't affect results at all. This means we don't need to
             * ask graphics platforms to tell us when an output is removed.
             */
            c = children.erase(c);
        }
        else
        {
            child.last_sync = child.last_frame;
            ++c;
        }
    }
}

void MultiSourceFrameClock::on_child_frame(ChildId child_id,
                                           Frame const& child_frame)
{
    FrameCallback callback;
    Frame callback_arg;

    {
        Lock lock(frame_mutex);
        auto found = children.find(child_id);
        if (found != children.end())
        {
            auto& child = found->second;
            if (child.contributed_to_multi_frame.msc == last_multi_frame.msc)
            {
                /*
                 * This is the primary/fastest display.
                 * Note our last_multi_frame counters must remain monotonic,
                 * even if the primary display changes to a child display with
                 * lower counters...
                 */
                last_multi_frame.msc = last_sync.msc +
                                       child_frame.msc - child.last_sync.msc;
                last_multi_frame.ust = last_sync.ust +
                                       child_frame.ust - child.last_sync.ust;
                callback = frame_callback;
                callback_arg = last_multi_frame;
            }

            child.last_frame = child_frame;
            child.contributed_to_multi_frame = last_multi_frame;

            /*
             * So what happens when the primary display vanishes? A secondary
             * display catches up next frame, and one frame later that
             * secondary display now qualifies as the primary display (if
             * statement).
             */
        }
    }

    if (callback)
        callback(callback_arg);
}

}} // namespace mir::graphics
