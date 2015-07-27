/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/spin_wait.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>
#include <chrono>
#include <thread>

namespace mt = mir::test;
namespace mtf = mir_test_framework;

using namespace std::chrono_literals;

namespace
{

struct ClientLibraryCallbacks : mtf::HeadlessInProcessServer
{
    std::atomic<MirConnection*> connection{nullptr};
    std::atomic<MirSurface*> surface{nullptr};
    std::atomic<int> buffers{0};

    void TearDown() override
    {
        if (surface)
            mir_surface_release_sync(surface);
        if (connection)
            mir_connection_release(connection);

        mtf::HeadlessInProcessServer::TearDown();
    }

    static void connection_callback(MirConnection* connection, void* context)
    {
        auto const config = reinterpret_cast<ClientLibraryCallbacks*>(context);
        config->connected(connection);
    }

    static void create_surface_callback(MirSurface* surface, void* context)
    {
        auto const config = reinterpret_cast<ClientLibraryCallbacks*>(context);
        config->surface_created(surface);
    }

    static void next_buffer_callback(MirBufferStream* bs, void* context)
    {
        auto const config = reinterpret_cast<ClientLibraryCallbacks*>(context);
        config->next_buffer(bs);
    }

    static void release_surface_callback(MirSurface* surface, void* context)
    {
        auto const config = reinterpret_cast<ClientLibraryCallbacks*>(context);
        config->surface_released(surface);
    }

    virtual void connected(MirConnection* conn)
    {
        std::this_thread::sleep_for(10ms);
        connection = conn;
    }

    virtual void surface_created(MirSurface* new_surface)
    {
        std::this_thread::sleep_for(10ms);
        surface = new_surface;
    }

    virtual void next_buffer(MirBufferStream*)
    {
        std::this_thread::sleep_for(10ms);
        ++buffers;
    }

    void surface_released(MirSurface*)
    {
        std::this_thread::sleep_for(10ms);
        surface = nullptr;
    }

    mtf::UsingStubClientPlatform using_stub_client_platform;
};

}

using namespace testing;

TEST_F(ClientLibraryCallbacks, connect_callback_is_called_before_wait_handler_has_result)
{
    auto const wh = mir_connect(
        new_connection().c_str(), __PRETTY_FUNCTION__,
        connection_callback, this);
    mir_wait_for(wh);

    EXPECT_THAT(connection.load(), NotNull());

    // Even if the test fails, wait for object to become ready so we can
    // tear down properly
    mt::spin_wait_for_condition_or_timeout(
        [this] { return connection != nullptr; },
        3s);
}

TEST_F(ClientLibraryCallbacks, create_surface_callback_is_called_before_wait_handler_has_result)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto const spec = mir_connection_create_spec_for_normal_surface(
        connection, 100, 100, mir_pixel_format_argb_8888);
    auto const wh = mir_surface_create(spec, create_surface_callback, this);
    mir_surface_spec_release(spec);
    mir_wait_for(wh);

    EXPECT_THAT(surface.load(), NotNull());

    // Even if the test fails, wait for object to become ready so we can
    // tear down properly
    mt::spin_wait_for_condition_or_timeout(
        [this] { return surface != nullptr; },
        3s);
}

TEST_F(ClientLibraryCallbacks, swap_buffers_callback_is_called_before_wait_handler_has_result)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    surface = mtf::make_any_surface(connection);

    auto const wh = mir_buffer_stream_swap_buffers(
        mir_surface_get_buffer_stream(surface), next_buffer_callback, this);
    mir_wait_for(wh);

    EXPECT_THAT(buffers, Eq(1));

    // Even if the test fails, wait for object to become ready so we can
    // tear down properly
    mt::spin_wait_for_condition_or_timeout(
        [this] { return buffers == 1; },
        3s);
}

TEST_F(ClientLibraryCallbacks, release_surface_callback_is_called_before_wait_handler_has_result)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    surface = mtf::make_any_surface(connection);

    auto const wh = mir_surface_release(surface, release_surface_callback, this);
    mir_wait_for(wh);

    EXPECT_THAT(surface.load(), IsNull());

    // Even if the test fails, wait for object to become ready so we can
    // tear down properly
    mt::spin_wait_for_condition_or_timeout(
        [this] { return surface == nullptr; },
        3s);
}
