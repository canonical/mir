/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_COMPOSITOR_BUFFER_ACQUISITION_H_
#define MIR_COMPOSITOR_BUFFER_ACQUISITION_H_

#include "mir/graphics/buffer_properties.h"
#include <memory>

namespace mir
{
namespace graphics { class Buffer; struct BufferProperties; }

namespace compositor
{

class BufferAcquisition
{
public:
    /**
     * Acquire the next buffer that's ready to display/composite.
     *
     * \note    The returned buffer is considered in-use until the its
     *          use-count reaches 0. In-use buffers will not be returned
     *          to the client, so for best performance it is important to
     *          release the returned buffer as soon as possible.
     *
     * \param [in] user_id A unique identifier of who is going to use the
     *                     buffer, to ensure that separate users representing
     *                     separate monitors who need the same frame will get
     *                     the same buffer. However consecutive calls for the
     *                     same user will get different buffers. To avoid
     *                     collisions, all callers should determine user_id
     *                     in the same way (e.g. always use "this" pointer).
     */
    virtual std::shared_ptr<graphics::Buffer>
        compositor_acquire(void const* user_id) = 0;

    /**
     * Acquire the most recently displayed buffer.
     *
     * In contrast with compositor_acquire() this does not consume a client
     * buffer.
     *
     * Like compositor_acquire(), you should release your reference to the
     * returned buffer as soon as possible.
     *
     * \return A shared reference to the most recent visible client buffer.
     */
    virtual std::shared_ptr<graphics::Buffer> snapshot_acquire() = 0;
    virtual ~BufferAcquisition() = default;

protected:
    BufferAcquisition() = default;
    BufferAcquisition(BufferAcquisition const&) = delete;
    BufferAcquisition& operator=(BufferAcquisition const&) = delete;
};

}
}

#endif /*MIR_COMPOSITOR_BUFFER_ACQUISITION_H_*/
