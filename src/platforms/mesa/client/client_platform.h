/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_CLIENT_MESA_CLIENT_PLATFORM_H_
#define MIR_CLIENT_MESA_CLIENT_PLATFORM_H_

#include "mir/client/client_platform.h"
#include "mir_toolkit/extensions/mesa_drm_auth.h"
#include "mir_toolkit/extensions/set_gbm_device.h"
#include "mir_toolkit/extensions/gbm_buffer.h"
#include "mir_toolkit/extensions/hardware_buffer_stream.h"

struct gbm_device;

namespace mir
{
namespace client
{
class EGLNativeDisplayContainer;

namespace mesa
{

class BufferFileOps;

class ClientPlatform : public client::ClientPlatform
{
public:
    ClientPlatform(ClientContext* const context,
                   std::shared_ptr<BufferFileOps> const& buffer_file_ops,
                   EGLNativeDisplayContainer& display_container);

    MirPlatformType platform_type() const override;
    void populate(MirPlatformPackage& package) const override;
    MirPlatformMessage* platform_operation(MirPlatformMessage const* request) override;
    std::shared_ptr<ClientBufferFactory> create_buffer_factory() override;
    void* request_interface(char const* name, int version) override;
    std::shared_ptr<void> create_egl_native_window(EGLNativeSurface* surface) override;
    void use_egl_native_window(std::shared_ptr<void> native_window, EGLNativeSurface* surface) override;
    std::shared_ptr<EGLNativeDisplayType> create_egl_native_display() override;
    MirNativeBuffer* convert_native_buffer(graphics::NativeBuffer*) const override;
    MirPixelFormat get_egl_pixel_format(EGLDisplay, EGLConfig) const override;
    void set_gbm_device(gbm_device*);
    uint32_t native_format_for(MirPixelFormat) const override;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    uint32_t native_flags_for(MirBufferUsage, mir::geometry::Size) const override;
#pragma GCC diagnostic pop

private:
    ClientContext* const context;
    std::shared_ptr<BufferFileOps> const buffer_file_ops;
    EGLNativeDisplayContainer& display_container;
    gbm_device* gbm_dev;
    MirExtensionMesaDRMAuthV1 drm_extensions;
    MirExtensionSetGbmDeviceV1 mesa_auth;
    MirExtensionGbmBufferV1 gbm_buffer1;
    MirExtensionGbmBufferV2 gbm_buffer2;
    MirExtensionHardwareBufferStreamV1 hw_stream;
};

}
}
}

#endif /* MIR_CLIENT_MESA_CLIENT_PLATFORM_H_ */
