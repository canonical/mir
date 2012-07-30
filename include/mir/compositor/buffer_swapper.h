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

#include <memory>

namespace mir
{
namespace compositor
{
class Buffer;

class BufferSwapper
{
public:
    virtual ~BufferSwapper() {}

    /* callers of client_acquire are returned a pointer to the buffer that is usable by clients
     * This call may potentially wait for a buffer to become available.
     * The BufferSwapper class releases its references to the returned buffer
    */ 
    virtual std::shared_ptr<Buffer> client_acquire() = 0;

    /* once a client is done with using a buffer, it must release the buffer back to the swapper 
     * class. The returned buffer becomes the next buffer the compositor will acquire.
     * BufferSwapper class retains ownership of the returned buffer until the BufferSwapper class
     * returns the buffer to its users via the *_aquire() functions 
     * This call may wait 
    */
    virtual void client_release(std::shared_ptr<Buffer> queued_buffer) = 0;

    /* caller of compositor_acquire buffer acquires ownership of the last buffer 
     * the client posted via client_release()
     * caller of this function guarantees that they will call compositor_release() in the future.
     * The client will potentially stall from the time between compositor_aquire and compositor_release
     * The BufferSwapper class releases its references to the returned buffer
     * This call's wait time is guaranteed to be minimal even in worse-case scenarios
    */ 
    virtual std::shared_ptr<Buffer> compositor_acquire() = 0;

    /* once the compositor is done with the finished buffer, it must queue it
     * This call's wait time is guaranteed to be minimal even in worse-case scenarios
    */ 
    virtual void compositor_release(std::shared_ptr<Buffer> released_buffer) = 0;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_SWAPPER_H_ */
