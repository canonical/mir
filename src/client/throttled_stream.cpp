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

void ThrottledStream::adopted_by(MirSurface*)
{
    // TODO
}

void ThrottledStream::unadopted_by(MirSurface*)
{
    // TODO
}

void ThrottledStream::wait_for_vsync()
{
    // TODO
}

void ThrottledStream::swap_buffers_sync()
{
    swap_buffers([](){})->wait_for_all();
    for (int i = 0; i < interval; ++i)
        wait_for_vsync();
}
