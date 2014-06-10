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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/client/rpc/mir_socket_rpc_channel.h"
#include "src/client/rpc/transport.h"
#include "src/client/surface_map.h"
#include "src/client/display_configuration.h"
#include "src/client/rpc/null_rpc_report.h"
#include "src/client/lifecycle_control.h"

#include "mir_protobuf.pb.h"

#include <list>
#include <endian.h>

#include <google/protobuf/descriptor.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace mclr = mir::client::rpc;

namespace
{

class StubSurfaceMap : public mcl::SurfaceMap
{
public:
    void with_surface_do(int /*surface_id*/, std::function<void(MirSurface*)> /*exec*/) const
    {
    }
};

class MockTransport : public mclr::Transport
{
public:
    void add_receive_hunk(std::vector<uint8_t>&& message)
    {
        datagrams.emplace_back(std::forward<std::vector<uint8_t>>(message));
    }

    bool all_data_consumed() const
    {
        return datagrams.empty();
    }

    void notify_data_received()
    {
        for(auto& callback : callbacks)
            callback();
    }

    // Transport interface
    void register_data_received_notification(data_received_notifier const& callback)
    {
        callbacks.push_back(callback);
    }

    bool data_available() const override
    {
        return !datagrams.empty();
    }

    size_t receive_data(void* buffer, size_t message_size)
    {
        size_t read_bytes{0};
        while ((message_size != 0) && !datagrams.empty())
        {
            size_t read_size = std::min(datagrams.front().size() - read_offset, message_size);

            memcpy(buffer, datagrams.front().data() + read_offset, read_size);
            read_bytes += read_size;
            read_offset += read_size;
            message_size -= read_size;

            if (read_offset == datagrams.front().size())
            {
                datagrams.pop_front();
                read_offset = 0;
            }
        }
        return read_bytes;
    }

    void receive_file_descriptors(std::vector<int>& /*fds*/)
    {
    }

    size_t send_data(std::vector<uint8_t> const& /*buffer*/)
    {
        return 0;
    }

private:
    std::list<data_received_notifier> callbacks;

    size_t read_offset{0};
    std::list<std::vector<uint8_t>> datagrams;
};

class MirProtobufRpcChannelTest : public testing::Test
{
public:
    MirProtobufRpcChannelTest()
        : transport{new MockTransport},
          channel{std::unique_ptr<MockTransport>{transport},
                  std::make_shared<StubSurfaceMap>(),
                  std::make_shared<mcl::DisplayConfiguration>(),
                  std::make_shared<mclr::NullRpcReport>(),
                  std::make_shared<mcl::LifecycleControl>()}
    {
    }

    MockTransport* transport;
    mclr::MirProtobufRpcChannel channel;
};

}

TEST_F(MirProtobufRpcChannelTest, ReadsFullMessages)
{
    std::vector<uint8_t> empty_message(sizeof(uint16_t));
    std::vector<uint8_t> small_message(sizeof(uint16_t) + 8);
    std::vector<uint8_t> large_message(sizeof(uint16_t) + 4096);

    *reinterpret_cast<uint16_t*>(empty_message.data()) = htobe16(0);
    *reinterpret_cast<uint16_t*>(small_message.data()) = htobe16(8);
    *reinterpret_cast<uint16_t*>(large_message.data()) = htobe16(4096);

    transport->add_receive_hunk(std::move(empty_message));
    transport->notify_data_received();
    EXPECT_TRUE(transport->all_data_consumed());

    transport->add_receive_hunk(std::move(small_message));
    transport->notify_data_received();
    EXPECT_TRUE(transport->all_data_consumed());

    transport->add_receive_hunk(std::move(large_message));
    transport->notify_data_received();
    EXPECT_TRUE(transport->all_data_consumed());
}

TEST_F(MirProtobufRpcChannelTest, ReadsAllQueuedMessages)
{
    std::vector<uint8_t> empty_message(sizeof(uint16_t));
    std::vector<uint8_t> small_message(sizeof(uint16_t) + 8);
    std::vector<uint8_t> large_message(sizeof(uint16_t) + 4096);

    *reinterpret_cast<uint16_t*>(empty_message.data()) = htobe16(0);
    *reinterpret_cast<uint16_t*>(small_message.data()) = htobe16(8);
    *reinterpret_cast<uint16_t*>(large_message.data()) = htobe16(4096);

    transport->add_receive_hunk(std::move(empty_message));
    transport->add_receive_hunk(std::move(small_message));
    transport->add_receive_hunk(std::move(large_message));

    transport->notify_data_received();
    EXPECT_TRUE(transport->all_data_consumed());
}
