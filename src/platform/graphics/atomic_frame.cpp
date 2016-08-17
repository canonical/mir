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
    unsigned long long frame_seq = f.msc;
    unsigned long long frame_usec = f.ust;
    struct timespec now;
    clock_gettime(f.clock_id, &now);
    unsigned long long now_usec = now.tv_sec*1000000ULL +
                                  now.tv_nsec/1000;
    long long age_usec = now_usec - frame_usec;
    fprintf(stderr, "Frame #%llu at %llu.%06llus (%lldus ago) delta %lldus\n",
                    frame_seq,
                    frame_usec/1000000ULL, frame_usec%1000000ULL,
                    age_usec, frame_usec - frame.ust);
#endif
    frame = f;
}

}}
