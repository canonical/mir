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

#include <atomic>
#include <condition_variable>

namespace mir
{
namespace compositor
{

class Buffer;

class BufferSwapperDouble : public BufferSwapper {
public:
    BufferSwapperDouble(Buffer* buffer_a, Buffer* buffer_b);

    void dequeue_free_buffer(Buffer*& buffer);
    void queue_finished_buffer(Buffer* buffer);
    void grab_last_posted(Buffer*& buffer);
    void ungrab(Buffer* buffer );

private:

    std::condition_variable_any no_dq_available;
    std::mutex cv_mutex;

    std::atomic<Buffer*> grabbed;
    std::atomic<Buffer*> dequeued;
    std::atomic<Buffer*> on_deck;
    std::atomic<Buffer*> last_posted;
    std::atomic<bool> new_last_posted;

};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_SWAPPER_H_ */
