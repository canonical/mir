/*
 * Copyright © 2012, 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "testdraw/graphics_region_factory.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir/graphics/android/native_buffer.h"
#include "mir_toolkit/common.h"

#include <hardware/gralloc.h>
#include <stdexcept>

namespace mtd=mir::test::draw;

namespace
{
struct RegionDeleter
{
    RegionDeleter(gralloc_module_t* grmod, native_handle_t const* handle)
     : grmod(grmod),
       handle(handle)
    {
    }

    void operator()(MirGraphicsRegion* region)
    {
        grmod->unlock(grmod, handle);
        delete region;
    }

    gralloc_module_t *grmod;
    native_handle_t const* handle;
};

class AndroidGraphicsRegionFactory : public mir::test::draw::GraphicsRegionFactory
{
public:
    AndroidGraphicsRegionFactory()
    {
        const hw_module_t *hw_module;
        if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module) != 0)
            throw std::runtime_error("error, hw module not available!\n");
        gralloc_open(hw_module, &alloc_dev);
        module = (gralloc_module_t*) hw_module;
    }

    ~AndroidGraphicsRegionFactory()
    {
        gralloc_close(alloc_dev);
    }

    std::shared_ptr<MirGraphicsRegion> graphic_region_from_handle(
        mir::graphics::NativeBuffer const& native_buffer)
    {
        auto anwb = native_buffer.anwb();
        int *vaddr;
        int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
        module->lock(
            module,
            anwb->handle, usage,
            0, 0, anwb->width, anwb->height,
            (void**) &vaddr);

        MirGraphicsRegion* region = new MirGraphicsRegion;
        RegionDeleter del(module, anwb->handle);

        region->vaddr = (char*) vaddr;
        region->stride = anwb->stride * MIR_BYTES_PER_PIXEL(mir_pixel_format_abgr_8888);
        region->width = anwb->width;
        region->height = anwb->height;
        region->pixel_format = mir_pixel_format_abgr_8888;

        return std::shared_ptr<MirGraphicsRegion>(region, del);
    }

private:
    gralloc_module_t* module;
    alloc_device_t* alloc_dev;
};

}

std::shared_ptr<mtd::GraphicsRegionFactory> mtd::create_graphics_region_factory()
{
    return std::make_shared<AndroidGraphicsRegionFactory>();
}
