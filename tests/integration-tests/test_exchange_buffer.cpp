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

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir_test_doubles/null_platform.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir/graphics/buffer_id.h"
#include "mir/scene/buffer_stream_factory.h"
#include "mir/compositor/buffer_stream.h"
#include "mir_toolkit/mir_client_library.h"
#include "src/client/mir_connection.h"
#include "mir_protobuf.pb.h"
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

namespace
{
MATCHER(DidNotTimeOut, "did not time out")
{
    return !arg;
}
struct ExchangeBufferTest : BespokeDisplayServerTestFixture
{
    std::vector<mg::BufferID> const buffer_id_exchange_seq{
        mg::BufferID{4}, mg::BufferID{8}, mg::BufferID{9}};
};

struct StubStream : public mc::BufferStream
{
    StubStream(std::vector<mg::BufferID> const& ids) :
        buffer_id_seq(ids),
        current{buffer_id_seq.begin()}
    {
    }

    void acquire_client_buffer(std::function<void(mg::Buffer* buffer)> complete)
    {
        auto b = std::make_shared<testing::NiceMock<mtd::MockBuffer>>();
        auto id = *current;
        if (current + 1 != buffer_id_seq.end())
            current++;
        ON_CALL(*b, id())
            .WillByDefault(testing::Return(id));
        client_buffer = b;
        complete(client_buffer.get());
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

    std::shared_ptr<mg::Buffer> client_buffer;
    std::vector<mg::BufferID> const buffer_id_seq;
    std::vector<mg::BufferID>::const_iterator current;
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
}

TEST_F(ExchangeBufferTest, exchanges_happen)
{
    struct ServerConfig : TestingServerConfiguration
    {
        ServerConfig(std::vector<mg::BufferID> const& id_seq) :
           stream_factory{std::make_shared<StubStreamFactory>(id_seq)}
        {
        }

        std::shared_ptr<msc::BufferStreamFactory> the_buffer_stream_factory() override
        {
            return stream_factory;
        }

        std::shared_ptr<msc::BufferStreamFactory> const stream_factory;
    } server_config{buffer_id_exchange_seq};

    launch_server_process(server_config);

    struct Client : TestingClientConfiguration
    {
        Client(std::vector<mg::BufferID> const& id_exchange_sequence)
            : buffer_id_seq{id_exchange_sequence}
        {
        }

        void buffer_arrival()
        {
            std::unique_lock<decltype(mutex)> lk(mutex);
            arrived = true;
        }

        bool wait_on_buffer(std::unique_lock<std::mutex>& lk)
        {
            if (!cv.wait_for(lk, std::chrono::seconds(1), [this]() {return arrived;}))
                return false;

            arrived = false;
            return true;
        }

        //TODO: once the next_buffer rpc is deprecated, change this code out for the
        //      mir_surface_next_buffer() api call
        bool exchange_buffer(mir::protobuf::DisplayServer& server)
        {
            std::unique_lock<decltype(mutex)> lk(mutex);
            mir::protobuf::Buffer next;
            server.exchange_buffer(0, &current_buffer, &next,
                           google::protobuf::NewCallback(this, &Client::buffer_arrival));
            if (!wait_on_buffer(lk))
                return false;
            
            current_buffer = next;
            return true;
        }
 
        void exec()
        {
            auto connection = mir_connect_sync(mtf::test_socket_file().c_str(), __PRETTY_FUNCTION__);

            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware,
                mir_display_output_id_invalid
            };
            auto surface = mir_connection_create_surface_sync(connection, &request_params);

            auto rpc_channel = connection->rpc_channel();
            mir::protobuf::DisplayServer::Stub server(
                rpc_channel.get(), ::google::protobuf::Service::STUB_DOESNT_OWN_CHANNEL);

            for(auto const& id : buffer_id_seq)
            { 
                ASSERT_THAT(exchange_buffer(server), DidNotTimeOut());
                EXPECT_THAT(id.as_uint32_t(), testing::Eq(current_buffer.buffer_id()));
            }

            mir_surface_release_sync(surface);
            mir_connection_release(connection);
        }

        private:
            std::mutex mutex;
            std::condition_variable cv;
            bool arrived{false};
            mir::protobuf::Buffer current_buffer; 
            std::vector<mg::BufferID> const buffer_id_seq;
    } client_config{buffer_id_exchange_seq};
    launch_client_process(client_config);
}
