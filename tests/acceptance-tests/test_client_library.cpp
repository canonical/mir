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
#include "mir_test/validity_matchers.h"

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
    int buffers = 0;

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

    static void next_buffer_callback(MirSurface* surface, void* context)
    {
        ClientLibrary* config = reinterpret_cast<ClientLibrary*>(context);
        config->next_buffer(surface);
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

    virtual void next_buffer(MirSurface*)
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

    MirEvent last_event{};
    MirSurface* last_event_surface = nullptr;

    static void event_callback(MirSurface* surface, MirEvent const* event,
                               void* ctx)
    {
        ClientLibrary* self = static_cast<ClientLibrary*>(ctx);
        self->last_event = *event;
        self->last_event_surface = surface;
    }

    static void nosey_thread(MirSurface *surf)
    {
        for (int i = 0; i < 10; i++)
        {
            mir_wait_for_one(mir_surface_set_state(surf,
                                            mir_surface_state_maximized));
            mir_wait_for_one(mir_surface_set_type(surf,
                                            mir_surface_type_normal));
            mir_wait_for_one(mir_surface_set_state(surf,
                                            mir_surface_state_restored));
            mir_wait_for_one(mir_surface_set_type(surf,
                                            mir_surface_type_utility));
            mir_wait_for_one(mir_surface_set_state(surf,
                                            mir_surface_state_fullscreen));
            mir_wait_for_one(mir_surface_set_type(surf,
                                            mir_surface_type_dialog));
            mir_wait_for_one(mir_surface_set_state(surf,
                                            mir_surface_state_minimized));
        }
    }
};
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

TEST_F(ClientLibrary, can_set_surface_types)
{
    mir_wait_for(mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__, connection_callback, this));

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    mir_wait_for(mir_connection_create_surface(connection, &request_params, create_surface_callback, this));

    EXPECT_THAT(mir_surface_get_type(surface), Eq(mir_surface_type_normal));

    mir_wait_for(mir_surface_set_type(surface, mir_surface_type_freestyle));
    EXPECT_THAT(mir_surface_get_type(surface), Eq(mir_surface_type_freestyle));

    mir_wait_for(mir_surface_set_type(surface, static_cast<MirSurfaceType>(999)));
    EXPECT_THAT(mir_surface_get_type(surface), Eq(mir_surface_type_freestyle));

    mir_wait_for(mir_surface_set_type(surface, mir_surface_type_dialog));
    EXPECT_THAT(mir_surface_get_type(surface), Eq(mir_surface_type_dialog));

    mir_wait_for(mir_surface_set_type(surface, static_cast<MirSurfaceType>(888)));
    EXPECT_THAT(mir_surface_get_type(surface), Eq(mir_surface_type_dialog));

    // Stress-test synchronization logic with some flooding
    for (int i = 0; i < 100; i++)
    {
        mir_surface_set_type(surface, mir_surface_type_normal);
        mir_surface_set_type(surface, mir_surface_type_utility);
        mir_surface_set_type(surface, mir_surface_type_dialog);
        mir_surface_set_type(surface, mir_surface_type_gloss);
        mir_surface_set_type(surface, mir_surface_type_freestyle);
        mir_wait_for(mir_surface_set_type(surface, mir_surface_type_menu));
        ASSERT_THAT(mir_surface_get_type(surface), Eq(mir_surface_type_menu));
    }

    mir_wait_for(mir_surface_release(surface, release_surface_callback, this));
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_set_surface_state)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    surface = mir_connection_create_surface_sync(connection, &request_params);

    EXPECT_THAT(mir_surface_get_state(surface), Eq(mir_surface_state_restored));

    mir_wait_for(mir_surface_set_state(surface, mir_surface_state_fullscreen));
    EXPECT_THAT(mir_surface_get_state(surface), Eq(mir_surface_state_fullscreen));

    mir_wait_for(mir_surface_set_state(surface, static_cast<MirSurfaceState>(999)));
    EXPECT_THAT(mir_surface_get_state(surface), Eq(mir_surface_state_fullscreen));

    mir_wait_for(mir_surface_set_state(surface, mir_surface_state_minimized));
    EXPECT_THAT(mir_surface_get_state(surface), Eq(mir_surface_state_minimized));

    mir_wait_for(mir_surface_set_state(surface, static_cast<MirSurfaceState>(888)));
    EXPECT_THAT(mir_surface_get_state(surface), Eq(mir_surface_state_minimized));

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

