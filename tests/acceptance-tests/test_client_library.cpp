/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
#include "mir_test_framework/stub_platform_helpers.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/validity_matchers.h"
#include "src/include/common/mir/protobuf/protocol_version.h"

#include "mir_protobuf.pb.h"

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
namespace mtf = mir_test_framework;
namespace
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// Assert our MirSurfaceType is 1to1 to MirWindowType
static_assert(
    static_cast<int32_t>(mir_surface_type_normal) ==
    static_cast<int32_t>(mir_window_type_normal),
    "mir_surface_type_normal != mir_window_type_normal");
static_assert(
    static_cast<int32_t>(mir_surface_type_utility) ==
    static_cast<int32_t>(mir_window_type_utility),
    "mir_surface_type_utility != mir_window_type_utility");
static_assert(
    static_cast<int32_t>(mir_surface_type_dialog) ==
    static_cast<int32_t>(mir_window_type_dialog),
    "mir_surface_type_dialog != mir_window_type_dialog");
static_assert(
    static_cast<int32_t>(mir_surface_type_gloss) ==
    static_cast<int32_t>(mir_window_type_gloss),
    "mir_surface_type_gloss != mir_window_type_gloss");
static_assert(
    static_cast<int32_t>(mir_surface_type_freestyle) ==
    static_cast<int32_t>(mir_window_type_freestyle),
    "mir_surface_type_freestyle != mir_window_type_freestyle");
static_assert(
    static_cast<int32_t>(mir_surface_type_popover) ==
    static_cast<int32_t>(mir_window_type_menu),
    "mir_surface_type_popover != mir_window_type_menu");
static_assert(
    static_cast<int32_t>(mir_surface_type_menu) ==
    static_cast<int32_t>(mir_window_type_menu),
    "mir_surface_type_menu != mir_window_type_menu");
static_assert(
    static_cast<int32_t>(mir_surface_type_inputmethod) ==
    static_cast<int32_t>(mir_window_type_inputmethod),
    "mir_surface_type_inputmethod != mir_window_type_inputmethod");
static_assert(
    static_cast<int32_t>(mir_surface_type_satellite) ==
    static_cast<int32_t>(mir_window_type_satellite),
    "mir_surface_type_satellite != mir_window_type_satellite");
static_assert(
    static_cast<int32_t>(mir_surface_type_tip) ==
    static_cast<int32_t>(mir_window_type_tip),
    "mir_surface_type_tip != mir_window_type_tip");

// Assert our MirSurfaceState is 1to1 to MirWindowState
static_assert(
    static_cast<int32_t>(mir_surface_state_unknown) ==
    static_cast<int32_t>(mir_window_state_unknown),
    "mir_surface_state_unknown != mir_window_state_unknown");
static_assert(
    static_cast<int32_t>(mir_surface_state_restored) ==
    static_cast<int32_t>(mir_window_state_restored),
    "mir_surface_state_restored != mir_window_state_restored");
static_assert(
    static_cast<int32_t>(mir_surface_state_minimized) ==
    static_cast<int32_t>(mir_window_state_minimized),
    "mir_surface_state_minimized != mir_window_state_minimized");
static_assert(
    static_cast<int32_t>(mir_surface_state_maximized) ==
    static_cast<int32_t>(mir_window_state_maximized),
    "mir_surface_state_maximized != mir_window_state_maximized");
static_assert(
    static_cast<int32_t>(mir_surface_state_vertmaximized) ==
    static_cast<int32_t>(mir_window_state_vertmaximized),
    "mir_surface_state_vertmaximized != mir_window_state_vertmaximized");
static_assert(
    static_cast<int32_t>(mir_surface_state_fullscreen) ==
    static_cast<int32_t>(mir_window_state_fullscreen),
    "mir_surface_state_fullscreen != mir_window_state_fullscreen");
static_assert(
    static_cast<int32_t>(mir_surface_state_horizmaximized) ==
    static_cast<int32_t>(mir_window_state_horizmaximized),
    "mir_surface_state_horizmaximized != mir_window_state_horizmaximized");
static_assert(
    static_cast<int32_t>(mir_surface_state_hidden) ==
    static_cast<int32_t>(mir_window_state_hidden),
    "mir_surface_state_hidden != mir_window_state_hidden");
static_assert(
    static_cast<int32_t>(mir_surface_states) ==
    static_cast<int32_t>(mir_window_states),
    "mir_surface_states != mir_window_states");
