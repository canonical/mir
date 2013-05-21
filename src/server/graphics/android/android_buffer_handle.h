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
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_BUFFER_HANDLE_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_BUFFER_HANDLE_H_

#include "mir/geometry/size.h"
#include "mir/geometry/pixel_format.h"

#include <memory>

struct ANativeWindowBuffer;

namespace mir
{
namespace compositor
{
class BufferIPCPackage;
}

namespace graphics
{
namespace android
{

class AndroidBufferHandle
{
public:
    virtual ~AndroidBufferHandle() {}

    virtual geometry::Size size() const   = 0;
    virtual geometry::Stride stride() const = 0;
    virtual geometry::PixelFormat format() const  = 0;
    virtual BufferUsage usage() const = 0;

    virtual std::shared_ptr<ANativeWindowBuffer> native_buffer_handle() const = 0;

protected:
    AndroidBufferHandle() = default;
};

}
}
}

#endif /*MIR_GRAPHICS_ANDROID_ANDROID_BUFFER_HANDLE_H_ */