TEST_F(ClientLibrary, receives_surface_dpi_value)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    surface = mir_connection_create_surface_sync(connection, &request_params);

    // Expect zero (not wired up to detect the physical display yet)
    EXPECT_THAT(mir_surface_get_dpi(surface), Eq(0));

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

#ifndef ANDROID
TEST_F(ClientLibrary, surface_scanout_flag_toggles)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    MirSurfaceParameters parm =
    {
        __PRETTY_FUNCTION__,
        1280, 1024,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    surface = mir_connection_create_surface_sync(connection, &parm);

    MirNativeBuffer *native;
    mir_surface_get_current_buffer(surface, &native);
    EXPECT_TRUE(native->flags & mir_buffer_flag_can_scanout);
    mir_surface_swap_buffers_sync(surface);
    EXPECT_TRUE(native->flags & mir_buffer_flag_can_scanout);
    mir_surface_release_sync(surface);

    parm.width = 100;
    parm.height = 100;

    surface = mir_connection_create_surface_sync(connection, &parm);
    mir_surface_get_current_buffer(surface, &native);
    EXPECT_FALSE(native->flags & mir_buffer_flag_can_scanout);
    mir_surface_swap_buffers_sync(surface);
    EXPECT_FALSE(native->flags & mir_buffer_flag_can_scanout);
    mir_surface_release_sync(surface);

    parm.width = 800;
    parm.height = 600;
    parm.buffer_usage = mir_buffer_usage_software;

    surface = mir_connection_create_surface_sync(connection, &parm);
    mir_surface_get_current_buffer(surface, &native);
    EXPECT_FALSE(native->flags & mir_buffer_flag_can_scanout);
    mir_surface_swap_buffers_sync(surface);
    EXPECT_FALSE(native->flags & mir_buffer_flag_can_scanout);
    mir_surface_release_sync(surface);

    parm.buffer_usage = mir_buffer_usage_hardware;

    surface = mir_connection_create_surface_sync(connection, &parm);
    mir_surface_get_current_buffer(surface, &native);
    EXPECT_TRUE(native->flags & mir_buffer_flag_can_scanout);
    mir_surface_swap_buffers_sync(surface);
    EXPECT_TRUE(native->flags & mir_buffer_flag_can_scanout);
    mir_surface_release_sync(surface);

    mir_connection_release(connection);
}
#endif

#ifdef ANDROID
// Mir's Android test infrastructure isn't quite ready for this yet.
TEST_F(ClientLibrary, DISABLED_gets_buffer_dimensions)
#else
TEST_F(ClientLibrary, gets_buffer_dimensions)
#endif
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    MirSurfaceParameters parm =
    {
        __PRETTY_FUNCTION__,
        0, 0,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    struct {int width, height;} const sizes[] =
    {
        {12, 34},
        {56, 78},
        {90, 21},
    };

    for (auto const& size : sizes)
    {
        parm.width = size.width;
        parm.height = size.height;

        surface = mir_connection_create_surface_sync(connection, &parm);

        MirNativeBuffer *native = NULL;
        mir_surface_get_current_buffer(surface, &native);
        ASSERT_THAT(native, NotNull());
        EXPECT_THAT(native->width, Eq(parm.width));
        ASSERT_THAT(native->height, Eq(parm.height));

        mir_surface_swap_buffers_sync(surface);
        mir_surface_get_current_buffer(surface, &native);
        ASSERT_THAT(native, NotNull());
        EXPECT_THAT(native->width, Eq(parm.width));
        ASSERT_THAT(native->height, Eq(parm.height));

        mir_surface_release_sync(surface);
    }

    mir_connection_release(connection);
}

TEST_F(ClientLibrary, creates_multiple_surfaces)
{
    int const n_surfaces = 13;
    size_t old_surface_count = 0;

    mir_wait_for(mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__, connection_callback, this));

    for (int i = 0; i != n_surfaces; ++i)
    {
        old_surface_count = current_surface_count();

        MirSurfaceParameters const request_params =
        {
            __PRETTY_FUNCTION__,
            640, 480,
            mir_pixel_format_abgr_8888,
            mir_buffer_usage_hardware,
            mir_display_output_id_invalid
        };

        mir_wait_for(mir_connection_create_surface(connection, &request_params, create_surface_callback, this));

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

    mir_connection_release(connection);
}

TEST_F(ClientLibrary, client_library_accesses_and_advances_buffers)
{
    mir_wait_for(mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__, connection_callback, this));

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    mir_wait_for(mir_connection_create_surface(connection, &request_params, create_surface_callback, this));

    buffers = 0;
    mir_wait_for(mir_surface_swap_buffers(surface, next_buffer_callback, this));
    EXPECT_THAT(buffers, Eq(1));

    mir_wait_for(mir_surface_release(surface, release_surface_callback, this));

    ASSERT_THAT(surface, IsNull());

    mir_connection_release(connection);
}