static_assert(sizeof(MirSurfaceState) == sizeof(MirWindowState),
    "sizeof(MirSurfaceState) != sizeof(MirWindowState)");

// Assert our MirSurfaceFocusState is 1to1 to MirWindowFocusState
static_assert(
    static_cast<int32_t>(mir_surface_unfocused) ==
    static_cast<int32_t>(mir_window_focus_state_unfocused),
    "mir_surface_unfocused != mir_window_focus_state_unfocused");
static_assert(
    static_cast<int32_t>(mir_surface_focused) ==
    static_cast<int32_t>(mir_window_focus_state_focused),
    "mir_surface_focused != mir_window_focus_state_focused");
static_assert(sizeof(MirSurfaceFocusState) == sizeof(MirWindowFocusState),
    "sizeof(MirSurfaceFocusState) != sizeof(MirWindowFocusState)");

// Assert our MirSurfaceVisibility is 1to1 to MirWindowVisibility
static_assert(
    static_cast<int32_t>(mir_surface_visibility_occluded) ==
    static_cast<int32_t>(mir_window_visibility_occluded),
    "mir_surface_visibility_occluded != mir_window_visibility_occluded");
static_assert(
    static_cast<int32_t>(mir_surface_visibility_exposed) ==
    static_cast<int32_t>(mir_window_visibility_exposed),
    "mir_surface_visibility_exposed != mir_window_visibility_exposed");
static_assert(sizeof(MirSurfaceVisibility) == sizeof(MirWindowVisibility),
    "sizeof(MirSurfaceVisibility) != sizeof(MirWindowVisibility)");

struct ClientLibrary : mtf::HeadlessInProcessServer
{
    std::set<MirWindow*> surfaces;
    MirConnection* connection = nullptr;
    MirWindow* window  = nullptr;
    std::atomic<int> buffers{0};
    std::mutex guard;
    std::condition_variable signal;

    static void connection_callback(MirConnection* connection, void* context)
    {
        ClientLibrary* config = reinterpret_cast<ClientLibrary*>(context);
        config->connected(connection);
    }

    static void create_surface_callback(MirWindow* window, void* context)
    {
        ClientLibrary* config = reinterpret_cast<ClientLibrary*>(context);
        config->surface_created(window);
    }

    static void swap_buffers_callback(MirBufferStream* bs, void* context)
    {
        ClientLibrary* config = reinterpret_cast<ClientLibrary*>(context);
        config->swap_buffers(bs);
    }

    static void release_surface_callback(MirWindow* window, void* context)
    {
        ClientLibrary* config = reinterpret_cast<ClientLibrary*>(context);
        config->surface_released(window);
    }

    virtual void connected(MirConnection* new_connection)
    {
        connection = new_connection;
    }

    virtual void surface_created(MirWindow* new_surface)
    {
        std::unique_lock<std::mutex> lock(guard);
        surfaces.insert(new_surface);
        window = new_surface;
        signal.notify_all();
    }

    void wait_for_window_create()
    {
        std::unique_lock<std::mutex> lock(guard);
        window = nullptr;
        signal.wait(lock, [&]{ return !!window; });
    }

    virtual void swap_buffers(MirBufferStream*)
    {
        ++buffers;
    }

    void surface_released(MirWindow* old_surface)
    {
        std::unique_lock<std::mutex> lock(guard);
        surfaces.erase(old_surface);
        window = NULL;
        signal.notify_all();
    }

    void wait_for_window_release()
    {
        std::unique_lock<std::mutex> lock(guard);
        signal.wait(lock, [&]{ return !window; });
    }

    MirWindow* any_surface()
    {
        return *surfaces.begin();
    }

    size_t current_surface_count()
    {
        return surfaces.size();
    }

    static void nosey_thread(MirWindow *surf)
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
    buffer << mir::protobuf::oldest_compatible_protocol_version();

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
    buffer << (mir::protobuf::oldest_compatible_protocol_version() - 1);
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
    buffer << mir::protobuf::current_protocol_version() + 1;
    add_to_environment(protocol_version_override, buffer.str().c_str());

    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    EXPECT_THAT(connection, NotNull());
    EXPECT_FALSE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), HasSubstr("not accepted by server"));

    mir_connection_release(connection);
}

