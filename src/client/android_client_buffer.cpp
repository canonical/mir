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

#include "mir_client/android_client_buffer.h"

namespace mcl=mir::client;

mcl::AndroidClientBuffer::AndroidClientBuffer(std::shared_ptr<AndroidRegistrar> registrar,
                                              std::shared_ptr<MirBufferPackage> && package )
 : buffer_registrar(registrar)
{
    auto buffer_package = std::move(package);
    native_handle = convert_to_native_handle(buffer_package);

    buffer_registrar->register_buffer(native_handle);
}

mcl::AndroidClientBuffer::~AndroidClientBuffer()
{
    buffer_registrar->unregister_buffer(native_handle);
    free(native_handle);
}

native_handle_t* mcl::AndroidClientBuffer::convert_to_native_handle(const std::shared_ptr<MirBufferPackage>& package)
{
    int native_handle_header_size = 3;
    int total = package->fd.size() + package->data.size() + native_handle_header_size;
    native_handle_t* handle = (native_handle_t*) malloc(sizeof(int) * total );

    handle->numFds  = package->fd.size();
    handle->numInts = package->data.size();

    int i=0;
    for(auto it= package->fd.begin(); it != package->fd.end(); it++)
    {
        handle->data[i++] = *it;
    } 
    for(auto it= package->fd.begin(); it != package->fd.end(); it++)
    {
        handle->data[i++] = *it;
    } 

    return handle;
}

struct MemoryRegionDeleter
{
    MemoryRegionDeleter(const std::shared_ptr<mcl::AndroidRegistrar> reg,
                        const native_handle_t* pack)
     : package(pack),
       registrar(reg)
    {}

    void operator()(mcl::MemoryRegion *reg)
    {
        registrar->release_from_cpu(package);
        delete reg;
    }
private:
    const native_handle_t* package;
    const std::shared_ptr<mcl::AndroidRegistrar> registrar;
};

std::shared_ptr<mcl::MemoryRegion> mcl::AndroidClientBuffer::secure_for_cpu_write()
{
    char* vaddr = buffer_registrar->secure_for_cpu(native_handle);

    MemoryRegion *region_raw = new mcl::MemoryRegion{0, 0, vaddr, 0};

    MemoryRegionDeleter del(buffer_registrar, native_handle);
    std::shared_ptr<mcl::MemoryRegion> region(region_raw, del);

    return region;
}
