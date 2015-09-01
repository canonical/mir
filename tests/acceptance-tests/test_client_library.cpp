/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/stub_platform_helpers.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/validity_matchers.h"

#include "src/include/client/mir/client_buffer.h"

#include "mir_protobuf.pb.h"

#ifdef ANDROID
/*
 * MirNativeBuffer for Android is defined opaquely, but we now depend on
 * it having width and height fields, for all platforms. So need definition...
 */
#include <system/window.h>  // for ANativeWindowBuffer AKA MirNativeBuffer
#endif

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>

namespace mf = mir::frontend;
namespace mc = mir::compositor;
namespace mcl = mir::client;
namespace mtf = mir_test_framework;
namespace
{
struct ClientLibrary : mtf::HeadlessInProcessServer
{
    std::set<MirSurface*> surfaces;
    MirConnection* connection = nullptr;
    MirSurface* surface  = nullptr;
    std::atomic<int> buffers{0};

    static void connection_callback(MirConnection* connection, void* context)
    {
        ClientLibrary* config = reinterpret_cast<ClientLibrary*>(context);
        config->connected(connection);
    }

    static void create_surface_callback(MirSurface* surface, void* context)
    {
        ClientLibrary* config = reinterpret_cast<ClientLibrary*>(context);
        config->surface_created(surface);
    }

    static void next_buffer_callback(MirBufferStream* bs, void* context)
    {
        ClientLibrary* config = reinterpret_cast<ClientLibrary*>(context);
        config->next_buffer(bs);
    }

    static void release_surface_callback(MirSurface* surface, void* context)
    {
        ClientLibrary* config = reinterpret_cast<ClientLibrary*>(context);
        config->surface_released(surface);
    }

    virtual void connected(MirConnection* new_connection)
    {
        connection = new_connection;
    }

    virtual void surface_created(MirSurface* new_surface)
    {
        surfaces.insert(new_surface);
        surface = new_surface;
    }

    virtual void next_buffer(MirBufferStream*)
    {
        ++buffers;
    }

    void surface_released(MirSurface* old_surface)
    {
        surfaces.erase(old_surface);
        surface = NULL;
    }

    MirSurface* any_surface()
    {
        return *surfaces.begin();
    }

    size_t current_surface_count()
    {
        return surfaces.size();
    }

    static void nosey_thread(MirSurface *surf)
    {
        for (int i = 0; i < 10; i++)
        {
            mir_wait_for_one(mir_surface_set_state(surf,
                                            mir_surface_state_maximized));
            mir_wait_for_one(mir_surface_set_state(surf,
                                            mir_surface_state_restored));
            mir_wait_for_one(mir_surface_set_state(surf,
                                            mir_surface_state_fullscreen));
            mir_wait_for_one(mir_surface_set_state(surf,
                                            mir_surface_state_minimized));
        }
    }
    
    mtf::UsingStubClientPlatform using_stub_client_platform;

    static auto constexpr current_protocol_epoch = 0;
};

auto const* const protocol_version_override = "MIR_CLIENT_TEST_OVERRRIDE_PROTOCOL_VERSION";
}

using namespace testing;

TEST_F(ClientLibrary, client_library_connects_and_disconnects)
{
    MirWaitHandle* wh = mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__, connection_callback, this);
    EXPECT_THAT(wh, NotNull());
    mir_wait_for(wh);

    ASSERT_THAT(connection, NotNull());
    EXPECT_TRUE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), StrEq(""));

    mir_connection_release(connection);
}

TEST_F(ClientLibrary, synchronous_connection)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    EXPECT_THAT(connection, NotNull());
    EXPECT_TRUE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), StrEq(""));

    mir_connection_release(connection);
}

TEST_F(ClientLibrary, connects_when_protobuf_protocol_oldest_supported)
{
    std::ostringstream buffer;
    buffer << MIR_VERSION_NUMBER(current_protocol_epoch, MIR_CLIENT_MAJOR_VERSION, 0);

    add_to_environment(protocol_version_override, buffer.str().c_str());

    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    EXPECT_THAT(connection, NotNull());
    EXPECT_TRUE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), StrEq(""));

    mir_connection_release(connection);
}

