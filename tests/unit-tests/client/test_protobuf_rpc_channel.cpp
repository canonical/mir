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
#include "mir_protobuf_wire.pb.h"

#include "mir_test_doubles/null_client_event_sink.h"

#include <list>
#include <endian.h>

#include <google/protobuf/descriptor.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace mclr = mir::client::rpc;
namespace mtd = mir::test::doubles;

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
    MockTransport()
    {
        using namespace testing;
        ON_CALL(*this, register_observer(_))
            .WillByDefault(Invoke(std::bind(&MockTransport::register_observer_default,
                                            this, std::placeholders::_1)));
        ON_CALL(*this, receive_data(_,_))
            .WillByDefault(Invoke(std::bind(&MockTransport::receive_data_default,
                                            this, std::placeholders::_1, std::placeholders::_2)));
        ON_CALL(*this, receive_file_descriptors(_))
            .WillByDefault(Invoke(std::bind(&MockTransport::receive_file_descriptors_default,
                                            this, std::placeholders::_1)));
        ON_CALL(*this, send_data(_))
            .WillByDefault(Invoke(std::bind(&MockTransport::send_data_default,
                                            this, std::placeholders::_1)));
    }

    void add_receive_hunk(std::vector<uint8_t>&& message)
    {
        datagrams.emplace_back(std::forward<std::vector<uint8_t>>(message), std::list<int>());
    }
    void add_receive_hunk(std::vector<uint8_t>&& message, std::initializer_list<int> fds)
    {
        datagrams.emplace_back(std::forward<std::vector<uint8_t>>(message), fds);
    }

    bool all_data_consumed() const
    {
        return datagrams.empty();
    }

    void notify_data_received()
    {
        do
        {
            for(auto& observer : observers)
                observer->on_data_available();
        }
        while (!all_data_consumed());
    }

    MOCK_METHOD1(register_observer, void(std::shared_ptr<Observer> const&));
    MOCK_METHOD2(receive_data, size_t(void*, size_t));
    MOCK_METHOD1(receive_file_descriptors, void(std::vector<int>&));
    MOCK_METHOD1(send_data, size_t(std::vector<uint8_t> const&));

    // Transport interface
    void register_observer_default(std::shared_ptr<Observer> const& observer)
    {
        observers.push_back(observer);
    }

    size_t receive_data_default(void* buffer, size_t message_size)
    {
        size_t read_bytes{0};
        while ((message_size != 0) && !datagrams.empty())
        {
            fds_for_current_message = datagrams.front().second;
            size_t read_size = std::min(datagrams.front().first.size() - read_offset, message_size);

            memcpy(buffer, datagrams.front().first.data() + read_offset, read_size);
            read_bytes += read_size;
            read_offset += read_size;
            message_size -= read_size;

            if (read_offset == datagrams.front().first.size())
            {
                datagrams.pop_front();
                read_offset = 0;
            }
        }
        return read_bytes;
    }

    void receive_file_descriptors_default(std::vector<int>& fds)
    {
        if (fds.size() > fds_for_current_message.size())
            throw std::runtime_error("Attempted to read more fds than are available");

        for (auto& fd : fds)
        {
            fd = fds_for_current_message.front();
            fds_for_current_message.pop_front();
        }
    }

    size_t send_data_default(std::vector<uint8_t> const& buffer)
    {
        sent_messages.push_back(buffer);
        return buffer.size();
    }

    std::list<std::shared_ptr<Observer>> observers;

    size_t read_offset{0};
    std::list<std::pair<std::vector<uint8_t>, std::list<int>>> datagrams;
    std::list<int> fds_for_current_message;
    std::list<std::vector<uint8_t>> sent_messages;
};

class MirProtobufRpcChannelTest : public testing::Test
{
public:
    MirProtobufRpcChannelTest()
        : transport{new testing::NiceMock<MockTransport>},
          lifecycle{std::make_shared<mcl::LifecycleControl>()},
          channel{new mclr::MirProtobufRpcChannel{
                  std::unique_ptr<MockTransport>{transport},
                  std::make_shared<StubSurfaceMap>(),
                  std::make_shared<mcl::DisplayConfiguration>(),
                  std::make_shared<mclr::NullRpcReport>(),
                  lifecycle,
                  std::make_shared<mtd::NullClientEventSink>()}}
    {
    }

