/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
}

void AtomicFrame::increment_now()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    frame.ust = Frame::Timestamp::now(frame.ust.clock_id);
    frame.msc++;
}

void AtomicFrame::increment_with_timestamp(Frame::Timestamp t)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    frame.ust = t;
    frame.msc++;
}

}} // namespace mir::graphics
