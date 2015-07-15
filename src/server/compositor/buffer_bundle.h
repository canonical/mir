/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_COMPOSITOR_BUFFER_BUNDLE_H_
#define MIR_COMPOSITOR_BUFFER_BUNDLE_H_

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
    virtual void compositor_release(std::shared_ptr<graphics::Buffer> const&) = 0;
    virtual std::shared_ptr<graphics::Buffer> snapshot_acquire() = 0;
    virtual void snapshot_release(std::shared_ptr<graphics::Buffer> const&) = 0;
    virtual ~BufferAcquisition() = default;

protected:
    BufferAcquisition() = default;
    BufferAcquisition(BufferAcquisition const&) = delete;
    BufferAcquisition& operator=(BufferAcquisition const&) = delete;
};

class BufferBundle : public BufferAcquisition
{
public:
    virtual ~BufferBundle() noexcept {}
    virtual void client_acquire(std::function<void(graphics::Buffer* buffer)> complete) = 0;
    virtual void client_release(graphics::Buffer*) = 0;

    virtual graphics::BufferProperties properties() const = 0;
    virtual void allow_framedropping(bool dropping_allowed) = 0;
    virtual void force_requests_to_complete() = 0;
    virtual void resize(const geometry::Size &newsize) = 0;
    virtual int buffers_ready_for_compositor(void const* user_id) const = 0;

    /**
     * Return the number of client acquisitions that can be completed
     * synchronously without blocking, before a compositor consumes one. This
     * is used for pre-filling the queue in tests. Don't assume it's always
     * nbuffers-1 as it might be less.
     */
    virtual int buffers_free_for_client() const = 0;
    virtual void drop_old_buffers() = 0;
    virtual void drop_client_requests() = 0;

protected:
    BufferBundle() = default;
    BufferBundle(BufferBundle const&) = delete;
    BufferBundle& operator=(BufferBundle const&) = delete;
};
}
}

#endif /*MIR_COMPOSITOR_BUFFER_BUNDLE_H_*/
