/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_SWAPPER_H_
#define MIR_COMPOSITOR_BUFFER_SWAPPER_H_

#include <memory>

namespace mir
{
namespace compositor
{
class Buffer;

class BufferSwapper
{
public:
    /* callers of client_acquire are returned a pointer to the
      currently usable buffer. This call may potentially wait for a
      buffer to become available */
    virtual std::shared_ptr<Buffer> client_acquire() = 0;

    /* once a client is done with the finished buffer, it must queue
       it. This modifies the buffer the compositor posts to the screen */
    virtual void client_release(std::shared_ptr<Buffer> const& queued_buffer) = 0;

    /* caller of compositor_acquire buffer should get no-wait access to the
        last posted buffer. However, the client will potentially stall
        until control of the buffer is returned via compositor_release() */
    virtual std::shared_ptr<Buffer> compositor_acquire() = 0;

    virtual void compositor_release(std::shared_ptr<Buffer> const& released_buffer) = 0;

    virtual void shutdown() = 0;

    virtual ~BufferSwapper() {}

protected:
    BufferSwapper() = default;
    BufferSwapper(BufferSwapper const&) = delete;
    BufferSwapper& operator=(BufferSwapper const&) = delete;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_SWAPPER_H_ */
