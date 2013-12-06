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

#include "mir_toolkit/mir_client_library.h"
#include "client_platform.h"
#include "client_buffer_factory.h"
#include "mesa_native_display_container.h"
#include "buffer_file_ops.h"
#include "native_surface.h"
#include "../mir_connection.h"
#include "../client_buffer_factory.h"
#include "../native_client_platform_factory.h"

#include <sys/mman.h>
#include <unistd.h>

namespace mcl=mir::client;
namespace mclm=mir::client::mesa;
namespace geom=mir::geometry;

namespace
{

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

struct NativeDisplayDeleter
{
    NativeDisplayDeleter(mcl::EGLNativeDisplayContainer& container)
    : container(container)
    {
    }
    void operator() (EGLNativeDisplayType* p)
    {
        auto display = *(reinterpret_cast<MirEGLNativeDisplayType*>(p));
        container.release(display);
        delete p;
    }

    mcl::EGLNativeDisplayContainer& container;
};

}

std::shared_ptr<mcl::ClientPlatform>
mcl::NativeClientPlatformFactory::create_client_platform(mcl::ClientContext* context)
{
    auto buffer_file_ops = std::make_shared<RealBufferFileOps>();
    return std::make_shared<mclm::ClientPlatform>(
        context, buffer_file_ops, mcl::EGLNativeDisplayContainer::instance());
}

mclm::ClientPlatform::ClientPlatform(
        ClientContext* const context,
        std::shared_ptr<BufferFileOps> const& buffer_file_ops,
        mcl::EGLNativeDisplayContainer& display_container)
    : context{context},
      buffer_file_ops{buffer_file_ops},
      display_container(display_container)
{
}

std::shared_ptr<mcl::ClientBufferFactory> mclm::ClientPlatform::create_buffer_factory()
{
    return std::make_shared<mclm::ClientBufferFactory>(buffer_file_ops);
}

namespace
{
struct NativeWindowDeleter
{
    NativeWindowDeleter(mclm::NativeSurface* window)
     : window(window) {}

    void operator()(EGLNativeWindowType* type)
    {
        delete type;
        delete window;
    }

private:
    mclm::NativeSurface* window;
};
}

std::shared_ptr<EGLNativeWindowType> mclm::ClientPlatform::create_egl_native_window(ClientSurface* client_surface)
{
    //TODO: this is awkward on both android and gbm...
    auto native_window = new NativeSurface(*client_surface);
    auto egl_native_window = new EGLNativeWindowType;
    *egl_native_window = native_window;
    NativeWindowDeleter deleter(native_window);
    return std::shared_ptr<EGLNativeWindowType>(egl_native_window, deleter);
}

std::shared_ptr<EGLNativeDisplayType> mclm::ClientPlatform::create_egl_native_display()
{
    MirEGLNativeDisplayType *mir_native_display = new MirEGLNativeDisplayType;
    *mir_native_display = display_container.create(context->mir_connection());
    auto egl_native_display = reinterpret_cast<EGLNativeDisplayType*>(mir_native_display);

    return std::shared_ptr<EGLNativeDisplayType>(egl_native_display, NativeDisplayDeleter(display_container));
}

MirPlatformType mclm::ClientPlatform::platform_type() const
{
    return mir_platform_type_gbm;
}

MirNativeBuffer* mclm::ClientPlatform::convert_native_buffer(graphics::NativeBuffer* buf) const
{
    //MirNativeBuffer is type-compatible with the MirNativeBuffer
    return buf;
}
