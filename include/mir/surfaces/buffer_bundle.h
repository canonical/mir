/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_SURFACES_BUFFER_BUNDLE_H_
#define MIR_SURFACES_BUFFER_BUNDLE_H_

#include "mir/geometry/size.h"
#include "mir/geometry/pixel_format.h"
#include "mir/compositor/buffer_id.h"

#include <memory>

namespace mir
{
namespace compositor
{
class Buffer;
}

namespace surfaces
{
class GraphicRegion;

class BufferBundle
{
public:
    virtual std::shared_ptr<compositor::Buffer> secure_client_buffer() = 0;
    virtual std::shared_ptr<surfaces::GraphicRegion> lock_back_buffer() = 0;
    virtual geometry::PixelFormat get_bundle_pixel_format() = 0;
    virtual geometry::Size bundle_size() = 0;
    virtual void shutdown() = 0;
};

}
}

#endif /* MIR_SURFACES_BUFFER_BUNDLE_H_ */
