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

#include "mir_test_framework/stub_client_platform_factory.h"
#include "mir/test/doubles/stub_client_buffer_factory.h"
#include "mir/client_buffer_factory.h"
#include "mir/client_buffer.h"
#include "mir/client_platform.h"
#include "mir/client_context.h"

#include <unistd.h>
#include <string.h>

namespace mcl = mir::client;
namespace geom = mir::geometry;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

namespace
{
struct StubClientPlatform : public mcl::ClientPlatform
{
    StubClientPlatform(mcl::ClientContext* context)
        : context{context}
    {
    }

    MirPlatformType platform_type() const
    {
        return mir_platform_type_gbm;
    }

    void populate(MirPlatformPackage& package) const
    {
        context->populate_server_package(package);
    }

    MirPlatformMessage* platform_operation(MirPlatformMessage const*) override
    {
        return nullptr;
    }

    std::shared_ptr<mcl::ClientBufferFactory> create_buffer_factory()
    {
        return std::make_shared<mtd::StubClientBufferFactory>();
    }

    std::shared_ptr<EGLNativeWindowType> create_egl_native_window(mcl::EGLNativeSurface* surface)
    {
        auto fake_window = reinterpret_cast<EGLNativeWindowType>(surface);
        return std::make_shared<EGLNativeWindowType>(fake_window);
    }

    std::shared_ptr<EGLNativeDisplayType> create_egl_native_display()
    {
        auto fake_display = reinterpret_cast<EGLNativeDisplayType>(0x12345678lu);
        return std::make_shared<EGLNativeDisplayType>(fake_display);
    }

    MirNativeBuffer* convert_native_buffer(mir::graphics::NativeBuffer* buf) const
    {
        static_cast<void>(buf);
#ifdef MESA_KMS
        return buf;
#else
        return nullptr;
#endif
    }

    MirPixelFormat get_egl_pixel_format(EGLDisplay, EGLConfig) const override
    {
        return mir_pixel_format_argb_8888;
    }

    mcl::ClientContext* const context;
};
}

std::shared_ptr<mcl::ClientPlatform>
mtf::StubClientPlatformFactory::create_client_platform(mcl::ClientContext* context)
{
    return std::make_shared<StubClientPlatform>(context);
}