TEST_F(ClientLibrary, reports_error_when_protobuf_protocol_obsolete)
{
    std::ostringstream buffer;
    buffer << MIR_VERSION_NUMBER(current_protocol_epoch, MIR_CLIENT_MAJOR_VERSION-1, 0);
    add_to_environment(protocol_version_override, buffer.str().c_str());

    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    EXPECT_THAT(connection, NotNull());
    EXPECT_FALSE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), HasSubstr("not accepted by server"));

    mir_connection_release(connection);
}

TEST_F(ClientLibrary, reports_error_when_protobuf_protocol_too_new)
{
    std::ostringstream buffer;
    buffer << MIR_VERSION_NUMBER(current_protocol_epoch, MIR_CLIENT_MAJOR_VERSION, MIR_CLIENT_MINOR_VERSION+1);
    add_to_environment(protocol_version_override, buffer.str().c_str());

    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    EXPECT_THAT(connection, NotNull());
    EXPECT_FALSE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), HasSubstr("not accepted by server"));

    mir_connection_release(connection);
}

TEST_F(ClientLibrary, reports_error_when_protobuf_protocol_epoch_too_new)
{
    std::ostringstream buffer;
    buffer << MIR_VERSION_NUMBER(current_protocol_epoch+1, MIR_CLIENT_MAJOR_VERSION, MIR_CLIENT_MINOR_VERSION);
    add_to_environment(protocol_version_override, buffer.str().c_str());

    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    EXPECT_THAT(connection, NotNull());
    EXPECT_FALSE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), HasSubstr("not accepted by server"));

    mir_connection_release(connection);
}

TEST_F(ClientLibrary, creates_surface)
{
    mir_wait_for(mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__, connection_callback, this));

    int request_width = 640, request_height = 480;
    MirPixelFormat request_format = mir_pixel_format_abgr_8888;
    MirBufferUsage request_buffer_usage = mir_buffer_usage_hardware;

    auto spec = mir_connection_create_spec_for_normal_surface(connection, request_width,
                                                              request_height, request_format);
    mir_surface_spec_set_buffer_usage(spec, request_buffer_usage);
    surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    ASSERT_THAT(surface, NotNull());
    EXPECT_TRUE(mir_surface_is_valid(surface));
    EXPECT_THAT(mir_surface_get_error_message(surface), StrEq(""));

    MirSurfaceParameters response_params;
    mir_surface_get_parameters(surface, &response_params);
    EXPECT_EQ(request_width, response_params.width);
    EXPECT_EQ(request_height, response_params.height);
    EXPECT_EQ(request_format, response_params.pixel_format);
    EXPECT_EQ(request_buffer_usage, response_params.buffer_usage);

    mir_wait_for(mir_surface_release( surface, release_surface_callback, this));
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_set_surface_state)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto const spec =
        mir_connection_create_spec_for_normal_surface(
            connection, 640, 480, mir_pixel_format_abgr_8888);

    mir_wait_for(mir_surface_create(spec, create_surface_callback, this));
    mir_surface_spec_release(spec);

    EXPECT_THAT(mir_surface_get_state(surface), Eq(mir_surface_state_restored));

    mir_wait_for(mir_surface_set_state(surface, mir_surface_state_fullscreen));
    EXPECT_THAT(mir_surface_get_state(surface), Eq(mir_surface_state_fullscreen));

    mir_wait_for(mir_surface_set_state(surface, static_cast<MirSurfaceState>(999)));
    EXPECT_THAT(mir_surface_get_state(surface), Eq(mir_surface_state_fullscreen));

    mir_wait_for(mir_surface_set_state(surface, mir_surface_state_horizmaximized));
    EXPECT_THAT(mir_surface_get_state(surface), Eq(mir_surface_state_horizmaximized));

    mir_wait_for(mir_surface_set_state(surface, static_cast<MirSurfaceState>(888)));
    EXPECT_THAT(mir_surface_get_state(surface), Eq(mir_surface_state_horizmaximized));

    // Stress-test synchronization logic with some flooding
    for (int i = 0; i < 100; i++)
    {
        mir_surface_set_state(surface, mir_surface_state_maximized);
        mir_surface_set_state(surface, mir_surface_state_restored);
        mir_wait_for(mir_surface_set_state(surface, mir_surface_state_fullscreen));
        ASSERT_THAT(mir_surface_get_state(surface), Eq(mir_surface_state_fullscreen));
    }

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_set_surface_min_width)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width = 640;
    int const height = 480;
    auto const format = mir_pixel_format_abgr_8888;
    auto const spec =
        mir_connection_create_spec_for_normal_surface(connection, width, height, format);

    int const min_width = 480;
    mir_surface_spec_set_min_width(spec, min_width);
    surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    EXPECT_THAT(surface, IsValid());

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_set_surface_min_height)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width = 640;
    int const height = 480;
    auto const format = mir_pixel_format_abgr_8888;
    auto const spec =
        mir_connection_create_spec_for_normal_surface(connection, width, height, format);

    int const min_height = 480;
    mir_surface_spec_set_min_height(spec, min_height);
    surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    EXPECT_THAT(surface, IsValid());

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_set_surface_max_width)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width = 640;
    int const height = 480;
    auto const format = mir_pixel_format_abgr_8888;
    auto const spec =
        mir_connection_create_spec_for_normal_surface(connection, width, height, format);

    int const max_width = 1024;
    mir_surface_spec_set_max_width(spec, max_width);
    surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    EXPECT_THAT(surface, IsValid());

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_set_surface_max_height)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width = 640;
    int const height = 480;
    auto const format = mir_pixel_format_abgr_8888;
    auto const spec =
        mir_connection_create_spec_for_normal_surface(connection, width, height, format);

    int const max_height = 1024;
    mir_surface_spec_set_max_height(spec, max_height);
    surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    EXPECT_THAT(surface, IsValid());

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, min_size_respected_when_placing_surface)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width = 6400;
    int const height = 4800;
    auto const format = mir_pixel_format_abgr_8888;
    auto const spec =
        mir_connection_create_spec_for_normal_surface(connection, width, height, format);

    int const min_width = 4800;
    int const min_height = 3200;

    mir_surface_spec_set_min_width(spec, min_width);
    mir_surface_spec_set_min_height(spec, min_height);
    surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    auto const buffer_stream = mir_surface_get_buffer_stream(surface);

    MirGraphicsRegion graphics_region;
    mir_buffer_stream_get_graphics_region(buffer_stream, &graphics_region);
    EXPECT_THAT(graphics_region.width, Ge(min_width));
    EXPECT_THAT(graphics_region.height, Ge(min_height));

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, receives_surface_dpi_value)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto const spec =
        mir_connection_create_spec_for_normal_surface(
            connection, 640, 480, mir_pixel_format_abgr_8888);

    surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    // Expect zero (not wired up to detect the physical display yet)
    EXPECT_THAT(mir_surface_get_dpi(surface), Eq(0));

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