TEST_F(ClientLibrary, reports_error_when_protobuf_protocol_much_too_new)
{
    std::ostringstream buffer;
    buffer << mir::protobuf::next_incompatible_protocol_version();
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
    MirBufferUsage request_buffer_usage = mir_buffer_usage_software;

    auto spec = mir_create_normal_window_spec(connection, request_width, request_height);
    mir_window_spec_set_pixel_format(spec, request_format);
    mir_window_spec_set_buffer_usage(spec, request_buffer_usage);
    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    ASSERT_THAT(window, NotNull());
    EXPECT_TRUE(mir_window_is_valid(window));
    EXPECT_THAT(mir_window_get_error_message(window), StrEq(""));

    MirWindowParameters response_params;
    mir_window_get_parameters(window, &response_params);
    EXPECT_EQ(request_width, response_params.width);
    EXPECT_EQ(request_height, response_params.height);
    EXPECT_EQ(request_format, response_params.pixel_format);
    EXPECT_EQ(request_buffer_usage, response_params.buffer_usage);

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, shutdown_race_is_resolved_safely)
{   // An attempt at a regression test for race LP: #1653658
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    auto const spec = mir_create_normal_window_spec(connection, 123, 456);
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    EXPECT_THAT(window, IsValid());

    mir_window_release(window, [](MirWindow*, void*){ sleep(1); }, NULL);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_set_surface_state)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto const spec =
        mir_create_normal_window_spec(connection, 640, 480);
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);

    window = mir_create_window_sync(spec);

    mir_window_spec_release(spec);

    EXPECT_THAT(mir_window_get_state(window), Eq(mir_window_state_restored));

    mir_wait_for(mir_surface_set_state(window, mir_surface_state_fullscreen));
    EXPECT_THAT(mir_surface_get_state(window), Eq(mir_surface_state_fullscreen));

    mir_wait_for(mir_surface_set_state(window, static_cast<MirSurfaceState>(999)));
    EXPECT_THAT(mir_surface_get_state(window), Eq(mir_surface_state_fullscreen));

    mir_wait_for(mir_surface_set_state(window, mir_surface_state_horizmaximized));
    EXPECT_THAT(mir_surface_get_state(window), Eq(mir_surface_state_horizmaximized));

    mir_wait_for(mir_surface_set_state(window, static_cast<MirSurfaceState>(888)));
    EXPECT_THAT(mir_surface_get_state(window), Eq(mir_surface_state_horizmaximized));

    // Stress-test synchronization logic with some flooding
    for (int i = 0; i < 100; i++)
    {
        mir_window_set_state(window, mir_window_state_maximized);
        mir_window_set_state(window, mir_window_state_restored);
        mir_wait_for(mir_surface_set_state(window, mir_surface_state_fullscreen));
        ASSERT_THAT(mir_surface_get_state(window), Eq(mir_surface_state_fullscreen));
    }

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_set_pointer_confinement)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    int const width = 640;
    int const height = 480;
    auto const format = mir_pixel_format_abgr_8888;
    auto const spec = mir_create_normal_window_spec(connection, width, height);
    mir_window_spec_set_pixel_format(spec, format);

    mir_window_spec_set_pointer_confinement(spec, mir_pointer_confined_to_window);
    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    EXPECT_THAT(window, IsValid());

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_set_surface_min_width)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width = 640;
    int const height = 480;
    auto const format = mir_pixel_format_abgr_8888;
    auto const spec = mir_create_normal_window_spec(connection, width, height);
    mir_window_spec_set_pixel_format(spec, format);

    int const min_width = 480;
    mir_window_spec_set_min_width(spec, min_width);
    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    EXPECT_THAT(window, IsValid());

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_set_surface_min_height)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width = 640;
    int const height = 480;
    auto const format = mir_pixel_format_abgr_8888;
    auto const spec = mir_create_normal_window_spec(connection, width, height);
    mir_window_spec_set_pixel_format(spec, format);

    int const min_height = 480;
    mir_window_spec_set_min_height(spec, min_height);
    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    EXPECT_THAT(window, IsValid());

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_set_surface_max_width)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width = 640;
    int const height = 480;
    auto const format = mir_pixel_format_abgr_8888;
    auto const spec = mir_create_normal_window_spec(connection, width, height);
    mir_window_spec_set_pixel_format(spec, format);

    int const max_width = 1024;
    mir_window_spec_set_max_width(spec, max_width);
    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    EXPECT_THAT(window, IsValid());

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_set_surface_max_height)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width = 640;
    int const height = 480;
    auto const format = mir_pixel_format_abgr_8888;
    auto const spec = mir_create_normal_window_spec(connection, width, height);
    mir_window_spec_set_pixel_format(spec, format);

    int const max_height = 1024;
    mir_window_spec_set_max_height(spec, max_height);
    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    EXPECT_THAT(window, IsValid());

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, min_size_respected_when_placing_surface)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width = 6400;
    int const height = 4800;
    auto const format = mir_pixel_format_abgr_8888;
    auto const spec = mir_create_normal_window_spec(connection, width, height);
    mir_window_spec_set_pixel_format(spec, format);

    int const min_width = 4800;
    int const min_height = 3200;

    mir_window_spec_set_min_width(spec, min_width);
    mir_window_spec_set_min_height(spec, min_height);
    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    auto const buffer_stream = mir_window_get_buffer_stream(window);

    MirGraphicsRegion graphics_region;
    EXPECT_TRUE(mir_buffer_stream_get_graphics_region(buffer_stream, &graphics_region));
    EXPECT_THAT(graphics_region.width, Ge(min_width));
    EXPECT_THAT(graphics_region.height, Ge(min_height));

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, receives_surface_dpi_value)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto const spec = mir_create_normal_window_spec(connection, 640, 480);
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);

    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    // Expect zero (not wired up to detect the physical display yet)
    EXPECT_THAT(mir_window_get_dpi(window), Eq(0));

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, gets_buffer_dimensions)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto const spec =
        mir_create_normal_window_spec(connection, 0, 0);
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
    mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);

    struct {int width, height;} const sizes[] =
    {
        {12, 34},
        {56, 78},
        {90, 21},
    };

    for (auto const& size : sizes)
    {
        mir_window_spec_set_width(spec, size.width);
        mir_window_spec_set_height(spec, size.height);

        window = mir_create_window_sync(spec);

        auto bs = mir_window_get_buffer_stream(window);

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

        mir_window_release_sync(window);
    }

    mir_window_spec_release(spec);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, creates_multiple_surfaces)
{
    int const n_surfaces = 13;
    size_t old_surface_count = 0;

    mir_wait_for(mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__, connection_callback, this));

    auto const spec =
        mir_create_normal_window_spec(connection, 640, 480);
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);

    mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);

    for (int i = 0; i != n_surfaces; ++i)
    {
        old_surface_count = current_surface_count();

        mir_create_window(spec, create_surface_callback, this);
        wait_for_window_create();

        ASSERT_THAT(current_surface_count(), Eq(old_surface_count + 1));
    }
    for (int i = 0; i != n_surfaces; ++i)
    {
        old_surface_count = current_surface_count();

        ASSERT_THAT(old_surface_count, Ne(0u));

        window = any_surface();
        mir_window_release(window, release_surface_callback, this);
        wait_for_window_release();

        ASSERT_THAT(current_surface_count(), Eq(old_surface_count - 1));
    }

    mir_window_spec_release(spec);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, client_library_accesses_and_advances_buffers)
{
    mir_wait_for(mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__, connection_callback, this));

    window = mtf::make_any_surface(connection);

    buffers = 0;

    mir_wait_for(mir_buffer_stream_swap_buffers(mir_window_get_buffer_stream(window), swap_buffers_callback, this));

    EXPECT_THAT(buffers, Eq(1));

    mir_window_release(window, release_surface_callback, this);
    wait_for_window_release();

    ASSERT_THAT(window, IsNull());

    mir_connection_release(connection);
}

