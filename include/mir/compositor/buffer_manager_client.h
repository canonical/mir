/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_MANAGER_CLIENT_H_
#define MIR_COMPOSITOR_BUFFER_MANAGER_CLIENT_H_

#include "buffer_texture_binder.h"
#include "mir/compositor/buffer.h"

#include <vector>
#include <atomic>
#include <mutex>
#include <memory>

namespace mir
{
namespace compositor
{

class BufferManagerClient : public BufferTextureBinder
{
public:
    BufferManagerClient();
    ~BufferManagerClient();
    void add_buffer(std::shared_ptr<Buffer> buffer);
    int remove_all_buffers();

    void bind_back_buffer();
    void dequeue_client_buffer();
private:
    std::vector<std::shared_ptr<Buffer>> buffer_list;
    std::mutex buffer_list_guard;

    std::atomic<std::shared_ptr<Buffer>> compositor_buffer;
    std::atomic<std::shared_ptr<Buffer>> client_buffer;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_MANAGER_CLIENT_H_ */