#ifdef MESA_KMS
TEST_F(ClientLibrary, surface_scanout_flag_toggles)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto const spec =
        mir_connection_create_spec_for_normal_surface(
            connection, 1280, 1024, mir_pixel_format_abgr_8888);
    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);

    surface = mir_surface_create_sync(spec);

    MirNativeBuffer *native;
    auto bs = mir_surface_get_buffer_stream(surface);
    mir_buffer_stream_get_current_buffer(bs, &native);
    EXPECT_TRUE(native->flags & mir_buffer_flag_can_scanout);
    mir_buffer_stream_swap_buffers_sync(bs);
    EXPECT_TRUE(native->flags & mir_buffer_flag_can_scanout);
    mir_surface_release_sync(surface);

    mir_surface_spec_set_width(spec, 100);
    mir_surface_spec_set_height(spec, 100);

    surface = mir_surface_create_sync(spec);
    bs = mir_surface_get_buffer_stream(surface);
    mir_buffer_stream_get_current_buffer(bs, &native);
    EXPECT_FALSE(native->flags & mir_buffer_flag_can_scanout);
    mir_buffer_stream_swap_buffers_sync(bs);
    EXPECT_FALSE(native->flags & mir_buffer_flag_can_scanout);
    mir_surface_release_sync(surface);


    mir_surface_spec_set_width(spec, 800);
    mir_surface_spec_set_height(spec, 600);
    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_software);

    surface = mir_surface_create_sync(spec);
    bs = mir_surface_get_buffer_stream(surface);
    mir_buffer_stream_get_current_buffer(bs, &native);
    EXPECT_FALSE(native->flags & mir_buffer_flag_can_scanout);
    mir_buffer_stream_swap_buffers_sync(bs);
    EXPECT_FALSE(native->flags & mir_buffer_flag_can_scanout);
    mir_surface_release_sync(surface);

    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);

    surface = mir_surface_create_sync(spec);
    bs = mir_surface_get_buffer_stream(surface);
    mir_buffer_stream_get_current_buffer(bs, &native);
    EXPECT_TRUE(native->flags & mir_buffer_flag_can_scanout);
    mir_buffer_stream_swap_buffers_sync(bs);
    EXPECT_TRUE(native->flags & mir_buffer_flag_can_scanout);
    mir_surface_release_sync(surface);

    mir_surface_spec_release(spec);
    mir_connection_release(connection);
}
#endif

