/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_SWAPPER_MULTI_H_
#define MIR_COMPOSITOR_BUFFER_SWAPPER_MULTI_H_

#include "buffer_swapper.h"

#include <mutex>
#include <condition_variable>
#include <vector>
#include <memory>
#include <deque>
#include <map>

namespace mir
{
namespace compositor
{

class Buffer;
class BufferIDUniqueGenerator;

class BufferSwapperMulti : public BufferSwapper
{
public:
    explicit BufferSwapperMulti(std::initializer_list<std::shared_ptr<compositor::Buffer>> buffer_list);
    explicit BufferSwapperMulti(std::vector<std::shared_ptr<compositor::Buffer>> buffer_list);

    void client_acquire(std::shared_ptr<Buffer>& buffer_reference, BufferID& dequeued_buffer);
    void client_release(BufferID queued_buffer);
    void compositor_acquire(std::shared_ptr<Buffer>& buffer_reference, BufferID& acquired_buffer);
    void compositor_release(BufferID released_buffer);
    void shutdown();

private:
    template<class T>
    void initialize_queues(T);

    std::map<BufferID, std::shared_ptr<Buffer>> buffers;

    std::mutex swapper_mutex;

    std::condition_variable client_available_cv;
    std::deque<BufferID> client_queue;
    std::deque<BufferID> compositor_queue;
    unsigned int in_use_by_client;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_SWAPPER_MULTI_H_ */
