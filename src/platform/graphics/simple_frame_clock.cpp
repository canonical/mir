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

#include "mir/graphics/simple_frame_clock.h"

namespace mir { namespace graphics {

void SimpleFrameClock::set_frame_callback(FrameCallback const& cb)
{
    std::lock_guard<Mutex> lock(mutex);
    callback = cb;
}

void SimpleFrameClock::notify_frame(Frame const& frame)
{
    std::unique_lock<Mutex> lock(mutex);
    if (callback)
    {
        auto cb = callback;
        lock.unlock();
        cb(frame);
    }
}

}} // namespace mir::graphics
