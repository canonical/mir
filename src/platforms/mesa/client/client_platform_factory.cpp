/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/client/client_platform_factory.h"
#include "client_platform.h"
#include "mir_toolkit/client_types.h"
#include "mir/client/client_context.h"
#include "buffer_file_ops.h"
#include "mir/client/egl_native_display_container.h"
#include "mir/assert_module_entry_point.h"
#include "mir/module_deleter.h"

#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <cstring>

namespace mcl = mir::client;
namespace mclm = mcl::mesa;

namespace
{
// Re-export our own symbols from mesa.so.N globally so that Mesa itself can
// find them with a simple dlsym(NULL,)
void ensure_loaded_with_rtld_global_mesa_client()
{
#ifdef MIR_EGL_SUPPORTED
    Dl_info info;

    dladdr(reinterpret_cast<void*>(&ensure_loaded_with_rtld_global_mesa_client), &info);
    void* reexport_self_global =
        dlopen(info.dli_fname, RTLD_NOW | RTLD_NOLOAD | RTLD_GLOBAL);
    // Yes, RTLD_NOLOAD does increase the ref count. So dlclose...
    if (reexport_self_global)
        dlclose(reexport_self_global);
#endif
}

struct RealBufferFileOps : public mclm::BufferFileOps
{
    int close(int fd) const
    {
        while (::close(fd) == -1)
        {
            // Retry on EINTR, return error on anything else
            if (errno != EINTR)
                return errno;
        }
        return 0;
    }

    void* map(int fd, off_t offset, size_t size) const
    {
        return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    fd, offset);
    }

    void unmap(void* addr, size_t size) const
    {
        munmap(addr, size);
    }
};

}

mir::UniqueModulePtr<mcl::ClientPlatform> create_client_platform(
    mcl::ClientContext* context,
    std::shared_ptr<mir::logging::Logger> const& /*logger*/)
{
    mir::assert_entry_point_signature<mcl::CreateClientPlatform>(&create_client_platform);
    ensure_loaded_with_rtld_global_mesa_client();
    MirPlatformPackage package;
    context->populate_server_package(package);
    if (package.data_items != 0 || package.fd_items != 1)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Attempted to create Mesa client platform on non-Mesa server"}));
    }
    auto buffer_file_ops = std::make_shared<RealBufferFileOps>();
    return mir::make_module_ptr<mclm::ClientPlatform>(
        context, buffer_file_ops, mcl::EGLNativeDisplayContainer::instance());
}

bool
is_appropriate_module(mcl::ClientContext* context)
{
    mir::assert_entry_point_signature<mcl::ClientPlatformProbe>(&is_appropriate_module);

    MirModuleProperties server_graphics_module;
    context->populate_graphics_module(server_graphics_module);

    // We may in future wish to compare versions, but the mesa server/mesa client interface hasn't
    // changed in ages.
    return (strncmp("mir:mesa", server_graphics_module.name, strlen("mir:mesa")) == 0);
}
