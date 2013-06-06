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

    /**
     * Forces requests on the buffer swapper to complete.
     *
     * Requests, like client_acquire(), can block waiting for a buffer to
     * become available. This method tries to force the request to
     * complete while ensuring that swapper keeps functioning properly.
     *
     * Note that it's not always possible to force a request to complete
     * without breaking the swapper. It's a logic error to attempt to call
     * this method if the circumstances don't allow a forced completion.
     *
     * To successfully use this method you should ensure that (i.e. the
     * preconditions for calling this method are):
     *
     * - The compositor is not holding any buffers (e.g., it has been stopped).
     * - The client is trying to acquire at most one buffer at a time (which is
     *   normal usage, and enforced by the high level API).
     */
    virtual void force_requests_to_complete() = 0;

    virtual ~BufferSwapper() = default;
protected:
    BufferSwapper() = default;
    BufferSwapper(BufferSwapper const&) = delete;
    BufferSwapper& operator=(BufferSwapper const&) = delete;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_SWAPPER_H_ */