TEST_F(ClientLibrary, fully_synchronous_client)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_software,
        mir_display_output_id_invalid
    };

    surface = mir_connection_create_surface_sync(connection, &request_params);

    mir_surface_swap_buffers_sync(surface);
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

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_software,
        mir_display_output_id_invalid
    };

    surface = mir_connection_create_surface_sync(connection, &request_params);

    std::thread a(nosey_thread, surface);
    std::thread b(nosey_thread, surface);
    std::thread c(nosey_thread, surface);

    a.join();
    b.join();
    c.join();

    EXPECT_THAT(mir_surface_get_type(surface), Eq(mir_surface_type_dialog));
    EXPECT_THAT(mir_surface_get_type(surface), Eq(mir_surface_state_minimized));

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

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    MirSurface* surf_one = mir_connection_create_surface_sync(connection, &request_params);
    MirSurface* surf_two = mir_connection_create_surface_sync(connection, &request_params);

    ASSERT_THAT(surf_one, NotNull());
    ASSERT_THAT(surf_two, NotNull());

    buffers = 0;

    while (buffers < 1024)
    {
        mir_surface_swap_buffers_sync(surf_one);
        mir_surface_swap_buffers_sync(surf_two);

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

#ifndef ANDROID
TEST_F(ClientLibrary, create_simple_normal_surface_from_spec)
#else
TEST_F(ClientLibrary, DISABLED_create_simple_normal_surface_from_spec)
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
    mir_surface_get_current_buffer(surface, &native_buffer);

    EXPECT_THAT(native_buffer->width, Eq(width));
    EXPECT_THAT(native_buffer->height, Eq(height));
    EXPECT_THAT(mir_surface_get_type(surface), Eq(mir_surface_type_normal));

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

#ifndef ANDROID
TEST_F(ClientLibrary, create_simple_normal_surface_from_spec_async)
#else
TEST_F(ClientLibrary, DISABLED_create_simple_normal_surface_from_spec_async)
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
    mir_surface_get_current_buffer(surface, &native_buffer);

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
    EXPECT_TRUE(mir_surface_spec_set_name(surface_spec, name));

    int const width{999}, height{555};
    EXPECT_TRUE(mir_surface_spec_set_width(surface_spec, width));
    EXPECT_TRUE(mir_surface_spec_set_height(surface_spec, height));

    MirPixelFormat const pixel_format{mir_pixel_format_argb_8888};
    EXPECT_TRUE(mir_surface_spec_set_pixel_format(surface_spec, pixel_format));

    MirBufferUsage const buffer_usage{mir_buffer_usage_hardware};
    EXPECT_TRUE(mir_surface_spec_set_buffer_usage(surface_spec, buffer_usage));

    auto surface = mir_surface_create_sync(surface_spec);
    mir_surface_spec_release(surface_spec);

    EXPECT_THAT(surface, IsValid());

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

#ifndef ANDROID
TEST_F(ClientLibrary, set_fullscreen_on_output_makes_fullscreen_surface)
#else
TEST_F(ClientLibrary, DISABLED_set_fullscreen_on_output_makes_fullscreen_surface)
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
    mir_surface_get_current_buffer(surface, &native_buffer);

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
    EXPECT_TRUE(mir_surface_spec_set_buffer_usage(surface_spec, buffer_usage));

    auto surface = mir_surface_create_sync(surface_spec);
    mir_surface_spec_release(surface_spec);

    EXPECT_THAT(surface, IsValid());

    MirNativeBuffer* native_buffer;
    // We use the fact that our stub client platform returns NULL if asked for a native
    // buffer on a surface with mir_buffer_usage_software set.
    mir_surface_get_current_buffer(surface, &native_buffer);

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
    EXPECT_TRUE(mir_surface_spec_set_buffer_usage(surface_spec, buffer_usage));

    auto surface = mir_surface_create_sync(surface_spec);
    mir_surface_spec_release(surface_spec);

    EXPECT_THAT(surface, IsValid());

    MirGraphicsRegion graphics_region;
    // We use the fact that our stub client platform returns a NULL vaddr if
    // asked to map a hardware buffer.
    mir_surface_get_graphics_region(surface, &graphics_region);

    EXPECT_THAT(graphics_region.vaddr, Not(Eq(nullptr)));

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}
