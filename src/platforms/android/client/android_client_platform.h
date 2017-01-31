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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_CLIENT_ANDROID_ANDROID_CLIENT_PLATFORM_H_
#define MIR_CLIENT_ANDROID_ANDROID_CLIENT_PLATFORM_H_

#include "mir/client_platform.h"
#include "mir_toolkit/extensions/fenced_buffers.h"
#include "mir_toolkit/extensions/android_egl.h"
#include "mir_toolkit/extensions/android_buffer.h"
#include "mir_toolkit/extensions/hardware_buffer_stream.h"

namespace mir
{
namespace logging { class Logger; }
namespace client
{

namespace android
{

class AndroidClientPlatform : public ClientPlatform
{
public:
    AndroidClientPlatform(ClientContext* const context,
                          std::shared_ptr<logging::Logger> const& logger);
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
    uint32_t native_format_for(MirPixelFormat) const override;
    uint32_t native_flags_for(MirBufferUsage, mir::geometry::Size) const override;

private:
    ClientContext* const context;
    std::shared_ptr<logging::Logger> const logger;

    std::shared_ptr<EGLNativeDisplayType> const native_display;
    MirExtensionAndroidEGLV1 android_types_extension;
    MirExtensionFencedBuffersV1 fence_extension;
    MirExtensionAndroidBufferV1 buffer_extension;
    MirExtensionHardwareBufferStreamV1 hw_stream;
};

}
}
}

#endif /* MIR_CLIENT_ANDROID_ANDROID_CLIENT_PLATFORM_H_ */