    MockTransport* transport;
    std::shared_ptr<mcl::LifecycleControl> lifecycle;
    std::shared_ptr<::google::protobuf::RpcChannel> channel;
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

TEST_F(MirProtobufRpcChannelTest, SendsMessagesAtomically)
{
    mir::protobuf::DisplayServer::Stub channel_user{channel.get(), mir::protobuf::DisplayServer::STUB_DOESNT_OWN_CHANNEL};
    mir::protobuf::ConnectParameters message;
    message.set_application_name("I'm a little teapot!");

    channel_user.connect(nullptr, &message, nullptr, nullptr);

    EXPECT_EQ(transport->sent_messages.size(), 1);
}

TEST_F(MirProtobufRpcChannelTest, SetsCorrectSizeWhenSendingMessage)
{
    mir::protobuf::DisplayServer::Stub channel_user{channel.get(), mir::protobuf::DisplayServer::STUB_DOESNT_OWN_CHANNEL};
    mir::protobuf::ConnectParameters message;
    message.set_application_name("I'm a little teapot!");

    channel_user.connect(nullptr, &message, nullptr, nullptr);

    uint16_t message_header = *reinterpret_cast<uint16_t*>(transport->sent_messages.front().data());
    message_header = be16toh(message_header);
    EXPECT_EQ(transport->sent_messages.front().size() - sizeof(uint16_t), message_header);
}

TEST_F(MirProtobufRpcChannelTest, ReadsFds)
{
    mir::protobuf::DisplayServer::Stub channel_user{channel.get(), mir::protobuf::DisplayServer::STUB_DOESNT_OWN_CHANNEL};
    mir::protobuf::Buffer reply;
    mir::protobuf::Void dummy;

    channel_user.test_file_descriptors(nullptr, &dummy, &reply, google::protobuf::NewCallback([](){}));

    std::initializer_list<int> fds = {2, 3, 5};

    ASSERT_EQ(transport->sent_messages.size(), 1);
    {
        mir::protobuf::Buffer reply_message;

        for (auto fd : fds)
            reply_message.add_fd(fd);
        reply_message.set_fds_on_side_channel(fds.size());

        mir::protobuf::wire::Invocation request;
        mir::protobuf::wire::Result reply;

        request.ParseFromArray(transport->sent_messages.front().data() + sizeof(uint16_t),
                               transport->sent_messages.front().size() - sizeof(uint16_t));

        reply.set_id(request.id());
        reply.set_response(reply_message.SerializeAsString());

        ASSERT_TRUE(reply.has_id());
        ASSERT_TRUE(reply.has_response());

        std::vector<uint8_t> buffer(reply.ByteSize() + sizeof(uint16_t));
        *reinterpret_cast<uint16_t*>(buffer.data()) = htobe16(reply.ByteSize());
        ASSERT_TRUE(reply.SerializeToArray(buffer.data() + sizeof(uint16_t), buffer.size() - sizeof(uint16_t)));

        transport->add_receive_hunk(std::move(buffer), fds);
        transport->notify_data_received();
    }

    ASSERT_EQ(reply.fd_size(), fds.size());
    int i = 0;
    for (auto fd : fds)
    {
        EXPECT_EQ(reply.fd(i), fd);
        ++i;
    }
}

TEST_F(MirProtobufRpcChannelTest, NotifiesOfDisconnectOnWriteError)
{
    using namespace ::testing;

    bool disconnected{false};

    lifecycle->set_lifecycle_event_handler([&disconnected](MirLifecycleState state)
    {
        if (state == mir_lifecycle_connection_lost)
        {
            disconnected = true;
        }
    });

    EXPECT_CALL(*transport, send_data(_))
        .WillOnce(Throw(std::runtime_error("Eaten by giant space goat")));

    mir::protobuf::DisplayServer::Stub channel_user{channel.get(), mir::protobuf::DisplayServer::STUB_DOESNT_OWN_CHANNEL};
    mir::protobuf::Buffer reply;
    mir::protobuf::Void dummy;

    EXPECT_THROW(
        channel_user.test_file_descriptors(nullptr, &dummy, &reply, google::protobuf::NewCallback([](){})),
        std::runtime_error);

    EXPECT_TRUE(disconnected);
}

TEST_F(MirProtobufRpcChannelTest, ForwardsDisconnectNotification)
{
    using namespace ::testing;

    bool disconnected{false};

    lifecycle->set_lifecycle_event_handler([&disconnected](MirLifecycleState state)
    {
        if (state == mir_lifecycle_connection_lost)
        {
            disconnected = true;
        }
    });

    for(auto& observer : transport->observers)
    {
        observer->on_disconnected();
    }

    EXPECT_TRUE(disconnected);
}

TEST_F(MirProtobufRpcChannelTest, NotifiesOfDisconnectOnlyOnce)
{
    using namespace ::testing;

    bool disconnected{false};

    lifecycle->set_lifecycle_event_handler([&disconnected](MirLifecycleState state)
    {
        if (state == mir_lifecycle_connection_lost)
        {
            if (disconnected)
            {
                FAIL()<<"Received disconnected message twice";
            }
            disconnected = true;
        }
    });

    EXPECT_CALL(*transport, send_data(_))
        .WillOnce(DoAll(Throw(std::runtime_error("Eaten by giant space goat")),
                        InvokeWithoutArgs([this]()
    {
        for(auto& observer : transport->observers)
        {
            observer->on_disconnected();
        }
        return 0;
    })));

    mir::protobuf::DisplayServer::Stub channel_user{channel.get(), mir::protobuf::DisplayServer::STUB_DOESNT_OWN_CHANNEL};
    mir::protobuf::Buffer reply;
    mir::protobuf::Void dummy;

    EXPECT_THROW(
        channel_user.test_file_descriptors(nullptr, &dummy, &reply, google::protobuf::NewCallback([](){})),
        std::runtime_error);

    EXPECT_TRUE(disconnected);
}
