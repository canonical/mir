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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_GRAPHICS_ANDROID_ANDROID_ALLOC_ADAPTOR_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_ALLOC_ADAPTOR_H_

#include "graphic_alloc_adaptor.h"

#include <hardware/gralloc.h>
#include <memory>


namespace mir
{
namespace graphics
{
namespace android
{
class DeviceQuirks;

class AndroidAllocAdaptor : public GraphicAllocAdaptor
{
public:
    explicit AndroidAllocAdaptor(std::shared_ptr<struct alloc_device_t> const& alloc_device,
                                 std::shared_ptr<DeviceQuirks> const& quirks);
    std::shared_ptr<NativeBuffer> alloc_buffer(geometry::Size,
            MirPixelFormat, BufferUsage usage);

    /* note: alloc_device_t has a third method (dump) that isn't needed by us. it can be used to check buffer contents */

private:
    std::shared_ptr<struct alloc_device_t> alloc_dev;
    std::shared_ptr<DeviceQuirks> const quirks;
    int convert_to_android_usage(BufferUsage usage);
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_ANDROID_ALLOC_ADAPTOR_H_ */