TEST_F(ClientLibrary, fully_synchronous_client)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    window = mtf::make_any_surface(connection);

    mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window));

    EXPECT_TRUE(mir_window_is_valid(window));
    EXPECT_STREQ(mir_window_get_error_message(window), "");

    mir_window_release_sync(window);

    EXPECT_TRUE(mir_connection_is_valid(connection));
    EXPECT_STREQ("", mir_connection_get_error_message(connection));
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, highly_threaded_client)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    window = mtf::make_any_surface(connection);

    std::thread a(nosey_thread, window);
    std::thread b(nosey_thread, window);
    std::thread c(nosey_thread, window);

    a.join();
    b.join();
    c.join();

    EXPECT_THAT(mir_window_get_state(window), Eq(mir_window_state_minimized));

    mir_window_release_sync(window);

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

    auto configuration = mir_connection_create_display_configuration(connection);
    ASSERT_THAT(configuration, NotNull());

    size_t num_outputs = mir_display_config_get_num_outputs(configuration);
    ASSERT_GT(num_outputs, 0u);
    for (auto i=0u; i < num_outputs; i++)
    {
        auto output = mir_display_config_get_output(configuration, i);
        ASSERT_THAT(output, NotNull());
        // Since these return types are changing make the types explicit:
        int const num_modes = mir_output_get_num_modes(output);
        int const current_mode_index = mir_output_get_current_mode_index(output);
        EXPECT_GE(num_modes, current_mode_index);
        EXPECT_GE(mir_output_get_num_pixel_formats(output), 0);
    }

    mir_display_config_release(configuration);
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

    // StubDisplayConfiguration is set to 60Hz and we now actually honour
    // that. So to avoid 1024 frames taking 17 seconds or so...
    mir_buffer_stream_set_swapinterval(mir_window_get_buffer_stream(surf_one), 0);
    mir_buffer_stream_set_swapinterval(mir_window_get_buffer_stream(surf_two), 0);

    while (buffers < 1024)
    {
        mir_buffer_stream_swap_buffers_sync(
            mir_window_get_buffer_stream(surf_one));
        mir_buffer_stream_swap_buffers_sync(
            mir_window_get_buffer_stream(surf_two));

        buffers++;
    }

    /* We should not have any stray fds hanging around.
       Test this by trying to open a new one */
    int canary_fd;
    canary_fd = open("/dev/null", O_RDONLY);

    ASSERT_THAT(canary_fd, Gt(0)) << "Failed to open canary file descriptor: "<< strerror(errno);
    EXPECT_THAT(canary_fd, Lt(1024));

    close(canary_fd);

    mir_window_release(surf_one, release_surface_callback, this);
    wait_for_window_release();
    mir_window_release(surf_two, release_surface_callback, this);
    wait_for_window_release();

    ASSERT_THAT(current_surface_count(), testing::Eq(0u));

    mir_connection_release(connection);
}

