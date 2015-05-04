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

#ifndef MIR_FRONTEND_BUFFER_STREAM_TRACKER_H_
#define MIR_FRONTEND_BUFFER_STREAM_TRACKER_H_

#include "mir/frontend/buffer_stream_id.h"
#include "mir/graphics/buffer_id.h"

#include <unordered_map>
#include <memory>
#include <mutex>

namespace mir
{
namespace graphics
{
class Buffer;
}
namespace frontend
{
class ClientBufferTracker;
class BufferStreamTracker
{
public:
    BufferStreamTracker(size_t client_cache_size);
    BufferStreamTracker(BufferStreamTracker const&) = delete;
    BufferStreamTracker& operator=(BufferStreamTracker const&) = delete;

    /* track a buffer as associated with a buffer stream
     * \warning the buffer must correspond to a single buffer stream id
     * \param buffer_stream_id id that the the buffer is associated with
     * \param buffer     buffer to be tracked
     * \returns          true if the buffer is already tracked
     *                   false if the buffer is not tracked
     */
    bool track_buffer(BufferStreamId buffer_stream_id, graphics::Buffer* buffer);
    /* removes the buffer stream id from all tracking */
    void remove_buffer_stream(BufferStreamId);

    /* Access the buffer resource that the id corresponds to. */
    graphics::Buffer* buffer_from(graphics::BufferID) const;

private:
    size_t const client_cache_size;
    std::unordered_map<BufferStreamId, std::shared_ptr<ClientBufferTracker>> client_buffer_tracker;

public:
    /* accesses the last buffer given to track_buffer() for the given BufferStreamId */
    //TODO: once next_buffer rpc call is fully deprecated, this can be removed.
    graphics::Buffer* last_buffer(BufferStreamId) const;
private:
    mutable std::mutex mutex;
    std::unordered_map<BufferStreamId, graphics::Buffer*> client_buffer_resource;
};

}
}

#endif // MIR_FRONTEND_BUFFER_STREAM_TRACKER_H_
