/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <algorithm>

#include "client_buffer_tracker.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/buffer.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;

mf::ClientBufferTracker::ClientBufferTracker(unsigned int client_cache_size)
    : buffers{},
      cache_size{client_cache_size}
{
}

void mf::ClientBufferTracker::add(mg::Buffer* buffer)
{
    auto existing_buffer = std::find(buffers.begin(), buffers.end(), buffer);

    if (existing_buffer != buffers.end())
    {
        buffers.push_front(*existing_buffer);
        buffers.erase(existing_buffer);
    }
    else
    {
        buffers.push_front(buffer);
    }
    if (buffers.size() > cache_size)
    {
        buffers.pop_back();
    }
}

mg::Buffer* mf::ClientBufferTracker::buffer_from(mg::BufferID const& buffer_id) const
{
    auto it = std::find_if(buffers.begin(), buffers.end(),
        [&buffer_id](mg::Buffer* buffer)
        {
            return (buffer->id() == buffer_id);
        });

    if (it == buffers.end())
        return nullptr;
    else
        return *it;
}

bool mf::ClientBufferTracker::client_has(mg::BufferID const& buffer_id) const
{
    return (nullptr != buffer_from(buffer_id));
}
