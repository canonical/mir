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
struct NativeHandleDeleter
{
    NativeHandleDeleter(const std::shared_ptr<const gralloc_module_t>& mod)
        : module(mod)
    {}

    void operator()(const native_handle_t* t)
    {
        module->unregisterBuffer(module.get(), t);
        for(auto i = 0; i < t->numFds; i++)
        {
            close(t->data[i]);
        }
        ::operator delete(const_cast<native_handle_t*>(t));
    }
private:
    const std::shared_ptr<const gralloc_module_t> module;
};

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

std::shared_ptr<const native_handle_t> mcla::AndroidRegistrarGralloc::register_buffer(
    std::shared_ptr<MirBufferPackage> const& package) const
{
    int native_handle_header_size = sizeof(native_handle_t);
    int total_size = sizeof(int) *
                     (package->fd_items + package->data_items + native_handle_header_size);
    native_handle_t* handle = static_cast<native_handle_t*>(::operator new(total_size));
    handle->version = native_handle_header_size;
    handle->numFds  = package->fd_items;
    handle->numInts = package->data_items;
    for (auto i=0; i< handle->numFds; i++)
    {
        handle->data[i] = package->fd[i];
    }

    int offset_i = handle->numFds;
    for (auto i=0; i< handle->numInts; i++)
    {
        handle->data[offset_i+i] = package->data[i];
    }

    if ( gralloc_module->registerBuffer(gralloc_module.get(), handle) )
    {
        ::operator delete(const_cast<native_handle_t*>(handle));
        BOOST_THROW_EXCEPTION(std::runtime_error("error registering graphics buffer for client use\n"));
    }

    NativeHandleDeleter del(gralloc_module);
    return std::shared_ptr<const native_handle_t>(handle, del);
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
