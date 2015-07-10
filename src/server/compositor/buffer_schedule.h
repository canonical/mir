/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#ifndef MIR_BUFFER_SCHEDULE_H_
#define MIR_BUFFER_SCHEDULE_H_

#include "mir/frontend/buffer_stream_id.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/compositor/compositor_id.h"
#include <memory>
#include <map>
#include <mutex>
#include <deque>

namespace mir
{
namespace graphics { class Buffer; class GraphicBufferAllocator; class BufferProperties; }
namespace frontend { class EventSink; }
namespace compositor
{
class BufferSchedule
{
public:
    BufferSchedule(
        frontend::BufferStreamId id,
        std::shared_ptr<frontend::EventSink> const& sink,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator);

    void add_buffer(graphics::BufferProperties const& properties);
    void remove_buffer(graphics::BufferID id);

    void schedule_buffer(graphics::BufferID id);

    std::shared_ptr<graphics::Buffer> compositor_acquire(compositor::CompositorID id);
    void compositor_release(std::shared_ptr<graphics::Buffer> const&);

private:
    std::mutex mutable mutex;
    frontend::BufferStreamId const stream_id;
    std::shared_ptr<frontend::EventSink> const sink;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;

    typedef std::map<graphics::BufferID, std::shared_ptr<graphics::Buffer>> BufferMap;
    //used to keep strong reference
    BufferMap buffers;
    BufferMap::iterator checked_buffers_find(graphics::BufferID, std::unique_lock<std::mutex> const&);

    struct ScheduleEntry
    {
        std::shared_ptr<graphics::Buffer> buffer;
        unsigned int use_count;
        bool was_consumed;
        bool dead;
    };
    std::deque<ScheduleEntry> schedule;
};

}
}

#endif
