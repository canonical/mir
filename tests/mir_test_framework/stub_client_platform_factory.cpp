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
#include "mir/client_context.h"
#include "mir_test_framework/stub_platform_native_buffer.h"
#include "mir_test_framework/stub_platform_extension.h"
#include "mir_toolkit/mir_native_buffer.h"

#include <unistd.h>
#include <string.h>

namespace mcl = mir::client;
namespace geom = mir::geometry;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

namespace
{
char const* favorite_flavor_1()
{
    static char const* favorite = "banana";
    return favorite;
}
char const* favorite_flavor_9()
{
    static char const* favorite = "rhubarb";
    return favorite;
}
char const* animal_name()
{
    static char const* name = "bobcat";
    return name;
}

int get_fence(MirBuffer*)
{
    return -1;
}

void allocate_buffer(
    MirConnection* connection, int width, int height, unsigned int pf, unsigned int flags, MirBufferCallback cb, void* cb_context)
{
    auto context = mcl::to_client_context(connection);
    context->allocate_buffer(
        mir::geometry::Size{width, height}, pf, flags, cb, cb_context);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirBufferStream* get_stream(
    MirRenderSurface* rs,
    int width, int height,
    MirPixelFormat format)
{
    return mir_render_surface_get_buffer_stream(rs, width, height, format);
}
#pragma GCC diagnostic pop

void throw_exception_if_requested(
    std::unordered_map<mtf::FailurePoint, std::exception_ptr, std::hash<int>> const& fail_at,
    mtf::FailurePoint here)
{
    if (fail_at.count(here))
    {
        std::rethrow_exception(fail_at.at(here));
    }
}
}

mtf::StubClientPlatform::StubClientPlatform(mir::client::ClientContext* context)
    : StubClientPlatform(context, std::unordered_map<FailurePoint, std::exception_ptr, std::hash<int>>{})
{
}

mtf::StubClientPlatform::StubClientPlatform(
    mir::client::ClientContext* context,
    std::unordered_map<FailurePoint, std::exception_ptr, std::hash<int>>&& fail_at) :
    context{context},
    flavor_ext_1{favorite_flavor_1},
    flavor_ext_9{favorite_flavor_9},
    animal_ext{animal_name},
    fence_ext{get_fence, nullptr, nullptr},
    buffer_ext{allocate_buffer},
    stream_ext{get_stream},
    fail_at{std::move(fail_at)}
{
    throw_exception_if_requested(this->fail_at, FailurePoint::create_client_platform);
}

MirPlatformType mtf::StubClientPlatform::platform_type() const
{
    return mir_platform_type_gbm;
}

void mtf::StubClientPlatform::populate(MirPlatformPackage& package) const
{
    context->populate_server_package(package);
}

MirPlatformMessage* mtf::StubClientPlatform::platform_operation(MirPlatformMessage const*)
{
    return nullptr;
}

std::shared_ptr<mir::client::ClientBufferFactory> mtf::StubClientPlatform::create_buffer_factory()
{
    throw_exception_if_requested(this->fail_at, FailurePoint::create_buffer_factory);

    struct StubPlatformBufferFactory : mcl::ClientBufferFactory
    {
        std::shared_ptr<mcl::ClientBuffer> create_buffer(
            std::shared_ptr<MirBufferPackage> const& package,
            geom::Size size, MirPixelFormat pf) override
        {
            mir::graphics::BufferUsage usage = mir::graphics::BufferUsage::software;
            if (package->data[0] == static_cast<int>(mir::graphics::BufferUsage::hardware))
                usage = mir::graphics::BufferUsage::hardware;
            mir::graphics::BufferProperties properties {size, pf, usage }; 
            return std::make_shared<mtd::StubClientBuffer>(package, size, pf,
                std::make_shared<mtf::NativeBuffer>(properties));
        }

        std::shared_ptr<mcl::ClientBuffer> create_buffer(
            std::shared_ptr<MirBufferPackage> const& package, uint32_t, uint32_t) override
        {
            mir::graphics::BufferUsage usage = mir::graphics::BufferUsage::hardware;
            geom::Size size {package->width, package->height};
            auto pf = mir_pixel_format_abgr_8888;
            mir::graphics::BufferProperties properties { size, pf, usage }; 
            return std::make_shared<mtd::StubClientBuffer>(package, size, pf,
                std::make_shared<mtf::NativeBuffer>(properties));
        }
    };
    return std::make_shared<StubPlatformBufferFactory>();
}

void mtf::StubClientPlatform::use_egl_native_window(std::shared_ptr<void> /*native_window*/, mir::client::EGLNativeSurface* /*surface*/)
{
}

std::shared_ptr<void> mtf::StubClientPlatform::create_egl_native_window(mir::client::EGLNativeSurface* surface)
{
    throw_exception_if_requested(this->fail_at, FailurePoint::create_egl_native_window);
    if (surface)
        return std::shared_ptr<void>{surface, [](void*){}};
    return std::make_shared<int>(332);
}

std::shared_ptr<EGLNativeDisplayType> mtf::StubClientPlatform::create_egl_native_display()
{
    auto fake_display = reinterpret_cast<EGLNativeDisplayType>(0x12345678lu);
    return std::make_shared<EGLNativeDisplayType>(fake_display);
}

MirNativeBuffer* mtf::StubClientPlatform::convert_native_buffer(mir::graphics::NativeBuffer* b) const
{
    auto buf = dynamic_cast<mtf::NativeBuffer*>(b);
    if (!buf)
        BOOST_THROW_EXCEPTION(std::invalid_argument("could not convert NativeBuffer"));
    native_buffer.data_items = 1;
    native_buffer.data[0] = buf->data;
    native_buffer.fd_items = 1;
    native_buffer.fd[0] = buf->fd;
    native_buffer.width = buf->properties.size.width.as_int();
    native_buffer.height = buf->properties.size.height.as_int();
    //bit of mesa specific leakage into the client api here.
    if (native_buffer.width >= 800 && native_buffer.height >= 600 &&
        buf->properties.usage == mir::graphics::BufferUsage::hardware)
    {
        native_buffer.flags |= mir_buffer_flag_can_scanout;
    }
    else
    {
        native_buffer.flags &= ~mir_buffer_flag_can_scanout;
    }

    return &native_buffer;
}

MirPixelFormat mtf::StubClientPlatform::get_egl_pixel_format(EGLDisplay, EGLConfig) const
{
    return mir_pixel_format_argb_8888;
}

void* mtf::StubClientPlatform::request_interface(char const* name, int version)
{
    if (!strcmp(name, "mir_extension_favorite_flavor") && (version == 1))
        return &flavor_ext_1;
    if (!strcmp(name, "mir_extension_favorite_flavor") && (version == 9))
        return &flavor_ext_9;
    if (!strcmp(name, "mir_extension_animal_names") && (version == 1))
        return &animal_ext;
    if (!strcmp(name, "mir_extension_fenced_buffers") && (version == 1))
        return &fence_ext;
    if (!strcmp(name, "mir_extension_gbm_buffer") && (version == 1))
        return &buffer_ext;
    if (!strcmp(name, "mir_extension_hardware_buffer_stream") && (version == 1))
        return &stream_ext;
    return nullptr;
}

uint32_t mtf::StubClientPlatform::native_format_for(MirPixelFormat) const
{
    return 0u;
}

uint32_t mtf::StubClientPlatform::native_flags_for(MirBufferUsage usage, mir::geometry::Size) const
{
    return static_cast<uint32_t>(usage);
}

std::shared_ptr<mcl::ClientPlatform>
mtf::StubClientPlatformFactory::create_client_platform(mcl::ClientContext* context)
{
    return std::make_shared<StubClientPlatform>(
        context,
        std::unordered_map<FailurePoint, std::exception_ptr, std::hash<int>>{});
}

