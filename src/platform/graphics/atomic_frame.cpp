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

namespace mir { namespace graphics {

Frame AtomicFrame::load() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return frame;
}

void AtomicFrame::store(Frame const& f)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
#if 1
    unsigned long long msc = f.msc;
    unsigned long long ust = f.ust;
    unsigned long long interval = f.min_ust_interval;
    long long age = f.age();
    fprintf(stderr, "Frame #%llu at %llu.%06llus (%lldus ago) interval %lluus\n",
                    msc,
                    ust/1000000ULL, ust%1000000ULL,
                    age, interval);
#endif
    frame = f;
}

}}
