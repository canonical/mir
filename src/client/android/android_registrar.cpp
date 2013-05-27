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
 * Authored by: Kevin DuBois<kevin.dubois@canonical.com>
 */

#include "android_registrar_gralloc.h"
#include "../client_buffer.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mcl=mir::client;
namespace mcla=mir::client::android;
namespace geom=mir::geometry;

namespace
{
struct MemoryRegionDeleter
{
    MemoryRegionDeleter(const std::shared_ptr<const gralloc_module_t>& mod,
                        const std::shared_ptr<const native_handle_t>&  han)
     : handle(han),
       module(mod)
    {}

    void operator()(char *)
    {
        module->unlock(module.get(), handle.get());
        //we didn't alloc region(just mapped it), so we don't delete
    }
private:
    const std::shared_ptr<const native_handle_t>  handle;
    const std::shared_ptr<const gralloc_module_t> module;
};
}

mcla::AndroidRegistrarGralloc::AndroidRegistrarGralloc(const std::shared_ptr<const gralloc_module_t>& gr_module)
: gralloc_module(gr_module)
{
}

void mcla::AndroidRegistrarGralloc::register_buffer(const native_handle_t* handle)
{
    if ( gralloc_module->registerBuffer(gralloc_module.get(), handle) )
        BOOST_THROW_EXCEPTION(std::runtime_error("error registering graphics buffer for client use\n"));
}

void mcla::AndroidRegistrarGralloc::unregister_buffer(const native_handle_t* handle)
{
    gralloc_module->unregisterBuffer(gralloc_module.get(), handle);
}

std::shared_ptr<char> mcla::AndroidRegistrarGralloc::secure_for_cpu(std::shared_ptr<const native_handle_t> handle, const geometry::Rectangle rect)
{
    char* vaddr;
    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
    int width = rect.size.width.as_uint32_t();
    int height = rect.size.height.as_uint32_t();
    int top = rect.top_left.x.as_uint32_t();
    int left = rect.top_left.y.as_uint32_t();
    if ( gralloc_module->lock(gralloc_module.get(), handle.get(),
                              usage, top, left, width, height, (void**) &vaddr) )
        BOOST_THROW_EXCEPTION(std::runtime_error("error securing buffer for client cpu use"));

    MemoryRegionDeleter del(gralloc_module, handle);
    return std::shared_ptr<char>(vaddr, del);
}


