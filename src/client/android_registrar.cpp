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
 * Authored by: Kevin DuBois<kevin.dubois@canonical.com>
 */

#include "mir_client/android_registrar_gralloc.h"
#include "mir_client/client_buffer.h"

namespace mcl=mir::client;

mcl::AndroidRegistrarGralloc::AndroidRegistrarGralloc(const std::shared_ptr<const gralloc_module_t>& gr_module)
: gralloc_module(gr_module)
{
}

void mcl::AndroidRegistrarGralloc::register_buffer(const native_handle_t * handle)
{
    gralloc_module->registerBuffer(gralloc_module.get(), handle);
}

void mcl::AndroidRegistrarGralloc::unregister_buffer(const native_handle_t * handle)
{
    gralloc_module->unregisterBuffer(gralloc_module.get(), handle);
}


struct MemoryRegionDeleter
{
    MemoryRegionDeleter(const std::shared_ptr<const gralloc_module_t>& mod,
                        const std::shared_ptr<const native_handle_t>&  han)
     : handle(han),
       module(mod)
    {

    }

    void operator()(mcl::MemoryRegion *reg)
    {
        module->unlock(module.get(), handle.get());
        delete reg;
    }
private:
    const std::shared_ptr<const native_handle_t>  handle;
    const std::shared_ptr<const gralloc_module_t> module;
};

int mcl::AndroidRegistrarGralloc::extract_width_from_handle(const std::shared_ptr<const native_handle_t>& handle)
{
    int offset = handle->numFds + width_offset_from_fd; 
    return handle->data[offset];
}

int mcl::AndroidRegistrarGralloc::extract_height_from_handle(const std::shared_ptr<const native_handle_t>& handle)
{
    int offset = handle->numFds + height_offset_from_fd; 
    return handle->data[offset];
}

std::shared_ptr<mcl::MemoryRegion> mcl::AndroidRegistrarGralloc::secure_for_cpu(std::shared_ptr<const native_handle_t> handle)
{
    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;

    int width = extract_width_from_handle(handle);
    int height = extract_height_from_handle(handle);
    gralloc_module->lock(gralloc_module.get(), handle.get(), usage, 0, 0, width, height, 0);

    MemoryRegionDeleter del(gralloc_module, handle);
    auto region = new mcl::MemoryRegion; 
    return std::shared_ptr<mcl::MemoryRegion>(region, del);
}


