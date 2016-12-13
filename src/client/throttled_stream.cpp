/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "./throttled_stream.h"
#include "mir_wait_handle.h"

using mir::client::ThrottledStream;

ThrottledStream::ThrottledStream()
    : interval(1)
{
}

ThrottledStream::~ThrottledStream()
{
}

int ThrottledStream::swap_interval() const
{
    // TODO locking?
    return interval;
}

MirWaitHandle* ThrottledStream::set_swap_interval(int i)
{
    // TODO locking?
    interval = i;
    return nullptr;
}

void ThrottledStream::adopted_by(MirSurface* surface)
{
    // TODO locking?
    users.insert(surface);
    if (!frame_clock)
        frame_clock = surface->get_frame_clock();
}

void ThrottledStream::unadopted_by(MirSurface* surface)
{
    // TODO locking?
    users.erase(surface);
    if (frame_clock == surface->get_frame_clock())
    {
        frame_clock = users.empty() ? nullptr
                                    : (*users.begin())->get_frame_clock();
    }
}

void ThrottledStream::wait_for_vsync()
{
    if (!frame_clock)
        return;

    mir::time::PosixTimestamp target;
    {
        //std::lock_guard<decltype(mutex)> lock(mutex);
        target = frame_clock->next_frame_after(last_vsync);
    }
    sleep_until(target);
    {
        //std::lock_guard<decltype(mutex)> lock(mutex);
        /*
         * Note we record the target wakeup time and not the actual wakeup time
         * so that the frame interval remains perfectly spaced even in the
         * face of scheduling irregularities.
         */
        if (target > last_vsync)
            last_vsync = target;
    }
}

void ThrottledStream::swap_buffers_sync()
{
    swap_buffers([](){})->wait_for_all();
    for (int i = 0; i < interval; ++i)
        wait_for_vsync();
}
