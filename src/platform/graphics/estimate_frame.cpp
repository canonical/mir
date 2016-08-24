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

#include "mir/graphics/estimate_frame.h"

namespace mir { namespace graphics {

void EstimateFrame::increment_now()
{
    increment_with_timestamp(Timestamp::now(CLOCK_MONOTONIC));
}

void EstimateFrame::increment_with_timestamp(Timestamp t)
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        // Only update min_ust_interval after the first frame:
        if (frame.msc || frame.ust.microseconds || frame.min_ust_interval)
            frame.min_ust_interval = t - frame.ust;
        frame.ust = t;
        frame.msc++;
    }
    log();
}

}} // namespace mir::graphics
