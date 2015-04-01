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
#include "mir/client_buffer_factory.h"
#include "mir/client_context.h"

//#include <cstring>

namespace mcl=mir::client;
namespace mclx=mcl::X;
namespace geom=mir::geometry;

mclx::ClientPlatform::ClientPlatform()
{
}

std::shared_ptr<mcl::ClientBufferFactory> mclx::ClientPlatform::create_buffer_factory()
{
//    return std::make_shared<mclm::ClientBufferFactory>(buffer_file_ops);
    return nullptr;
}

std::shared_ptr<EGLNativeWindowType> mclx::ClientPlatform::create_egl_native_window(EGLNativeSurface* /* client_surface */)
{
#if 0
    //TODO: this is awkward on both android and gbm...
    auto native_window = new NativeSurface(*client_surface);
    auto egl_native_window = new EGLNativeWindowType;
    *egl_native_window = reinterpret_cast<EGLNativeWindowType>(native_window);
    NativeWindowDeleter deleter(native_window);
    return std::shared_ptr<EGLNativeWindowType>(egl_native_window, deleter);
#else
    return 0;
#endif
}

std::shared_ptr<EGLNativeDisplayType> mclx::ClientPlatform::create_egl_native_display()
{
#if 0
    MirEGLNativeDisplayType *mir_native_display = new MirEGLNativeDisplayType;
    *mir_native_display = display_container.create(this);
    auto egl_native_display = reinterpret_cast<EGLNativeDisplayType*>(mir_native_display);

    return std::shared_ptr<EGLNativeDisplayType>(egl_native_display, NativeDisplayDeleter(display_container));
#else
    return 0;
#endif
}

MirPlatformType mclx::ClientPlatform::platform_type() const
{
    return mir_platform_type_X;
}

void mclx::ClientPlatform::populate(MirPlatformPackage& /* package*/) const
{
#if 0
    size_t constexpr pointer_size_in_ints = division_ceiling(sizeof(gbm_dev), sizeof(int));

    context->populate_server_package(package);

    auto const total_data_size = package.data_items + pointer_size_in_ints;
    if (gbm_dev && total_data_size <= mir_platform_package_max)
    {
        int gbm_ptr[pointer_size_in_ints]{};
        std::memcpy(&gbm_ptr, &gbm_dev, sizeof(gbm_dev));

        for (auto i : gbm_ptr)
            package.data[package.data_items++] = i;
    }
#endif
}

MirPlatformMessage* mclx::ClientPlatform::platform_operation(
    MirPlatformMessage const* /*msg*/)
{
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
    //MirNativeBuffer is type-compatible with the MirNativeBuffer
    return buf;
}
