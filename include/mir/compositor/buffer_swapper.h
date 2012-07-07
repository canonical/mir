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

#ifndef MIR_COMPOSITOR_BUFFER_SWAPPER_H_
#define MIR_COMPOSITOR_BUFFER_SWAPPER_H_

#include "buffer.h"

#include <atomic>
#include <memory>

namespace mir
{
namespace compositor
{
class Buffer;

class BufferSwapper {
public:
    virtual void dequeue_free_buffer(Buffer*& buffer) = 0;
    virtual void queue_finished_buffer(Buffer* buffer) = 0;
    virtual void grab_last_posted(Buffer*& buffer) = 0;
    virtual void ungrab(Buffer* buffer ) = 0;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_SWAPPER_H_ */
