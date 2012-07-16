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

#ifndef MIR_COMPOSITOR_BUFFER_SWAPPER_DOUBLE_H_
#define MIR_COMPOSITOR_BUFFER_SWAPPER_DOUBLE_H_

#include "buffer_swapper.h"

#include <memory>
#include <atomic>
#include <thread>

namespace mir
{
namespace compositor
{

class Buffer;

class BufferSwapperDouble : public BufferSwapper
{
public:
    BufferSwapperDouble(std::unique_ptr<Buffer> && buffer_a, std::unique_ptr<Buffer> && buffer_b);

    Buffer* dequeue_free_buffer();
    void queue_finished_buffer();
    Buffer* grab_last_posted();
    void ungrab();

private:
    void compositor_to_grabbed();
    void compositor_to_ungrabbed();
    void compositor_change_toggle_pattern();
    void client_to_dequeued();
    void client_to_queued();

    typedef std::unique_ptr<Buffer> BufferPtr;
    BufferPtr buffer_a;
    BufferPtr buffer_b;

    BufferPtr invalid0;
    BufferPtr invalid1;

    std::atomic<BufferPtr*> grabbed;
    std::atomic<BufferPtr*> dequeued;


    std::mutex test_mut;
    int test;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_SWAPPER_DOUBLE_H_ */
