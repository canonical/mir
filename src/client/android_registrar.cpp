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
    {}

    void operator()(mcl::MemoryRegion *reg)
    {
        module->unlock(module.get(), handle.get());
        delete reg;
    }
private:
    const std::shared_ptr<const native_handle_t>  handle;
    const std::shared_ptr<const gralloc_module_t> module;
};
#if 0
    char* vaddr = buffer_registrar->secure_for_cpu(native_handle);

    MemoryRegion *region_raw = new mcl::MemoryRegion{0, 0, vaddr, 0};

    MemoryRegionDeleter del(buffer_registrar, native_handle);
    std::shared_ptr<mcl::MemoryRegion> region(region_raw, del);
#endif

std::shared_ptr<mcl::MemoryRegion> mcl::AndroidRegistrarGralloc::secure_for_cpu(std::shared_ptr<const native_handle_t>)
{
//    MemoryRegionDeleter(gralloc_module,  
    return std::make_shared<mcl::MemoryRegion>();
}


