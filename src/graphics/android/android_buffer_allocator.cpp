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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/graphics/android/android_buffer_allocator.h"
#include "mir/graphics/android/android_alloc_adaptor.h"
#include "mir/graphics/android/android_buffer.h"

#include <stdexcept>

namespace mg  = mir::graphics;
namespace mga = mir::graphics::android;
namespace mc  = mir::compositor;
namespace geom = mir::geometry;


struct AllocDevDeleter
{
    void operator()(alloc_device_t* t)
    {
        /* android takes care of delete for us */
        t->common.close((hw_device_t*)t);
    }
};

mga::AndroidBufferAllocator::AndroidBufferAllocator()
{
    int err;

    err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module);
    if (err < 0)
        throw std::runtime_error("Could not open hardware module");

    struct alloc_device_t* alloc_dev;
    err = hw_module->methods->open(hw_module, GRALLOC_HARDWARE_GPU0, (struct hw_device_t**) &alloc_dev);
    if (err < 0)
        throw std::runtime_error("Could not open hardware module");

    /* note for future use: at this point, the hardware module should be filled with vendor information
       that we can determine different courses of action based upon */

    AllocDevDeleter del;
    std::shared_ptr<struct alloc_device_t> alloc_dev_ptr(alloc_dev, del);
    alloc_device = std::shared_ptr<mga::GraphicAllocAdaptor>(new AndroidAllocAdaptor(alloc_dev_ptr));
}

std::unique_ptr<mc::Buffer> mga::AndroidBufferAllocator::alloc_buffer(
    geom::Width width, geom::Height height, mc::PixelFormat pf)
{
    auto handle = new AndroidBuffer(alloc_device, width, height, pf);
#if 0
    auto tmp =  handle->native_window_buffer_handle->get_egl_client_buffer();
    ANativeWindowBuffer* tmp2 = (ANativeWindowBuffer*) tmp;
    const native_handle_t* nathand = tmp2->handle;

    const gralloc_module_t *module = (const gralloc_module_t*) hw_module;
    int* addr;


    module->lock(module, nathand, 0x030, 0, 0, 200, 400, (void **) &addr);
    for(int i=0; i < 200*400; i++)
        addr[i] = 0xFFFFFFFF;
    module->unlock(module, nathand);
    printf("0x%X\n", (int)addr);
#endif
    return std::unique_ptr<mc::Buffer>(handle); 
}

std::shared_ptr<mc::GraphicBufferAllocator> mg::create_buffer_allocator()
{
    return std::make_shared<mga::AndroidBufferAllocator>();
}
