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


#include "mir_test/draw/android_graphics.h"

#include <fstream>
#include <stdexcept>
#include <dirent.h>
#include <fnmatch.h>

namespace mtd=mir::test::draw;
namespace mc=mir::compositor;
namespace geom=mir::geometry;

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

static const char* proc_dir = "/proc";
static const char* surface_flinger_executable_name = "surfaceflinger";
int surface_flinger_filter(const struct dirent* d)
{
    if (fnmatch("[1-9]*", d->d_name, 0))
        return 0;

    char path[256];
    snprintf(path, sizeof(path), "%s/%s/cmdline", proc_dir, d->d_name);

    std::ifstream in(path);
    std::string line;

    while(std::getline(in, line))
    {
        if (line.find(surface_flinger_executable_name) != std::string::npos)
            return 1;
    }

    return 0;
}
}

mtd::TestGrallocMapper::TestGrallocMapper()
 : gralloc_ownership(true)
{
    const hw_module_t *hw_module;
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module) != 0)
        throw std::runtime_error("error, hw module not available!\n");
    gralloc_open(hw_module, &alloc_dev);
    module = (gralloc_module_t*) hw_module;
}

mtd::TestGrallocMapper::TestGrallocMapper(const hw_module_t *hw_module,
                                     alloc_device_t* alloc_dev)
 : gralloc_ownership(false),
   module((gralloc_module_t*)hw_module),
   alloc_dev(alloc_dev)
{
}

mtd::TestGrallocMapper::~TestGrallocMapper()
{
    if (gralloc_ownership)
        gralloc_close(alloc_dev);
}

std::shared_ptr<MirGraphicsRegion> mtd::TestGrallocMapper::graphic_region_from_handle(
    ANativeWindowBuffer* package)
{
    int *vaddr;
    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
    module->lock(module, package->handle, usage, 0, 0, package->width, package->height, (void**) &vaddr);

    MirGraphicsRegion* region = new MirGraphicsRegion;
    RegionDeleter del(module, package->handle);

    region->vaddr = (char*) vaddr;
    region->stride = package->stride;
    region->width = package->width;
    region->height = package->height;
    region->pixel_format = mir_pixel_format_abgr_8888;

    return std::shared_ptr<MirGraphicsRegion>(region, del);
}

bool mtd::is_surface_flinger_running()
{
    struct dirent **namelist;
    return 0 < scandir(proc_dir, &namelist, surface_flinger_filter, 0);
}
