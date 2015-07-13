/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef MIR_COMPOSITOR_BUFFER_MAP_H_
#define MIR_COMPOSITOR_BUFFER_MAP_H_

#include "mir/frontend/buffer_stream_id.h"
#include "mir/frontend/client_buffers.h"
#include <mutex>
#include <map>

namespace mir
{
namespace graphics { class GraphicBufferAllocator; }
namespace frontend { class EventSink; }
namespace compositor
{
class BufferMap : public frontend::ClientBuffers
{
public:
    BufferMap(
        frontend::BufferStreamId id,
        std::shared_ptr<frontend::EventSink> const& sink,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator);

    void add_buffer(graphics::BufferProperties const& properties) override;
    void remove_buffer(graphics::BufferID id) override;
    void send_buffer(graphics::BufferID id) override;
    std::shared_ptr<graphics::Buffer>& operator[](graphics::BufferID) override;
    
private:
    std::mutex mutable mutex;
    typedef std::map<graphics::BufferID, std::shared_ptr<graphics::Buffer>> Map;
    //used to keep strong reference
    Map buffers;
    Map::iterator checked_buffers_find(graphics::BufferID, std::unique_lock<std::mutex> const&);

    frontend::BufferStreamId const stream_id;
    std::shared_ptr<frontend::EventSink> const sink;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
};
}
}
#endif /* MIR_COMPOSITOR_BUFFER_MAP_H_ */
