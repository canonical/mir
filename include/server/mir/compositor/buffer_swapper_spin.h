/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_SWAPPER_SPIN_H_
#define MIR_COMPOSITOR_BUFFER_SWAPPER_SPIN_H_

#include "buffer_swapper.h"

#include <mutex>
#include <vector>
#include <memory>
#include <deque>

#include <boost/throw_exception.hpp>

namespace mir
{
namespace compositor
{

class Buffer;

/**
 * Spinning buffer swapping variant
 *
 * BufferSwapperSpin provides a "spinning" swapping variant, in which the
 * client always has an available buffer to draw to, and can submit buffers
 * unrestricted by the vsync rate. When the compositor needs a buffer, it
 * uses the last submitted one, and the client continues to "spin" between
 * the remaining buffers.
 */
class BufferSwapperSpin : public BufferSwapper
{
public:
    template<typename BufferPtrContainer>
    BufferSwapperSpin(BufferPtrContainer const& buffer_list)
        : buffer_queue{buffer_list.begin(), buffer_list.end()},
          in_use_by_client{0},
          swapper_size{buffer_queue.size()}
    {
        if (swapper_size != 3)
        {
            BOOST_THROW_EXCEPTION(
                std::logic_error("BufferSwapperSpin is only validated for 3 buffers"));
        }
    }

    std::shared_ptr<Buffer> client_acquire();
    void client_release(std::shared_ptr<Buffer> const& queued_buffer);
    std::shared_ptr<Buffer> compositor_acquire();
    void compositor_release(std::shared_ptr<Buffer> const& released_buffer);

    void force_requests_to_complete();

private:
    template<class T>
    void initialize_queues(T);

    std::mutex swapper_mutex;

    std::deque<std::shared_ptr<Buffer>> buffer_queue;
    std::shared_ptr<Buffer> last_update_buffer;
    unsigned int in_use_by_client;
    bool client_submitted_new_buffer;
    size_t const swapper_size;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_SWAPPER_MULTI_SPIN_H_ */
