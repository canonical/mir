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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_PLATFORM_GRAPHIC_ANDROID_ANDROID_ALLOC_ADAPTOR_H_
#define MIR_PLATFORM_GRAPHIC_ANDROID_ANDROID_ALLOC_ADAPTOR_H_

#include <hardware/gralloc.h>
#include "mir_platform/graphic_alloc_adaptor.h"
#include <memory>

namespace mir
{
namespace graphics
{

class AndroidBufferHandle: public BufferData
{
public:
    AndroidBufferHandle(buffer_handle_t han)
     :handle(han)
    {}
    buffer_handle_t handle;
};

class AndroidAllocAdaptor : public GraphicAllocAdaptor 
{
public:
    AndroidAllocAdaptor(std::shared_ptr<struct alloc_device_t> alloc_device);
    bool alloc_buffer(std::shared_ptr<BufferData>&, geometry::Stride&,
                      geometry::Width, geometry::Height,
                      compositor::PixelFormat, BufferUsage usage);
    bool inspect_buffer(char *buf, int buf_len);

private:
    std::shared_ptr<struct alloc_device_t> alloc_dev;
    int convert_to_android_format(compositor::PixelFormat pf);
    int convert_to_android_usage(BufferUsage usage);

};

}
}

#endif /* MIR_PLATFORM_GRAPHIC_ANDROID_ANDROID_ALLOC_ADAPTOR_H_ */
