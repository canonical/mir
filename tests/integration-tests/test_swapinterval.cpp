/*
 * Copyright © 2013-2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 *              Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/scene/buffer_stream_factory.h"

#include "mir/test/doubles/stub_buffer_stream.h"
#include "mir/test/doubles/stub_buffer_stream_factory.h"

#include "mir_test_framework/any_surface.h"
#include "mir_test_framework/basic_client_server_fixture.h"
#include "mir_test_framework/testing_server_configuration.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>

#include <atomic>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace
{

class StubBufferStream : public mtd::StubBufferStream
{
public:
    StubBufferStream(std::atomic<bool>& framedropping_enabled)
        : framedropping_enabled{framedropping_enabled}
    {
    }

    void allow_framedropping(bool allow) override
    {
        framedropping_enabled = allow;
    }

private:
    std::atomic<bool>& framedropping_enabled;
};

class StubBufferStreamFactory : public ms::BufferStreamFactory
{
public:
    StubBufferStreamFactory(std::atomic<bool>& framedropping_enabled)
        : framedropping_enabled{framedropping_enabled}
    {
    }

    std::shared_ptr<mc::BufferStream> create_buffer_stream(
        int, mg::BufferProperties const& p) override
    {
        return create_buffer_stream(p);
    }

    std::shared_ptr<mc::BufferStream> create_buffer_stream(
        mg::BufferProperties const&) override
    {
        return std::make_shared<StubBufferStream>(framedropping_enabled);
    }

private:
    std::atomic<bool>& framedropping_enabled;
};


struct ServerConfig : mtf::TestingServerConfiguration
{
    std::shared_ptr<ms::BufferStreamFactory> the_buffer_stream_factory() override
    {
        if (!stub_buffer_stream_factory)
        {
            stub_buffer_stream_factory =
                std::make_shared<StubBufferStreamFactory>(framedropping_enabled);
        }
        return stub_buffer_stream_factory;
    }

    std::atomic<bool> framedropping_enabled{false};
    std::shared_ptr<StubBufferStreamFactory> stub_buffer_stream_factory;
};


struct SwapInterval : mtf::BasicClientServerFixture<ServerConfig>
{
    void SetUp()
    {
        mtf::BasicClientServerFixture<ServerConfig>::SetUp();
        window = mtf::make_any_surface(connection);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        stream = mir_connection_create_buffer_stream_sync(
            connection, 10, 10, mir_pixel_format_abgr_8888, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
    }

    void TearDown()
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_buffer_stream_release_sync(stream);
#pragma GCC diagnostic pop
        mir_window_release_sync(window);
        mtf::BasicClientServerFixture<ServerConfig>::TearDown();
    }

    bool framedropping_enabled()
    {
        return server_configuration.framedropping_enabled;
    }

    MirWindow* window;
    MirBufferStream* stream;
};

}

TEST_F(SwapInterval, defaults_to_one)
{
    EXPECT_EQ(1, mir_buffer_stream_get_swapinterval(stream));
    EXPECT_FALSE(framedropping_enabled());
}

TEST_F(SwapInterval, change_to_zero_enables_framedropping)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_wait_for(mir_buffer_stream_set_swapinterval(stream, 0));
#pragma GCC diagnostic pop

    EXPECT_EQ(0, mir_buffer_stream_get_swapinterval(stream));
    EXPECT_TRUE(framedropping_enabled());
}

TEST_F(SwapInterval, change_to_one_disables_framedropping)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_wait_for(mir_buffer_stream_set_swapinterval(stream, 0));
    mir_wait_for(mir_buffer_stream_set_swapinterval(stream, 1));
#pragma GCC diagnostic pop

    EXPECT_EQ(1, mir_buffer_stream_get_swapinterval(stream));
    EXPECT_FALSE(framedropping_enabled());
}
