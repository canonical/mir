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

#include "src/client/client_buffer.h"

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
    mtf::UsingStubClientPlatform using_stub_client_platform;

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

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    mir_wait_for(mir_connection_create_surface(connection, &request_params, create_surface_callback, this));

    ASSERT_THAT(surface, NotNull());
    EXPECT_TRUE(mir_surface_is_valid(surface));
    EXPECT_THAT(mir_surface_get_error_message(surface), StrEq(""));

    MirSurfaceParameters response_params;
    mir_surface_get_parameters(surface, &response_params);
    EXPECT_EQ(request_params.width, response_params.width);
    EXPECT_EQ(request_params.height, response_params.height);
    EXPECT_EQ(request_params.pixel_format, response_params.pixel_format);
    EXPECT_EQ(request_params.buffer_usage, response_params.buffer_usage);

    mir_wait_for(mir_surface_release( surface, release_surface_callback, this));

    ASSERT_THAT(surface, IsNull());

    surface = mir_connection_create_surface_sync(connection, &request_params);

    ASSERT_THAT(surface, NotNull());
    EXPECT_TRUE(mir_surface_is_valid(surface));
    EXPECT_THAT(mir_surface_get_error_message(surface), StrEq(""));

    mir_surface_get_parameters(surface, &response_params);
    EXPECT_THAT(response_params.width, Eq(request_params.width));
    EXPECT_THAT(response_params.height, Eq(request_params.height));
    EXPECT_THAT(response_params.pixel_format, Eq(request_params.pixel_format));
    EXPECT_THAT(response_params.buffer_usage, Eq(request_params.buffer_usage));

    mir_surface_release_sync(surface);
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
        mir_surface_set_type(surface, mir_surface_type_overlay);
        mir_surface_set_type(surface, mir_surface_type_freestyle);
        mir_wait_for(mir_surface_set_type(surface, mir_surface_type_popover));
        ASSERT_THAT(mir_surface_get_type(surface), Eq(mir_surface_type_popover));
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
    mir_wait_for(mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__, connection_callback, this));

    MirPlatformPackage platform_package;
    platform_package.data_items = -1;
    platform_package.fd_items = -1;

    mir_connection_get_platform(connection, &platform_package);
    EXPECT_GE(0, platform_package.data_items);
    EXPECT_GE(0, platform_package.fd_items);

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

TEST_F(ClientLibrary, connect_errors_handled)
{
    mir_wait_for(mir_connect("garbage", __PRETTY_FUNCTION__, connection_callback, this));
    ASSERT_THAT(connection, NotNull());

    char const* error = mir_connection_get_error_message(connection);

    if (std::strcmp("connect: No such file or directory", error) &&
        std::strcmp("Can't find MIR server", error) &&
        !std::strstr(error, "Failed to connect to server socket"))
    {
        FAIL() << error;
    }
}

TEST_F(ClientLibrary, connect_errors_dont_blow_up)
{
    mir_wait_for(mir_connect("garbage", __PRETTY_FUNCTION__, connection_callback, this));

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    mir_wait_for(mir_connection_create_surface(connection, &request_params, create_surface_callback, this));
// TODO surface_create needs to fail safe too. After that is done we should add the following:
// TODO    mir_wait_for(mir_surface_swap_buffers(surface, next_buffer_callback, this));
// TODO    mir_wait_for(mir_surface_release( surface, release_surface_callback, this));

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
