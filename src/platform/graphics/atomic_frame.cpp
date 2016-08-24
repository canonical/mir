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
void log(mir::graphics::Frame const& f)
{
    // long long to match printf format
    long long msc = f.msc,
              ust = f.ust.microseconds,
              interval = f.min_ust_interval,
              age = f.age();
    mir::log_debug("Frame #%lld at %lld.%06llds (%lldus ago) interval %lldus",
                   msc, ust/1000000ULL, ust%1000000ULL, age, interval);
}
} // mir::graphics::anonymous namespace

Frame AtomicFrame::load() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return frame;
}

void AtomicFrame::store(Frame const& f)
{
    if (1)  // For manual testing of new graphics drivers only
        log(f);
    std::lock_guard<decltype(mutex)> lock(mutex);
    frame = f;
}

}} // namespace mir::graphics
