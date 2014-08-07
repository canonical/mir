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
#include "mir/graphics/buffer_id.h"
#include "mir_toolkit/mir_client_library.h"
#include "src/client/mir_connection.h"
#include "mir_protobuf.pb.h"
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mtf = mir_test_framework;
namespace mg = mir::graphics;

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
}

TEST_F(ExchangeBufferTest, exchanges_happen)
{

    struct ServerConfig : TestingServerConfiguration
    {
        ServerConfig(std::vector<mg::BufferID> const& id_exchange_sequence)
            : buffer_id_seq{id_exchange_sequence}
        {
        }

        std::vector<mg::BufferID> const buffer_id_seq;
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
