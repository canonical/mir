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

#include "mir/graphics/atomic_frame.h"
#include "mir/log.h"

namespace mir { namespace graphics {

namespace {
void log(Frame const& frame)
{
    // Enable this only during development
    if (0) return;
    // long long to match printf format
    long long msc = frame.msc,
              ust = frame.ust.microseconds,
              interval = 0, // TODO replace
              age = frame.age();
    mir::log_debug(
        "Frame counter %p: #%lld at %lld.%06llds (%lld\xce\xbcs ago) interval %lld\xce\xbcs",
        (void*)&frame, msc, ust/1000000, ust%1000000, age, interval);
}
} // namesapce mir::graphics::<anonymous>

Frame AtomicFrame::load() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return frame;
}

void AtomicFrame::store(Frame const& f)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    frame = f;
    log(frame);
}

void AtomicFrame::increment_now()
{
    increment_with_timestamp(Timestamp::now(CLOCK_MONOTONIC));
}

void AtomicFrame::increment_with_timestamp(Timestamp t)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    frame.ust = t;
    frame.msc++;
    log(frame);
}

}} // namespace mir::graphics
