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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_GRAPHIC_ALLOC_ADAPTOR_H_
#define MIR_GRAPHICS_ANDROID_GRAPHIC_ALLOC_ADAPTOR_H_

#include "android_buffer_handle.h"

#include <memory>

namespace mir
{
namespace graphics
{
namespace android
{

/* note: we will need a new concrete class implementing this interface whenever gralloc interface changes (or for hw specific quirks) */
class GraphicAllocAdaptor
{
public:
    virtual std::shared_ptr<AndroidBufferHandle> alloc_buffer(geometry::Size size,
        geometry::PixelFormat, BufferUsage usage) = 0;
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