#if defined(ANDROID) || defined(MESA_X11)
// Mir's Android test infrastructure isn't quite ready for this yet.
TEST_F(ClientLibrary, DISABLED_gets_buffer_dimensions)
#else
TEST_F(ClientLibrary, gets_buffer_dimensions)
#endif
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto const spec =
        mir_connection_create_spec_for_normal_surface(
            connection, 0, 0, mir_pixel_format_abgr_8888);
    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);

    struct {int width, height;} const sizes[] =
    {
        {12, 34},
        {56, 78},
        {90, 21},
    };

    for (auto const& size : sizes)
    {
        mir_surface_spec_set_width(spec, size.width);
        mir_surface_spec_set_height(spec, size.height);

        surface = mir_surface_create_sync(spec);
        auto bs = mir_surface_get_buffer_stream(surface);

        MirNativeBuffer *native = NULL;
        mir_buffer_stream_get_current_buffer(bs, &native);
        ASSERT_THAT(native, NotNull());
        EXPECT_THAT(native->width, Eq(size.width));
        ASSERT_THAT(native->height, Eq(size.height));

        mir_buffer_stream_swap_buffers_sync(bs);
        mir_buffer_stream_get_current_buffer(bs, &native);
        ASSERT_THAT(native, NotNull());
        EXPECT_THAT(native->width, Eq(size.width));
        ASSERT_THAT(native->height, Eq(size.height));

        mir_surface_release_sync(surface);
    }

    mir_surface_spec_release(spec);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, creates_multiple_surfaces)
{
    int const n_surfaces = 13;
    size_t old_surface_count = 0;

    mir_wait_for(mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__, connection_callback, this));

    auto const spec =
        mir_connection_create_spec_for_normal_surface(
            connection, 640, 480, mir_pixel_format_abgr_8888);

    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
    for (int i = 0; i != n_surfaces; ++i)
    {
        old_surface_count = current_surface_count();

        mir_wait_for(mir_surface_create(spec, create_surface_callback, this));

        ASSERT_THAT(current_surface_count(), Eq(old_surface_count + 1));
    }
    for (int i = 0; i != n_surfaces; ++i)
    {
        old_surface_count = current_surface_count();

        ASSERT_THAT(old_surface_count, Ne(0u));

        MirSurface * surface = any_surface();
        mir_wait_for(mir_surface_release( surface, release_surface_callback, this));

        ASSERT_THAT(current_surface_count(), Eq(old_surface_count - 1));
    }

    mir_surface_spec_release(spec);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, client_library_accesses_and_advances_buffers)
{
    mir_wait_for(mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__, connection_callback, this));

    surface = mtf::make_any_surface(connection);

    buffers = 0;
    mir_wait_for(mir_buffer_stream_swap_buffers(mir_surface_get_buffer_stream(surface), next_buffer_callback, this));
    EXPECT_THAT(buffers, Eq(1));

    mir_wait_for(mir_surface_release(surface, release_surface_callback, this));

    ASSERT_THAT(surface, IsNull());

    mir_connection_release(connection);
}

TEST_F(ClientLibrary, fully_synchronous_client)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    surface = mtf::make_any_surface(connection);

    mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
    EXPECT_TRUE(mir_surface_is_valid(surface));
    EXPECT_STREQ(mir_surface_get_error_message(surface), "");

    mir_surface_release_sync(surface);

    EXPECT_TRUE(mir_connection_is_valid(connection));
    EXPECT_STREQ("", mir_connection_get_error_message(connection));
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, highly_threaded_client)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    surface = mtf::make_any_surface(connection);

    std::thread a(nosey_thread, surface);
    std::thread b(nosey_thread, surface);
    std::thread c(nosey_thread, surface);

    a.join();
    b.join();
    c.join();

    EXPECT_THAT(mir_surface_get_state(surface), Eq(mir_surface_state_minimized));

    mir_surface_release_sync(surface);

    EXPECT_TRUE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), StrEq(""));
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, accesses_platform_package)
{
    using namespace testing;
    mir_wait_for(mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__, connection_callback, this));

    MirPlatformPackage platform_package;
    ::memset(&platform_package, -1, sizeof(platform_package));

    mir_connection_get_platform(connection, &platform_package);
    EXPECT_THAT(platform_package, mtf::IsStubPlatformPackage());

    mir_connection_release(connection);
}

