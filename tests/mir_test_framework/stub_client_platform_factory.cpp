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
#include "src/client/client_buffer_factory.h"
#include "src/client/client_buffer.h"
#include "src/client/client_platform.h"

#include <string.h>

namespace mcl = mir::client;
namespace geom = mir::geometry;
namespace mtf = mir_test_framework;

namespace
{
class StubClientBuffer : public mcl::ClientBuffer
{
public:
    StubClientBuffer(std::shared_ptr<MirBufferPackage> const& package)
    {
        static_cast<void>(package);
#ifndef ANDROID
        native = package;
#endif
    }

    std::shared_ptr<mcl::MemoryRegion> secure_for_cpu_write()
    {
        return nullptr;
    }

    geom::Size size() const
    {
        return geom::Size{};
    }

    geom::Stride stride() const
    {
        return geom::Stride{};
    }

    MirPixelFormat pixel_format() const
    {
        return mir_pixel_format_abgr_8888;
    }

    uint32_t age() const
    {
        return 0;
    }
    void increment_age()
    {
    }
    void mark_as_submitted()
    {
    }
    std::shared_ptr<mir::graphics::NativeBuffer> native_buffer_handle() const
    {
#ifdef ANDROID
        return nullptr;
#else
        return native;
#endif
    }
    void update_from(MirBufferPackage const& package) override
    {
        static_cast<void>(package);
#ifndef ANDROID
        ::memcpy(native.get(), &package, sizeof(package));
#endif
    }
private:
#ifndef ANDROID
    std::shared_ptr<mir::graphics::NativeBuffer> native;
#endif

    virtual void fill_update_msg(MirBufferPackage& /*message*/) override
    {
    }
};

struct StubClientBufferFactory : public mcl::ClientBufferFactory
{
    std::shared_ptr<mcl::ClientBuffer> create_buffer(std::shared_ptr<MirBufferPackage> const& package,
                                                     geom::Size, MirPixelFormat)
    {
        return std::make_shared<StubClientBuffer>(package);
    }
};

struct StubClientPlatform : public mcl::ClientPlatform
{
    MirPlatformType platform_type() const
    {
        return mir_platform_type_gbm;
    }

    std::shared_ptr<mcl::ClientBufferFactory> create_buffer_factory()
    {
        return std::make_shared<StubClientBufferFactory>();
    }

    std::shared_ptr<EGLNativeWindowType> create_egl_native_window(mcl::ClientSurface*)
    {
        auto fake_window = reinterpret_cast<EGLNativeWindowType>(0x12345678lu);
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
#ifndef ANDROID
        return buf;
#else
        return nullptr;
#endif
    }
};
}

std::shared_ptr<mcl::ClientPlatform>
mtf::StubClientPlatformFactory::create_client_platform(mcl::ClientContext*)
{
    return std::make_shared<StubClientPlatform>();
}
