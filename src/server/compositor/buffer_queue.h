/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_BUFFER_QUEUE_H_
#define MIR_BUFFER_QUEUE_H_

#include "buffer_bundle.h"

#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>

namespace mir
{
namespace graphics
{
class Buffer;
class GraphicBufferAllocator;
}
namespace compositor
{

class BufferQueue : public BufferBundle
{
public:
    typedef std::function<void(graphics::Buffer* buffer)> Callback;

    BufferQueue(int nbuffers,
                std::shared_ptr<graphics::GraphicBufferAllocator> const& alloc,
                graphics::BufferProperties const& props);

    void client_acquire(Callback complete) override;
    void client_release(graphics::Buffer* buffer) override;
    std::shared_ptr<graphics::Buffer> compositor_acquire(void const* user_id) override;
    void compositor_release(std::shared_ptr<graphics::Buffer> const& buffer) override;
    std::shared_ptr<graphics::Buffer> snapshot_acquire() override;
    void snapshot_release(std::shared_ptr<graphics::Buffer> const& buffer) override;

    graphics::BufferProperties properties() const override;
    void allow_framedropping(bool dropping_allowed) override;
    void force_requests_to_complete() override;
    void resize(const geometry::Size &newsize) override;
    int buffers_ready_for_compositor() const override;
    bool framedropping_allowed() const;

private:
    void give_buffer_to_client(graphics::Buffer* buffer,
        std::unique_lock<std::mutex> lock);
    bool should_reuse_current_buffer(void const* user_id);
    void release(graphics::Buffer* buffer,
        std::unique_lock<std::mutex> lock);

    mutable std::mutex guard;

    std::vector<std::shared_ptr<graphics::Buffer>> buffers;
    std::deque<graphics::Buffer*> ready_to_composite_queue;
    std::deque<graphics::Buffer*> buffers_owned_by_client;
    std::vector<graphics::Buffer*> free_buffers;
    std::vector<graphics::Buffer*> buffers_sent_to_compositor;
    std::vector<graphics::Buffer*> pending_snapshots;

    std::vector<void const*> current_buffer_users;
    graphics::Buffer* current_compositor_buffer;

    std::deque<Callback> pending_client_notifications;

    int nbuffers;
    bool frame_dropping_enabled;
    graphics::BufferProperties the_properties;

    std::condition_variable snapshot_released;
    std::shared_ptr<graphics::GraphicBufferAllocator> gralloc;
};

}
}

#endif
