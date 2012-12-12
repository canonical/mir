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

#ifndef MIR_COMPOSITOR_BUFFER_SWAPPER_MULTI_H_
#define MIR_COMPOSITOR_BUFFER_SWAPPER_MULTI_H_

#include "buffer_swapper.h"

#include "mir/thread/all.h"

#include <memory>
#include <deque>
#include <vector>

namespace mir
{
namespace compositor
{

class Buffer;

class BufferSwapperMulti : public BufferSwapper
{
public:
    BufferSwapperMulti(std::unique_ptr<Buffer> buffer_a,
                       std::unique_ptr<Buffer> buffer_b);

    BufferSwapperMulti(std::unique_ptr<Buffer> buffer_a,
                       std::unique_ptr<Buffer> buffer_b,
                       std::unique_ptr<Buffer> buffer_c);

    Buffer* client_acquire();
    void client_release(Buffer* queued_buffer);
    Buffer* compositor_acquire();
    void compositor_release(Buffer* released_buffer);
    void shutdown();

private:
    std::vector<std::unique_ptr<Buffer>> buffers;

    std::mutex swapper_mutex;

    std::condition_variable client_available_cv;
    std::deque<Buffer*> client_queue;
    std::deque<Buffer*> compositor_queue;
    unsigned int in_use_by_client;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_SWAPPER_MULTI_H_ */
