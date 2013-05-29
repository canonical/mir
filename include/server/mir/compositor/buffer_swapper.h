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
      currently usable buffer. Depending on the swapper, this call may potentially wait
      for a buffer to become available */
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
     * Forces client requests on the buffer swapper to complete.
     *
     * client_acquire is the only function that can block to provide sync.
     * This function unblocks client_acquire, generally resulting in an exception
     * in threads with a waiting client_acquire()
     *
     * After this request, the compositor can keep acquiring and releasing buffers
     * but the client cannot. This used in shutdown of the swapper, the client cannot
     * be reactivated after this call completes.
     */
    virtual void force_client_completion() = 0;

    /**
     * Transfers all buffers within the swapper to another BufferSwapper. after
     * the completion of this call, neither the client, nor the compositor can
     * continue to request or return buffers to this swapper.
     */
    virtual void transfer_buffers_to(std::shared_ptr<BufferSwapper> const&) = 0;

    /**
     * Adopt a new buffer into the swapper.
     */
    virtual void adopt_buffer(std::shared_ptr<compositor::Buffer> const&) = 0;

    /**
     * If the swapper has been used, and you want to preserve the buffers that have been used, 
     * it is advisable to shutdown the BufferSwapper  by using force_client_completion()
     * and then transfer_buffers_to(). If these are not called, all buffers within the swapper
     * will be deallocated
     */
    virtual ~BufferSwapper() {}

protected:
    BufferSwapper() = default;
    BufferSwapper(BufferSwapper const&) = delete;
    BufferSwapper& operator=(BufferSwapper const&) = delete;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_SWAPPER_H_ */
