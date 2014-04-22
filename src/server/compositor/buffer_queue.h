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
    static const int min_num_buffers = 2;

    BufferQueue(int nbuffers,
                std::shared_ptr<graphics::GraphicBufferAllocator> const&,
                graphics::BufferProperties const&);

    void client_acquire(std::function<void(graphics::Buffer* buffer)> complete) override;
    void client_release(graphics::Buffer*) override;
    std::shared_ptr<graphics::Buffer> compositor_acquire(void const* user_id) override;
    void compositor_release(std::shared_ptr<graphics::Buffer> const&) override;
    std::shared_ptr<graphics::Buffer> snapshot_acquire() override;
    void snapshot_release(std::shared_ptr<graphics::Buffer> const&) override;

    graphics::BufferProperties properties() const override;
    void allow_framedropping(bool dropping_allowed) override;
    void force_requests_to_complete() override;
    void resize(const geometry::Size &newsize) override;
    int buffers_ready_for_compositor() const override;
    bool framedropping_allowed() const;

private:
    void give_buffer_to_client(std::shared_ptr<graphics::Buffer> const& buffer,
        std::unique_lock<std::mutex> lock);
    bool is_new_front_buffer_user(void const* user_id);

    mutable std::mutex guard;

    std::deque<std::shared_ptr<graphics::Buffer>> free_queue;
    std::deque<std::shared_ptr<graphics::Buffer>> ready_to_composite_queue;
    std::deque<std::shared_ptr<graphics::Buffer>> buffers_owned_by_client;
    std::vector<std::shared_ptr<graphics::Buffer>> buffers_sent_to_compositor;
    std::vector<std::shared_ptr<graphics::Buffer>> pending_snapshots;

    std::vector<void const*> front_buffer_users;
    std::shared_ptr<graphics::Buffer> front_buffer;
    std::vector<std::shared_ptr<graphics::Buffer>> buffers_owned_by_compositor;

    std::function<void(graphics::Buffer* buffer)> give_to_client;

    int nbuffers;
    int allocated_buffers;
    bool frame_dropping_enabled;
    graphics::BufferProperties the_properties;

    std::condition_variable snapshot_released;
    std::shared_ptr<graphics::GraphicBufferAllocator> gralloc;
};

}
}

#endif
