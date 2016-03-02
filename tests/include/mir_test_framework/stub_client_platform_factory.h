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

namespace mir_test_framework
{

struct StubClientPlatform : public mir::client::ClientPlatform
{
    StubClientPlatform(mir::client::ClientContext* context);
    MirPlatformType platform_type() const;
    void populate(MirPlatformPackage& package) const;
    MirPlatformMessage* platform_operation(MirPlatformMessage const*) override;
    std::shared_ptr<mir::client::ClientBufferFactory> create_buffer_factory();
    std::shared_ptr<void> create_egl_native_window(mir::client::EGLNativeSurface* surface);
    std::shared_ptr<EGLNativeDisplayType> create_egl_native_display();
    MirNativeBuffer* convert_native_buffer(mir::graphics::NativeBuffer* buf) const;
    MirPixelFormat get_egl_pixel_format(EGLDisplay, EGLConfig) const override;
    mir::client::ClientContext* const context;
};

struct StubClientPlatformFactory : public mir::client::ClientPlatformFactory
{
    std::shared_ptr<mir::client::ClientPlatform> create_client_platform(mir::client::ClientContext* context) override;
};

}
#endif /* MIR_TEST_FRAMEWORK_STUB_CLIENT_PLATFORM_ */
