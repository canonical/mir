/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_TEST_FRAMEWORK_STUB_CLIENT_PLATFORM_FACTORY_
#define MIR_TEST_FRAMEWORK_STUB_CLIENT_PLATFORM_FACTORY_

#include "mir/client_platform_factory.h"
#include "mir/client_platform.h"
#include "mir_toolkit/mir_native_buffer.h"
#include "mir_toolkit/extensions/fenced_buffers.h"
#include "mir_toolkit/extensions/gbm_buffer.h"
#include "mir_toolkit/extensions/hardware_buffer_stream.h"
#include "mir_test_framework/stub_platform_extension.h"
#include "mir_test_framework/stub_client_platform_options.h"

#include <unordered_map>
#include <exception>

namespace mir_test_framework
{

struct StubClientPlatform : public mir::client::ClientPlatform
{
    StubClientPlatform(mir::client::ClientContext* context);
    StubClientPlatform(
        mir::client::ClientContext* context,
        std::unordered_map<FailurePoint, std::exception_ptr, std::hash<int>>&& fail_at);

    MirPlatformType platform_type() const override;
    void populate(MirPlatformPackage& package) const override;
    MirPlatformMessage* platform_operation(MirPlatformMessage const*) override;
    std::shared_ptr<mir::client::ClientBufferFactory> create_buffer_factory() override;
    void* request_interface(char const* name, int version) override;
    void use_egl_native_window(std::shared_ptr<void> native_window, mir::client::EGLNativeSurface* surface) override;
    std::shared_ptr<void> create_egl_native_window(mir::client::EGLNativeSurface* surface) override;
    std::shared_ptr<EGLNativeDisplayType> create_egl_native_display() override;
    MirNativeBuffer* convert_native_buffer(mir::graphics::NativeBuffer* buf) const override;
    MirPixelFormat get_egl_pixel_format(EGLDisplay, EGLConfig) const override;
    uint32_t native_format_for(MirPixelFormat) const override;
    uint32_t native_flags_for(MirBufferUsage, mir::geometry::Size) const override;

    mir::client::ClientContext* const context;
    MirBufferPackage mutable native_buffer;

    MirExtensionFavoriteFlavorV1 flavor_ext_1;
    MirExtensionFavoriteFlavorV9 flavor_ext_9;
    MirExtensionAnimalNamesV1 animal_ext;
    MirExtensionFencedBuffersV1 fence_ext;
    MirExtensionGbmBufferV1 buffer_ext;
    MirExtensionHardwareBufferStreamV1 stream_ext;

    std::unordered_map<FailurePoint, std::exception_ptr, std::hash<int>> const fail_at;
};

struct StubClientPlatformFactory : public mir::client::ClientPlatformFactory
{
    std::shared_ptr<mir::client::ClientPlatform> create_client_platform(mir::client::ClientContext* context) override;
};

}
#endif /* MIR_TEST_FRAMEWORK_STUB_CLIENT_PLATFORM_ */
