/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "screencast_buffer_tracker.h"

namespace mf = mir::frontend;

bool mf::ScreencastBufferTracker::track_buffer(ScreencastSessionId id, graphics::Buffer* buffer)
{
    auto& set = tracker[id];
    auto pair = set.insert(buffer);
    bool const already_inserted = !pair.second;
    return already_inserted;
}

void mf::ScreencastBufferTracker::remove_session(ScreencastSessionId id)
{
    tracker.erase(id);
}

void mf::ScreencastBufferTracker::for_each_session(std::function<void(ScreencastSessionId)> f) const
{
    for (auto const& entry : tracker)
    {
        f(entry.first);
    }
}
