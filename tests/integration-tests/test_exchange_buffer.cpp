/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir/graphics/buffer_id.h"
#include "mir/scene/buffer_stream_factory.h"
#include "mir/compositor/buffer_stream.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_debug.h"
#include "src/client/mir_connection.h"
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace msc = mir::scene;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;

namespace
{
MATCHER(DidNotTimeOut, "did not time out")
{
    return arg;
}

struct StubStream : public mc::BufferStream
{
    StubStream(std::vector<mg::BufferID> const& ids) :
        buffer_id_seq(ids)
    {
    }

    void acquire_client_buffer(std::function<void(mg::Buffer* buffer)> complete)
    {
        std::shared_ptr<mg::Buffer> stub_buffer;
        if (buffers_acquired < buffer_id_seq.size())
            stub_buffer = std::make_shared<mtd::StubBuffer>(buffer_id_seq.at(buffers_acquired++));
        else
            stub_buffer = std::make_shared<mtd::StubBuffer>(buffer_id_seq.back());

        client_buffers.push_back(stub_buffer);
        complete(stub_buffer.get());
    }

    void release_client_buffer(mg::Buffer*) {}
    std::shared_ptr<mg::Buffer> lock_compositor_buffer(void const*)
    { return std::make_shared<mtd::StubBuffer>(); }
    std::shared_ptr<mg::Buffer> lock_snapshot_buffer()
    { return std::make_shared<mtd::StubBuffer>(); }
    MirPixelFormat get_stream_pixel_format() { return mir_pixel_format_abgr_8888; }
    geom::Size stream_size() { return geom::Size{1,212121}; }
    void resize(geom::Size const&) {};
    void allow_framedropping(bool) {};
    void force_requests_to_complete() {};
    int buffers_ready_for_compositor() const { return -5; }
    void drop_old_buffers() {}

    std::vector<std::shared_ptr<mg::Buffer>> client_buffers;
    std::vector<mg::BufferID> const buffer_id_seq;
    unsigned int buffers_acquired{0};
};

struct StubStreamFactory : public msc::BufferStreamFactory
{
    StubStreamFactory(std::vector<mg::BufferID> const& ids) :
        buffer_id_seq(ids)
    {}

    std::shared_ptr<mc::BufferStream> create_buffer_stream(mg::BufferProperties const&) override
    { return std::make_shared<StubStream>(buffer_id_seq); }
    std::vector<mg::BufferID> const buffer_id_seq;
};

struct ExchangeServerConfiguration : mtf::StubbedServerConfiguration
{
    ExchangeServerConfiguration(std::vector<mg::BufferID> const& id_seq) :
       stream_factory{std::make_shared<StubStreamFactory>(id_seq)}
    {
    }

    std::shared_ptr<msc::BufferStreamFactory> the_buffer_stream_factory() override
    {
        return stream_factory;
    }

    std::shared_ptr<msc::BufferStreamFactory> const stream_factory;
};

struct ExchangeBufferTest : mir_test_framework::InProcessServer
{
    std::vector<mg::BufferID> const buffer_id_exchange_seq{
        mg::BufferID{4}, mg::BufferID{8}, mg::BufferID{9}, mg::BufferID{3}, mg::BufferID{4}};
    ExchangeServerConfiguration server_configuration{buffer_id_exchange_seq};
    mir::DefaultServerConfiguration& server_config() override { return server_configuration; }
    mtf::UsingStubClientPlatform using_stub_client_platform;

    void buffer_arrival(MirSurface*, void*)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        arrived = true;
        cv.notify_all();
    }

    bool exchange_buffer()
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        arrived = false;
        return cv.wait_for(lk, std::chrono::seconds(5), [this]() {return arrived;});
    }

    std::mutex mutex;
    std::condition_variable cv;
    bool arrived{false};
};
}

TEST_F(ExchangeBufferTest, exchanges_happen)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };
    auto surface = mir_connection_create_surface_sync(connection, &request_params);
    for(auto const& id : buffer_id_exchange_seq)
    {
        EXPECT_THAT(mir_debug_surface_current_buffer_id(surface), testing::Eq(id.as_value()));
        mir_surface_swap_buffers_sync(surface);//(surface, std::bind(this, &ExchangeBufferTest::buffer_arrival));
//        ASSERT_THAT(wait_for_exchange(), DidNotTimeout());
    }

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}
