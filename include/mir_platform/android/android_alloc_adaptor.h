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

namespace mir
{
namespace graphics
{

class AndroidAllocAdaptor : public GraphicAllocAdaptor 
{
public:
    AndroidAllocAdaptor(const hw_module_t* module);
    int  alloc_buffer(geometry::Width, geometry::Height, compositor::PixelFormat, int usage, BufferData*, geometry::Stride*);
    int  free_buffer(BufferData handle);
    bool inspect_buffer(char *buf, int buf_len);

private:
    struct alloc_device_t* alloc_dev;
};

}
}

#endif /* MIR_PLATFORM_GRAPHIC_ANDROID_ANDROID_ALLOC_ADAPTOR_H_ */
