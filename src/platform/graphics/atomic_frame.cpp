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

Frame AtomicFrame::load() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return frame;
}

void AtomicFrame::store(Frame const& f)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    frame = f;
    report_new_frame(lock);
}

void AtomicFrame::increment_now()
{
    increment_with_timestamp(Frame::Timestamp::now(frame.ust.clock_id));
}

void AtomicFrame::increment_with_timestamp(Frame::Timestamp t)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    frame.ust = t;
    frame.msc++;
    report_new_frame(lock);
}

void AtomicFrame::report_new_frame(std::lock_guard<std::mutex> const&)
{
    // In future someone may choose to make this call a proper report but
    // for the time being we only need a little logging during development...

    // Enable this only during development
    if (0) return;

    // long long to match printf format
    long long msc = frame.msc,
              ust_ns = frame.ust.nanoseconds,
              interval_ns = frame.ust.nanoseconds - prev_ust.nanoseconds,
              now_ns = Frame::Timestamp::now(frame.ust.clock_id).nanoseconds,
              age_ns = now_ns - ust_ns;
    static const char usec_utf8[] = "\xce\xbcs";
    mir::log_debug(
        "AtomicFrame %p: #%lld at %lld.%06llds (%lld%s ago) interval %lld%s",
        (void*)this,
        msc, ust_ns/1000000000, (ust_ns%1000000000)/1000,
        age_ns/1000, usec_utf8,
        interval_ns/1000, usec_utf8);

    prev_ust = frame.ust;
}

}} // namespace mir::graphics
