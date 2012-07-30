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

namespace mir
{
namespace compositor
{
class Buffer;

class BufferSwapper
{
public:
    virtual ~BufferSwapper() {}

    /* callers of client_acquire are returned a pointer to the
      currently usable buffer. This call may potentially wait for a
      buffer to become available */
    /* note: (kdub) we could probably come up with a richer type for the
                    BufferSwapper interface than a Buffer* as the return
                    for client_acquire and compositor_acquire */
    // TODO (alan_g) Agree with kdub - the returned object should likely
    //               be responsible for "release", not the client code?
    virtual Buffer* client_acquire() = 0;

    /* once a client is done with the finished buffer, it must queue
       it. This modifies the buffer the compositor posts to the screen */
    virtual void client_release(Buffer* queued_buffer) = 0;

    /* caller of compositor_acquire buffer should get no-wait access to the
        last posted buffer. However, the client will potentially stall
        until control of the buffer is returned via compositor_release() */
    // TODO (alan_g) the returned object should likely be responsible
    //               for release, not the client code?
    virtual Buffer* compositor_acquire() = 0;

    virtual void compositor_release(Buffer* released_buffer) = 0;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_SWAPPER_H_ */