/* TODO: Our stub platform support is a bit terrible.
 *
 * These acceptance tests accidentally work on mesa because the mesa client
 * platform doesn't validate any of its input and we don't touch anything that requires
 * syscalls.
 *
 */

TEST_F(ClientLibrary, create_simple_normal_surface_from_spec)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width{800}, height{600};
    MirPixelFormat const format{mir_pixel_format_bgr_888};
    auto surface_spec = mir_create_normal_window_spec(connection, width, height);

    mir_window_spec_set_pixel_format(surface_spec, format);

    auto window = mir_create_window_sync(surface_spec);
    mir_window_spec_release(surface_spec);

    EXPECT_THAT(window, IsValid());

    MirNativeBuffer* native_buffer;
    mir_buffer_stream_get_current_buffer(
        mir_window_get_buffer_stream(window), &native_buffer);

    EXPECT_THAT(native_buffer->width, Eq(width));
    EXPECT_THAT(native_buffer->height, Eq(height));
    EXPECT_THAT(mir_window_get_type(window), Eq(mir_window_type_normal));

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, create_simple_normal_surface_from_spec_async)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    int const width{800}, height{600};
    MirPixelFormat const format{mir_pixel_format_xbgr_8888};
    auto surface_spec = mir_create_normal_window_spec(connection, width, height);

    mir_window_spec_set_pixel_format(surface_spec, format);

    window = mir_create_window_sync(surface_spec);
    mir_window_spec_release(surface_spec);

    EXPECT_THAT(window, IsValid());

    MirNativeBuffer* native_buffer;

    mir_buffer_stream_get_current_buffer(
        mir_window_get_buffer_stream(window), &native_buffer);

    EXPECT_THAT(native_buffer->width, Eq(width));
    EXPECT_THAT(native_buffer->height, Eq(height));
    EXPECT_THAT(mir_window_get_type(window), Eq(mir_window_type_normal));

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, can_specify_all_normal_surface_parameters_from_spec)
{
    using namespace testing;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto surface_spec = mir_create_normal_window_spec(connection, 800, 600);
    mir_window_spec_set_pixel_format(surface_spec, mir_pixel_format_bgr_888);

    char const* name = "The magnificent Dandy Warhols";
    mir_window_spec_set_name(surface_spec, name);

    int const width{999}, height{555};
    mir_window_spec_set_width(surface_spec, width);
    mir_window_spec_set_height(surface_spec, height);

    MirPixelFormat const pixel_format{mir_pixel_format_argb_8888};
    mir_window_spec_set_pixel_format(surface_spec, pixel_format);

    MirBufferUsage const buffer_usage{mir_buffer_usage_software};
    mir_window_spec_set_buffer_usage(surface_spec, buffer_usage);

    auto window = mir_create_window_sync(surface_spec);
    mir_window_spec_release(surface_spec);

    EXPECT_THAT(window, IsValid());

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, set_fullscreen_on_output_makes_fullscreen_surface)
{
    using namespace testing;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto surface_spec = mir_create_normal_window_spec(connection, 780, 555);
    mir_window_spec_set_pixel_format(surface_spec, mir_pixel_format_xbgr_8888);
    // We need to specify a valid output id, so we need to find which ones are valid...
    auto configuration = mir_connection_create_display_configuration(connection);
    auto num_outputs = mir_display_config_get_num_outputs(configuration);
    ASSERT_THAT(num_outputs, Ge(1u));

    auto output = mir_display_config_get_output(configuration, 0);

    auto id = mir_output_get_id(output);
    mir_window_spec_set_fullscreen_on_output(surface_spec, id);

    auto window = mir_create_window_sync(surface_spec);
    mir_window_spec_release(surface_spec);

    EXPECT_THAT(window, IsValid());

    MirNativeBuffer* native_buffer;
    mir_buffer_stream_get_current_buffer(
        mir_window_get_buffer_stream(window), &native_buffer);

    auto current_mode = mir_output_get_current_mode(output);
    int const mode_width = mir_output_mode_get_width(current_mode);
    EXPECT_THAT(native_buffer->width, Eq(mode_width));
    int const mode_height = mir_output_mode_get_height(current_mode);
    EXPECT_THAT(native_buffer->height, Eq(mode_height));

// TODO: This is racy. Fix in subsequent "send all the things on construction" branch
//    EXPECT_THAT(mir_window_get_state(window), Eq(mir_window_state_fullscreen));

    mir_window_release_sync(window);
    mir_display_config_release(configuration);
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

    auto surface_spec = mir_create_normal_window_spec(connection, 800, 600);
    mir_window_spec_set_pixel_format(surface_spec, mir_pixel_format_bgr_888);
    MirBufferUsage const buffer_usage{mir_buffer_usage_software};
    mir_window_spec_set_buffer_usage(surface_spec, buffer_usage);

    auto window = mir_create_window_sync(surface_spec);
    mir_window_spec_release(surface_spec);

    EXPECT_THAT(window, IsValid());

    MirNativeBuffer* native_buffer;
    // We use the fact that our stub client platform returns NULL if asked for a native
    // buffer on a window with mir_buffer_usage_software set.
    mir_buffer_stream_get_current_buffer(
        mir_window_get_buffer_stream(window), &native_buffer);

    EXPECT_THAT(native_buffer, Not(Eq(nullptr)));

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, DISABLED_can_create_buffer_usage_software_surface)
{
    using namespace testing;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto surface_spec = mir_create_normal_window_spec(connection, 800, 600);
    mir_window_spec_set_pixel_format(surface_spec, mir_pixel_format_bgr_888);
    MirBufferUsage const buffer_usage{mir_buffer_usage_software};
    mir_window_spec_set_buffer_usage(surface_spec, buffer_usage);

    auto window = mir_create_window_sync(surface_spec);
    mir_window_spec_release(surface_spec);

    EXPECT_THAT(window, IsValid());

    MirGraphicsRegion graphics_region;
    // We use the fact that our stub client platform returns a NULL vaddr if
    // asked to map a hardware buffer.
    mir_buffer_stream_get_graphics_region(
        mir_window_get_buffer_stream(window), &graphics_region);

    EXPECT_THAT(graphics_region.vaddr, Not(Eq(nullptr)));

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

namespace
{
void dummy_event_handler_one(MirWindow*, MirEvent const*, void*)
{
}

void dummy_event_handler_two(MirWindow*, MirEvent const*, void*)
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

    auto surface_spec = mir_create_normal_window_spec(connection, 800, 600);
    mir_window_spec_set_pixel_format(surface_spec, mir_pixel_format_argb_8888);
    auto window = mir_create_window_sync(surface_spec);
    mir_window_spec_release(surface_spec);

    ASSERT_THAT(window, IsValid());

    /* TODO: When provide-event-fd lands, change this into a better test that actually
     * tests that the correct event handler is called.
     *
     * Without manual dispatch, it's racy to try and test that.
     */
    mir_window_set_event_handler(window, &dummy_event_handler_one, nullptr);
    mir_window_set_event_handler(window, &dummy_event_handler_two, nullptr);

    mir_window_release_sync(window);
    mir_connection_release(connection);
}



TEST_F(ClientLibrary, can_get_persistent_surface_id)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto surface_spec = mir_create_normal_window_spec(connection, 800, 600);
    mir_window_spec_set_pixel_format(surface_spec, mir_pixel_format_argb_8888);
    auto window = mir_create_window_sync(surface_spec);
    mir_window_spec_release(surface_spec);

    ASSERT_THAT(window, IsValid());

    auto surface_id = mir_window_request_persistent_id_sync(window);
    EXPECT_TRUE(mir_persistent_id_is_valid(surface_id));

    mir_window_release_sync(window);
    mir_persistent_id_release(surface_id);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, input_method_can_specify_foreign_surface_id)
{
    auto first_client = mir_connect_sync(new_connection().c_str(), "Regular Client");

    auto surface_spec = mir_create_normal_window_spec(first_client, 800, 600);
    mir_window_spec_set_pixel_format(surface_spec, mir_pixel_format_argb_8888);
    auto main_surface = mir_create_window_sync(surface_spec);
    mir_window_spec_release(surface_spec);

    ASSERT_THAT(main_surface, IsValid());

    auto main_surface_id = mir_window_request_persistent_id_sync(main_surface);
    ASSERT_TRUE(mir_persistent_id_is_valid(main_surface_id));

    // Serialise & deserialise the ID
    auto im_parent_id = mir_persistent_id_from_string(mir_persistent_id_as_string(main_surface_id));

    auto im_client = mir_connect_sync(new_connection().c_str(), "IM Client");
    surface_spec = mir_create_input_method_window_spec(im_client, 200, 20);
    mir_window_spec_set_pixel_format(surface_spec, mir_pixel_format_argb_8888);
    MirRectangle attachment_rect {
        200,
        200,
        10,
        10
    };
    mir_window_spec_attach_to_foreign_parent(surface_spec,
                                              im_parent_id,
                                              &attachment_rect,
                                              mir_edge_attachment_any);
    auto im_surface = mir_create_window_sync(surface_spec);

    EXPECT_THAT(im_surface, IsValid());

    mir_window_spec_release(surface_spec);
    mir_persistent_id_release(main_surface_id);
    mir_persistent_id_release(im_parent_id);
    mir_window_release_sync(main_surface);
    mir_window_release_sync(im_surface);
    mir_connection_release(first_client);
    mir_connection_release(im_client);
}