TEST_F(ClientLibrary, accesses_display_info)
{
    mir_wait_for(mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__, connection_callback, this));

    auto configuration = mir_connection_create_display_config(connection);
    ASSERT_THAT(configuration, NotNull());
    ASSERT_GT(configuration->num_outputs, 0u);
    ASSERT_THAT(configuration->outputs, NotNull());
    for (auto i=0u; i < configuration->num_outputs; i++)
    {
        MirDisplayOutput* disp = &configuration->outputs[i];
        ASSERT_THAT(disp, NotNull());
        EXPECT_GE(disp->num_modes, disp->current_mode);
        EXPECT_GE(disp->num_output_formats, disp->current_format);
    }

    mir_display_config_destroy(configuration);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, MultiSurfaceClientTracksBufferFdsCorrectly)
{
    mir_wait_for(mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__, connection_callback, this));

    auto const surf_one = mtf::make_any_surface(connection);
    auto const surf_two = mtf::make_any_surface(connection);

    ASSERT_THAT(surf_one, NotNull());
    ASSERT_THAT(surf_two, NotNull());

    buffers = 0;

    while (buffers < 1024)
    {
        mir_buffer_stream_swap_buffers_sync(
            mir_surface_get_buffer_stream(surf_one));
        mir_buffer_stream_swap_buffers_sync(
            mir_surface_get_buffer_stream(surf_two));

        buffers++;
    }

    /* We should not have any stray fds hanging around.
       Test this by trying to open a new one */
    int canary_fd;
    canary_fd = open("/dev/null", O_RDONLY);

    ASSERT_THAT(canary_fd, Gt(0)) << "Failed to open canary file descriptor: "<< strerror(errno);
    EXPECT_THAT(canary_fd, Lt(1024));

    close(canary_fd);

    mir_wait_for(mir_surface_release(surf_one, release_surface_callback, this));
    mir_wait_for(mir_surface_release(surf_two, release_surface_callback, this));

    ASSERT_THAT(current_surface_count(), testing::Eq(0));

    mir_connection_release(connection);
}

/* TODO: Our stub platform support is a bit terrible.
 *
 * These acceptance tests accidentally work on mesa because the mesa client
 * platform doesn't validate any of its input and we don't touch anything that requires
 * syscalls.
 *
 * The Android client platform *does* care about its input, and so the fact that it's
 * trying to marshall stub buffers causes crashes.
 */

#if defined(ANDROID) || defined(MESA_X11)
TEST_F(ClientLibrary, DISABLED_create_simple_normal_surface_from_spec)
#else
TEST_F(ClientLibrary, create_simple_normal_surface_from_spec)
#endif
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width{800}, height{600};
    MirPixelFormat const format{mir_pixel_format_bgr_888};
    auto surface_spec = mir_connection_create_spec_for_normal_surface(connection,
                                                                      width, height,
                                                                      format);

    auto surface = mir_surface_create_sync(surface_spec);
    mir_surface_spec_release(surface_spec);

    EXPECT_THAT(surface, IsValid());

    MirNativeBuffer* native_buffer;
    mir_buffer_stream_get_current_buffer(
        mir_surface_get_buffer_stream(surface), &native_buffer);

    EXPECT_THAT(native_buffer->width, Eq(width));
    EXPECT_THAT(native_buffer->height, Eq(height));
    EXPECT_THAT(mir_surface_get_type(surface), Eq(mir_surface_type_normal));

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

#if defined(ANDROID) || defined(MESA_X11)
TEST_F(ClientLibrary, DISABLED_create_simple_normal_surface_from_spec_async)
#else
TEST_F(ClientLibrary, create_simple_normal_surface_from_spec_async)
#endif
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width{800}, height{600};
    MirPixelFormat const format{mir_pixel_format_xbgr_8888};
    auto surface_spec = mir_connection_create_spec_for_normal_surface(connection,
                                                                      width, height,
                                                                      format);

    mir_wait_for(mir_surface_create(surface_spec, create_surface_callback, this));
    mir_surface_spec_release(surface_spec);

    EXPECT_THAT(surface, IsValid());

    MirNativeBuffer* native_buffer;
    mir_buffer_stream_get_current_buffer(
        mir_surface_get_buffer_stream(surface), &native_buffer);

    EXPECT_THAT(native_buffer->width, Eq(width));
    EXPECT_THAT(native_buffer->height, Eq(height));
    EXPECT_THAT(mir_surface_get_type(surface), Eq(mir_surface_type_normal));

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

