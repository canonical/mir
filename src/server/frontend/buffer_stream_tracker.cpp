/*
 * Copyright © 2013-2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "buffer_stream_tracker.h"
#include "client_buffer_tracker.h"

#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_id.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mf = mir::frontend;
namespace mg = mir::graphics;

mf::BufferStreamTracker::BufferStreamTracker(size_t client_cache_size) :
    client_cache_size{client_cache_size}
{
}

bool mf::BufferStreamTracker::track_buffer(BufferStreamId buffer_stream_id, mg::Buffer* buffer)
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    auto& tracker = client_buffer_tracker[buffer_stream_id];

    if (!tracker)
        tracker = std::make_shared<ClientBufferTracker>(client_cache_size);

    for (auto it = client_buffer_tracker.begin(); it != client_buffer_tracker.end(); it++)
    {
        if (it->first == buffer_stream_id) continue;
        if (it->second->client_has(buffer->id()))
            BOOST_THROW_EXCEPTION(std::logic_error("buffer already associated with another surface"));
    }

    auto already_tracked = tracker->client_has(buffer->id());
    tracker->add(buffer);

    client_buffer_resource[buffer_stream_id] = buffer;

    return already_tracked;
}

void mf::BufferStreamTracker::remove_buffer_stream(BufferStreamId buffer_stream_id)
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    auto it = client_buffer_tracker.find(buffer_stream_id);

    if (it != client_buffer_tracker.end())
        client_buffer_tracker.erase(it);

    auto last_buffer_it = client_buffer_resource.find(buffer_stream_id);
    if (last_buffer_it != client_buffer_resource.end())
        client_buffer_resource.erase(last_buffer_it);
}

mg::Buffer* mf::BufferStreamTracker::last_buffer(BufferStreamId buffer_stream_id) const
{

    std::lock_guard<decltype(mutex)> lock{mutex};
    auto it = client_buffer_resource.find(buffer_stream_id);

    if (it != client_buffer_resource.end())
        return it->second;
    else
        //should really throw, but that is difficult with the way the code currently works
        return nullptr;
}

mg::Buffer* mf::BufferStreamTracker::buffer_from(mg::BufferID buffer_id) const
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    for (auto const& tracker : client_buffer_tracker)
    {
        auto buffer = tracker.second->buffer_from(buffer_id);
        if (buffer != nullptr)
            return buffer;
    }
    BOOST_THROW_EXCEPTION(std::logic_error("Buffer is not tracked"));
}
