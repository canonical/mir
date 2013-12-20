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

#ifndef MIR_GRAPHICS_ANDROID_GRAPHIC_ALLOC_ADAPTOR_H_
#define MIR_GRAPHICS_ANDROID_GRAPHIC_ALLOC_ADAPTOR_H_

#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"
#include "buffer_usage.h"
#include <memory>

namespace mir
{
namespace graphics
{

class NativeBuffer;
namespace android
{

class GraphicAllocAdaptor
{
public:
    virtual std::shared_ptr<NativeBuffer> alloc_buffer(geometry::Size size,
        MirPixelFormat, BufferUsage usage) = 0;
protected:
    GraphicAllocAdaptor() = default;
    virtual ~GraphicAllocAdaptor() {}
    GraphicAllocAdaptor(const GraphicAllocAdaptor&) = delete;
    GraphicAllocAdaptor& operator=(const GraphicAllocAdaptor&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_GRAPHIC_ALLOC_ADAPTOR_H_ */
