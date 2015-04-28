/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#include "mir_toolkit/mir_client_library.h"
#include "client_platform.h"
#include "client_buffer_factory.h"
#include "mesa_native_display_container.h"
#include "native_surface.h"
#include "mir/client_context.h"
#include "../debug.h"

#include <boost/throw_exception.hpp>

#include <EGL/egl.h>

namespace mcl=mir::client;
namespace mclx=mcl::X;
namespace geom=mir::geometry;

namespace
{
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

namespace
{
struct NativeWindowDeleter
{
    NativeWindowDeleter(mclx::NativeSurface* window)
     : window(window) {}

    void operator()(EGLNativeWindowType* type)
    {
        delete type;
        delete window;
    }

private:
    mclx::NativeSurface* window;
};
}

mclx::ClientPlatform::ClientPlatform(
        ClientContext* const context,
        std::shared_ptr<BufferFileOps> const& buffer_file_ops,
        mcl::EGLNativeDisplayContainer& display_container)
    : context{context},
      buffer_file_ops{buffer_file_ops},
      display_container(display_container)
{
    CALLED
}

std::shared_ptr<mcl::ClientBufferFactory> mclx::ClientPlatform::create_buffer_factory()
{
    CALLED

    return std::make_shared<mclx::ClientBufferFactory>(buffer_file_ops);
}

std::shared_ptr<EGLNativeWindowType> mclx::ClientPlatform::create_egl_native_window(EGLNativeSurface* client_surface)
{
    CALLED

    auto native_window = new NativeSurface(*client_surface);
    auto egl_native_window = new EGLNativeWindowType;
    *egl_native_window = reinterpret_cast<EGLNativeWindowType>(native_window);
    NativeWindowDeleter deleter(native_window);
    return std::shared_ptr<EGLNativeWindowType>(egl_native_window, deleter);
}

std::shared_ptr<EGLNativeDisplayType> mclx::ClientPlatform::create_egl_native_display()
{
    CALLED

    MirEGLNativeDisplayType *mir_native_display = new MirEGLNativeDisplayType;
    *mir_native_display = display_container.create(this);
    auto egl_native_display = reinterpret_cast<EGLNativeDisplayType*>(mir_native_display);

    return std::shared_ptr<EGLNativeDisplayType>(egl_native_display, NativeDisplayDeleter(display_container));
}

MirPlatformType mclx::ClientPlatform::platform_type() const
{
    CALLED

    return mir_platform_type_X;
}

void mclx::ClientPlatform::populate(MirPlatformPackage& package) const
{
    CALLED

    context->populate_server_package(package);
}

MirPlatformMessage* mclx::ClientPlatform::platform_operation(
    MirPlatformMessage const* /*msg*/)
{
    CALLED

#if 0
    auto const op = mir_platform_message_get_opcode(msg);
    auto const msg_data = mir_platform_message_get_data(msg);

    if (op == MirMesaPlatformOperation::set_gbm_device &&
        msg_data.size == sizeof(MirMesaSetGBMDeviceRequest))
    {
        MirMesaSetGBMDeviceRequest set_gbm_device_request{nullptr};
        std::memcpy(&set_gbm_device_request, msg_data.data, msg_data.size);

        gbm_dev = set_gbm_device_request.device;

        static int const success{0};
        MirMesaSetGBMDeviceResponse const response{success};
        auto const response_msg = mir_platform_message_create(op);
        mir_platform_message_set_data(response_msg, &response, sizeof(response));

        return response_msg;
    }
#endif
    return nullptr;
}

MirNativeBuffer* mclx::ClientPlatform::convert_native_buffer(graphics::NativeBuffer* buf) const
{
    CALLED

    //MirNativeBuffer is type-compatible with the MirNativeBuffer
    return buf;
}
