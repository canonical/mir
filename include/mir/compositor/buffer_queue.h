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

#ifndef MIR_COMPOSITOR_BUFFER_QUEUE_H_
#define MIR_COMPOSITOR_BUFFER_QUEUE_H_

#include "buffer.h"
#include <memory>

namespace mir
{
namespace compositor
{

class BufferQueue
{
public:
    virtual std::shared_ptr<Buffer> dequeue_client_buffer() = 0;
    virtual void queue_client_buffer() = 0;

};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_QUEUE_H_ */
