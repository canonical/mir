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

class BufferBundle
{
public:
    virtual ~BufferBundle() noexcept {}
    virtual void client_acquire(std::function<void(graphics::Buffer* buffer)> complete) = 0;
    virtual void client_release(graphics::Buffer*) = 0;
    virtual std::shared_ptr<graphics::Buffer>
        compositor_acquire(unsigned long frameno) = 0;
    virtual void compositor_release(std::shared_ptr<graphics::Buffer> const&) = 0;
    virtual std::shared_ptr<graphics::Buffer> snapshot_acquire() = 0;
    virtual void snapshot_release(std::shared_ptr<graphics::Buffer> const&) = 0;

    virtual graphics::BufferProperties properties() const = 0;
    virtual void allow_framedropping(bool dropping_allowed) = 0;
    virtual void force_requests_to_complete() = 0;
    virtual void resize(const geometry::Size &newsize) = 0;
protected:
    BufferBundle() = default;
    BufferBundle(BufferBundle const&) = delete;
    BufferBundle& operator=(BufferBundle const&) = delete;
};
}
}

#endif /*MIR_COMPOSITOR_BUFFER_BUNDLE_H_*/