//lp:1661704
TEST_F(ClientLibrary, can_get_window_id_more_than_once_in_quick_succession)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto surface_spec = mir_create_normal_window_spec(connection, 800, 600);
    mir_window_spec_set_pixel_format(surface_spec, mir_pixel_format_argb_8888);
    auto window = mir_create_window_sync(surface_spec);
    mir_window_spec_release(surface_spec);

    ASSERT_THAT(window, IsValid());

    MirWindowId* surface_id;
    MirWindowId* window_id = nullptr; //circumvent ‘window_id’ uninitialized error
    surface_id = mir_window_request_window_id_sync(window);
    EXPECT_NO_THROW({
        window_id = mir_window_request_window_id_sync(window);
    });

    mir_window_id_release(surface_id);
    mir_window_id_release(window_id);
    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, creates_buffer_streams)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    auto stream = mir_connection_create_buffer_stream_sync(connection,
        640, 480, mir_pixel_format_abgr_8888, mir_buffer_usage_software);

    ASSERT_THAT(stream, NotNull());
    EXPECT_TRUE(mir_buffer_stream_is_valid(stream));
    EXPECT_THAT(mir_buffer_stream_get_error_message(stream), StrEq(""));

    mir_buffer_stream_release_sync(stream);
    mir_connection_release(connection);
}

TEST_F(ClientLibrary, client_api_version)
{
    ASSERT_TRUE( MIR_VERSION_NUMBER(MIR_CLIENT_API_VERSION_MAJOR,
                                    MIR_CLIENT_API_VERSION_MINOR,
                                    MIR_CLIENT_API_VERSION_PATCH) == mir_get_client_api_version());
}
#pragma GCC diagnostic pop