#ifndef ANDROID
TEST_F(ClientLibrary, can_specify_all_normal_surface_parameters_from_spec)
#else
TEST_F(ClientLibrary, DISABLED_can_specify_all_normal_surface_parameters_from_spec)
#endif
{
    using namespace testing;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto surface_spec = mir_connection_create_spec_for_normal_surface(connection,
                                                                      800, 600,
                                                                      mir_pixel_format_bgr_888);

    char const* name = "The magnificent Dandy Warhols";
    mir_surface_spec_set_name(surface_spec, name);

    int const width{999}, height{555};
    mir_surface_spec_set_width(surface_spec, width);
    mir_surface_spec_set_height(surface_spec, height);

    MirPixelFormat const pixel_format{mir_pixel_format_argb_8888};
    mir_surface_spec_set_pixel_format(surface_spec, pixel_format);

    MirBufferUsage const buffer_usage{mir_buffer_usage_hardware};
    mir_surface_spec_set_buffer_usage(surface_spec, buffer_usage);

    auto surface = mir_surface_create_sync(surface_spec);
    mir_surface_spec_release(surface_spec);

    EXPECT_THAT(surface, IsValid());

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

#if defined(ANDROID) || defined(MESA_X11)
TEST_F(ClientLibrary, DISABLED_set_fullscreen_on_output_makes_fullscreen_surface)
#else
TEST_F(ClientLibrary, set_fullscreen_on_output_makes_fullscreen_surface)
#endif
{
    using namespace testing;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto surface_spec = mir_connection_create_spec_for_normal_surface(connection,
                                                                      780, 555,
                                                                      mir_pixel_format_xbgr_8888);

    // We need to specify a valid output id, so we need to find which ones are valid...
    auto configuration = mir_connection_create_display_config(connection);
    ASSERT_THAT(configuration->num_outputs, Ge(1));

    auto const requested_output = configuration->outputs[0];

    mir_surface_spec_set_fullscreen_on_output(surface_spec, requested_output.output_id);

    auto surface = mir_surface_create_sync(surface_spec);
    mir_surface_spec_release(surface_spec);

    EXPECT_THAT(surface, IsValid());

    MirNativeBuffer* native_buffer;
    mir_buffer_stream_get_current_buffer(
        mir_surface_get_buffer_stream(surface), &native_buffer);

    EXPECT_THAT(native_buffer->width,
                Eq(requested_output.modes[requested_output.current_mode].horizontal_resolution));
    EXPECT_THAT(native_buffer->height,
                Eq(requested_output.modes[requested_output.current_mode].vertical_resolution));

// TODO: This is racy. Fix in subsequent "send all the things on construction" branch
//    EXPECT_THAT(mir_surface_get_state(surface), Eq(mir_surface_state_fullscreen));

    mir_surface_release_sync(surface);
    mir_display_config_destroy(configuration);
    mir_connection_release(connection);
}

/*
 * We don't (yet) use a stub client platform, so can't rely on its behaviour
 * in these tests.
 *
 * At the moment, enabling them will either spuriously pass (hardware buffer, mesa)
 * or crash (everything else).
 */
TEST_F(ClientLibrary, DISABLED_can_create_buffer_usage_hardware_surface)
{
    using namespace testing;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto surface_spec = mir_connection_create_spec_for_normal_surface(connection,
                                                                      800, 600,
                                                                      mir_pixel_format_bgr_888);

    MirBufferUsage const buffer_usage{mir_buffer_usage_hardware};
    mir_surface_spec_set_buffer_usage(surface_spec, buffer_usage);

    auto surface = mir_surface_create_sync(surface_spec);
    mir_surface_spec_release(surface_spec);

    EXPECT_THAT(surface, IsValid());

    MirNativeBuffer* native_buffer;
    // We use the fact that our stub client platform returns NULL if asked for a native
    // buffer on a surface with mir_buffer_usage_software set.
    mir_buffer_stream_get_current_buffer(
        mir_surface_get_buffer_stream(surface), &native_buffer);

    EXPECT_THAT(native_buffer, Not(Eq(nullptr)));

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, DISABLED_can_create_buffer_usage_software_surface)
{
    using namespace testing;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto surface_spec = mir_connection_create_spec_for_normal_surface(connection,
                                                                      800, 600,
                                                                      mir_pixel_format_bgr_888);

    MirBufferUsage const buffer_usage{mir_buffer_usage_software};
    mir_surface_spec_set_buffer_usage(surface_spec, buffer_usage);

    auto surface = mir_surface_create_sync(surface_spec);
    mir_surface_spec_release(surface_spec);

    EXPECT_THAT(surface, IsValid());

    MirGraphicsRegion graphics_region;
    // We use the fact that our stub client platform returns a NULL vaddr if
    // asked to map a hardware buffer.
    mir_buffer_stream_get_graphics_region(
        mir_surface_get_buffer_stream(surface), &graphics_region);

    EXPECT_THAT(graphics_region.vaddr, Not(Eq(nullptr)));

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

namespace
{
void dummy_event_handler_one(MirSurface*, MirEvent const*, void*)
{
}

void dummy_event_handler_two(MirSurface*, MirEvent const*, void*)
{
}
}

/*
 * Regression test for LP: 1438160
 */
TEST_F(ClientLibrary, can_change_event_delegate)
{
    using namespace testing;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto surface_spec = mir_connection_create_spec_for_normal_surface(connection,
                                                                      800, 600,
                                                                      mir_pixel_format_argb_8888);
    auto surface = mir_surface_create_sync(surface_spec);
    mir_surface_spec_release(surface_spec);

    ASSERT_THAT(surface, IsValid());

    /* TODO: When provide-event-fd lands, change this into a better test that actually
     * tests that the correct event handler is called.
     *
     * Without manual dispatch, it's racy to try and test that.
     */
    mir_surface_set_event_handler(surface, &dummy_event_handler_one, nullptr);
    mir_surface_set_event_handler(surface, &dummy_event_handler_two, nullptr);

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_get_persistent_surface_id)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto surface_spec = mir_connection_create_spec_for_normal_surface(connection,
                                                                      800, 600,
                                                                      mir_pixel_format_argb_8888);
    auto surface = mir_surface_create_sync(surface_spec);
    mir_surface_spec_release(surface_spec);

    ASSERT_THAT(surface, IsValid());

    auto surface_id = mir_surface_request_persistent_id_sync(surface);
    EXPECT_TRUE(mir_persistent_id_is_valid(surface_id));

    mir_surface_release_sync(surface);
    mir_persistent_id_release(surface_id);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, input_method_can_specify_foreign_surface_id)
{
    auto first_client = mir_connect_sync(new_connection().c_str(), "Regular Client");

    auto surface_spec = mir_connection_create_spec_for_normal_surface(first_client,
                                                                      800, 600,
                                                                      mir_pixel_format_argb_8888);
    auto main_surface = mir_surface_create_sync(surface_spec);
    mir_surface_spec_release(surface_spec);

    ASSERT_THAT(main_surface, IsValid());

    auto main_surface_id = mir_surface_request_persistent_id_sync(main_surface);
    ASSERT_TRUE(mir_persistent_id_is_valid(main_surface_id));

    // Serialise & deserialise the ID
    auto im_parent_id = mir_persistent_id_from_string(mir_persistent_id_as_string(main_surface_id));

    auto im_client = mir_connect_sync(new_connection().c_str(), "IM Client");
    surface_spec = mir_connection_create_spec_for_input_method(im_client,
                                                               200, 20,
                                                               mir_pixel_format_argb_8888);
    MirRectangle attachment_rect {
        200,
        200,
        10,
        10
    };
    mir_surface_spec_attach_to_foreign_parent(surface_spec,
                                              im_parent_id,
                                              &attachment_rect,
                                              mir_edge_attachment_any);
    auto im_surface = mir_surface_create_sync(surface_spec);

    EXPECT_THAT(im_surface, IsValid());

    mir_surface_spec_release(surface_spec);
    mir_persistent_id_release(main_surface_id);
    mir_persistent_id_release(im_parent_id);
    mir_surface_release_sync(main_surface);
    mir_surface_release_sync(im_surface);
    mir_connection_release(first_client);
    mir_connection_release(im_client);
}
