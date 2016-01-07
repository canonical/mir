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

#include "src/client/rpc/mir_protobuf_rpc_channel.h"
#include "src/client/rpc/stream_transport.h"
#include "src/client/rpc/mir_display_server.h"
#include "src/client/surface_map.h"
#include "src/client/display_configuration.h"
#include "src/client/rpc/null_rpc_report.h"
#include "src/client/lifecycle_control.h"
#include "src/client/ping_handler.h"

#include "mir_protobuf.pb.h"
#include "mir_protobuf_wire.pb.h"

#include "mir/test/doubles/null_client_event_sink.h"
#include "mir/test/doubles/mock_client_buffer_stream.h"
#include "mir/test/fd_utils.h"

#include <list>
#include <endian.h>

#include <sys/eventfd.h>
#include <fcntl.h>

#include <boost/throw_exception.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace mclr = mir::client::rpc;
namespace mtd = mir::test::doubles;
namespace md = mir::dispatch;
namespace mt = mir::test;

namespace
{

struct MockSurfaceMap : mcl::SurfaceMap
{
    MOCK_CONST_METHOD2(with_surface_do,
        void(mir::frontend::SurfaceId, std::function<void(MirSurface*)> const&));
    MOCK_CONST_METHOD2(with_stream_do,
        void(mir::frontend::BufferStreamId, std::function<void(mcl::ClientBufferStream*)> const&));
    MOCK_CONST_METHOD1(with_all_streams_do,
        void(std::function<void(mcl::ClientBufferStream*)> const&));
}; 
 
class StubSurfaceMap : public mcl::SurfaceMap
{
public:
    void with_surface_do(
        mir::frontend::SurfaceId, std::function<void(MirSurface*)> const&) const override
    {
    }
    void with_stream_do(
        mir::frontend::BufferStreamId, std::function<void(mcl::ClientBufferStream*)> const&) const override
    {
    }
    void with_all_streams_do(std::function<void(mcl::ClientBufferStream*)> const&) const override
    {
    }
};

class MockStreamTransport : public mclr::StreamTransport
{
public:
    MockStreamTransport()
        : event_fd{eventfd(0, EFD_CLOEXEC)}
    {
        if (event_fd == mir::Fd::invalid)
        {
            BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                     std::system_category(),
                                                     "Failed to create event fd"}));
        }

        using namespace testing;
        ON_CALL(*this, register_observer(_))
            .WillByDefault(Invoke(std::bind(&MockStreamTransport::register_observer_default,
                                            this, std::placeholders::_1)));
        ON_CALL(*this, receive_data(_,_))
            .WillByDefault(Invoke([this](void* buffer, size_t message_size)
        {
            receive_data_default(buffer, message_size);
        }));

        ON_CALL(*this, receive_data(_,_,_))
            .WillByDefault(Invoke([this](void* buffer, size_t message_size, std::vector<mir::Fd>& fds)
        {
            receive_data_default(buffer, message_size, fds);
        }));

        ON_CALL(*this, send_message(_,_))
            .WillByDefault(Invoke(std::bind(&MockStreamTransport::send_message_default,
                                            this, std::placeholders::_1)));
    }

    void add_server_message(std::vector<uint8_t> const& message)
    {
        eventfd_t data_added{message.size()};
        eventfd_write(event_fd, data_added);
        received_data.insert(received_data.end(), message.begin(), message.end());
    }
    void add_server_message(std::vector<uint8_t> const& message, std::initializer_list<mir::Fd> fds)
    {
        add_server_message(message);
        received_fds.insert(received_fds.end(), fds);
    }

    bool all_data_consumed() const
    {
        return received_data.empty() && received_fds.empty();
    }

    MOCK_METHOD1(register_observer, void(std::shared_ptr<Observer> const&));
    MOCK_METHOD1(unregister_observer, void(std::shared_ptr<Observer> const&));
    MOCK_METHOD2(receive_data, void(void*, size_t));
    MOCK_METHOD3(receive_data, void(void*, size_t, std::vector<mir::Fd>&));
    MOCK_METHOD2(send_message, void(std::vector<uint8_t> const&, std::vector<mir::Fd> const&));

    mir::Fd watch_fd() const override
    {
        return event_fd;
    }

    bool dispatch(md::FdEvents /*events*/) override
    {
        for (auto& observer : observers)
        {
            observer->on_data_available();
        }
        return true;
    }

    md::FdEvents relevant_events() const override
    {
        return md::FdEvent::readable;
    }

    // Transport interface
    void register_observer_default(std::shared_ptr<Observer> const& observer)
    {
        observers.push_back(observer);
    }

    void receive_data_default(void* buffer, size_t read_bytes)
    {
        static std::vector<mir::Fd> dummy;
        receive_data_default(buffer, read_bytes, dummy);
    }

    void receive_data_default(void* buffer, size_t read_bytes, std::vector<mir::Fd>& fds)
    {
        if (read_bytes == 0)
            return;

        auto num_fds = fds.size();
        if (read_bytes > received_data.size())
        {
            throw std::runtime_error("Attempt to read more data than is available");
        }
        if (num_fds > received_fds.size())
        {
            throw std::runtime_error("Attempt to receive more fds than are available");
        }

        memcpy(buffer, received_data.data(), read_bytes);
        fds.assign(received_fds.begin(), received_fds.begin() + num_fds);

        received_data.erase(received_data.begin(), received_data.begin() + read_bytes);
        received_fds.erase(received_fds.begin(), received_fds.begin() + num_fds);

        eventfd_t remaining_bytes;
        eventfd_read(event_fd, &remaining_bytes);
        remaining_bytes -= read_bytes;
        eventfd_write(event_fd, remaining_bytes);
    }

    void send_message_default(std::vector<uint8_t> const& buffer)
    {
        sent_messages.push_back(buffer);
    }

    std::list<std::shared_ptr<Observer>> observers;

    size_t read_offset{0};
    std::vector<uint8_t> received_data;
    std::vector<mir::Fd> received_fds;
    std::list<std::vector<uint8_t>> sent_messages;

private:
    mir::Fd event_fd;
};

class MirProtobufRpcChannelTest : public testing::Test
{
public:
    MirProtobufRpcChannelTest()
        : transport{new testing::NiceMock<MockStreamTransport>},
          lifecycle{std::make_shared<mcl::LifecycleControl>()},
          channel{new mclr::MirProtobufRpcChannel{
                  std::unique_ptr<MockStreamTransport>{transport},
                  std::make_shared<StubSurfaceMap>(),
                  std::make_shared<mcl::DisplayConfiguration>(),
                  std::make_shared<mclr::NullRpcReport>(),
                  lifecycle,
                  std::make_shared<mir::client::PingHandler>(),
                  std::make_shared<mtd::NullClientEventSink>()}}
    {
    }

    MockStreamTransport* transport;
    std::shared_ptr<mcl::LifecycleControl> lifecycle;
    std::shared_ptr<mclr::MirProtobufRpcChannel> channel;
};

}

TEST_F(MirProtobufRpcChannelTest, reads_full_messages)
{
    std::vector<uint8_t> empty_message(sizeof(uint16_t));
    std::vector<uint8_t> small_message(sizeof(uint16_t) + 8);
    std::vector<uint8_t> large_message(sizeof(uint16_t) + 4096);

    *reinterpret_cast<uint16_t*>(empty_message.data()) = htobe16(0);
    *reinterpret_cast<uint16_t*>(small_message.data()) = htobe16(8);
    *reinterpret_cast<uint16_t*>(large_message.data()) = htobe16(4096);

    transport->add_server_message(empty_message);
    transport->dispatch(md::FdEvent::readable);
    EXPECT_TRUE(transport->all_data_consumed());

    transport->add_server_message(small_message);
    transport->dispatch(md::FdEvent::readable);
    EXPECT_TRUE(transport->all_data_consumed());

    transport->add_server_message(large_message);
    transport->dispatch(md::FdEvent::readable);
    EXPECT_TRUE(transport->all_data_consumed());
}

TEST_F(MirProtobufRpcChannelTest, reads_all_queued_messages)
{
    std::vector<uint8_t> empty_message(sizeof(uint16_t));
    std::vector<uint8_t> small_message(sizeof(uint16_t) + 8);
    std::vector<uint8_t> large_message(sizeof(uint16_t) + 4096);

    *reinterpret_cast<uint16_t*>(empty_message.data()) = htobe16(0);
    *reinterpret_cast<uint16_t*>(small_message.data()) = htobe16(8);
    *reinterpret_cast<uint16_t*>(large_message.data()) = htobe16(4096);

    transport->add_server_message(empty_message);
    transport->add_server_message(small_message);
    transport->add_server_message(large_message);

    while(mt::fd_is_readable(channel->watch_fd()))
    {
        channel->dispatch(md::FdEvent::readable);
    }
    EXPECT_TRUE(transport->all_data_consumed());
}

TEST_F(MirProtobufRpcChannelTest, sends_messages_atomically)
{
    mclr::DisplayServer channel_user{channel};
    mir::protobuf::ConnectParameters message;
    message.set_application_name("I'm a little teapot!");

    channel_user.connect(&message, nullptr, nullptr);

    EXPECT_EQ(transport->sent_messages.size(), 1);
}

TEST_F(MirProtobufRpcChannelTest, sets_correct_size_when_sending_message)
{
    mclr::DisplayServer channel_user{channel};
    mir::protobuf::ConnectParameters message;
    message.set_application_name("I'm a little teapot!");

    channel_user.connect(&message, nullptr, nullptr);

    uint16_t message_header = *reinterpret_cast<uint16_t*>(transport->sent_messages.front().data());
    message_header = be16toh(message_header);
    EXPECT_EQ(transport->sent_messages.front().size() - sizeof(uint16_t), message_header);
}

TEST_F(MirProtobufRpcChannelTest, reads_fds)
{
    mclr::DisplayServer channel_user{channel};
    mir::protobuf::Buffer reply;
    mir::protobuf::BufferRequest request;

    channel_user.exchange_buffer(&request, &reply, google::protobuf::NewCallback([](){}));

    std::initializer_list<mir::Fd> fds = {mir::Fd{open("/dev/null", O_RDONLY)},
                                          mir::Fd{open("/dev/null", O_RDONLY)},
                                          mir::Fd{open("/dev/null", O_RDONLY)}};

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

        transport->add_server_message(buffer);

        // Because our protocol is a bit silly...
        std::vector<uint8_t> dummy = {1};
        transport->add_server_message(dummy, fds);

        while(mt::fd_is_readable(channel->watch_fd()))
        {
            channel->dispatch(md::FdEvent::readable);
        }
    }

    ASSERT_EQ(reply.fd_size(), fds.size());
    int i = 0;
    for (auto fd : fds)
    {
        EXPECT_EQ(reply.fd(i), fd);
        ++i;
    }
}

TEST_F(MirProtobufRpcChannelTest, notifies_streams_of_disconnect)
{
    using namespace testing;
    auto stream_map = std::make_shared<MockSurfaceMap>();
    mtd::MockClientBufferStream stream;
    EXPECT_CALL(stream, buffer_unavailable());
    EXPECT_CALL(*stream_map, with_all_streams_do(_))
       .WillOnce(InvokeArgument<0>(&stream));
 
    mclr::MirProtobufRpcChannel channel{
                  std::make_unique<NiceMock<MockStreamTransport>>(),
                  stream_map,
                  std::make_shared<mcl::DisplayConfiguration>(),
                  std::make_shared<mclr::NullRpcReport>(),
                  lifecycle,
                  std::make_shared<mir::client::PingHandler>(),
                  std::make_shared<mtd::NullClientEventSink>()};
    channel.on_disconnected();
}

TEST_F(MirProtobufRpcChannelTest, notifies_of_disconnect_on_write_error)
{
    using namespace ::testing;

    bool disconnected{false};

    lifecycle->set_callback([&disconnected](MirLifecycleState state)
    {
        if (state == mir_lifecycle_connection_lost)
        {
            disconnected = true;
        }
    });

    EXPECT_CALL(*transport, send_message(_,_))
        .WillOnce(Throw(std::runtime_error("Eaten by giant space goat")));

    mclr::DisplayServer channel_user{channel};
    mir::protobuf::Buffer reply;
    mir::protobuf::BufferRequest request;

    EXPECT_THROW(
        channel_user.exchange_buffer(&request, &reply, google::protobuf::NewCallback([](){})),
        std::runtime_error);

    EXPECT_TRUE(disconnected);
}

TEST_F(MirProtobufRpcChannelTest, forwards_disconnect_notification)
{
    using namespace ::testing;

    bool disconnected{false};

    lifecycle->set_callback([&disconnected](MirLifecycleState state)
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

TEST_F(MirProtobufRpcChannelTest, notifies_of_disconnect_only_once)
{
    using namespace ::testing;

    bool disconnected{false};

    lifecycle->set_callback([&disconnected](MirLifecycleState state)
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

    EXPECT_CALL(*transport, send_message(_,_))
        .WillOnce(DoAll(Throw(std::runtime_error("Eaten by giant space goat")),
                        InvokeWithoutArgs([this]()
    {
        for(auto& observer : transport->observers)
        {
            observer->on_disconnected();
        }
    })));

    mclr::DisplayServer channel_user{channel};
    mir::protobuf::Buffer reply;
    mir::protobuf::BufferRequest request;

    EXPECT_THROW(
        channel_user.exchange_buffer(&request, &reply, google::protobuf::NewCallback([](){})),
        std::runtime_error);

    EXPECT_TRUE(disconnected);
}

namespace
{
void set_flag(bool* flag)
{
    *flag = true;
}
}

TEST_F(MirProtobufRpcChannelTest, delays_messages_not_requested)
{
    using namespace ::testing;

    auto typed_channel = std::dynamic_pointer_cast<mclr::MirProtobufRpcChannel>(channel);

    mclr::DisplayServer channel_user{channel};
    mir::protobuf::PingEvent request;
    mir::protobuf::Void reply;

    bool first_response_called{false};
    bool second_response_called{false};
    channel_user.pong(&request,
                      &reply,
                      google::protobuf::NewCallback(&set_flag, &first_response_called));

    typed_channel->process_next_request_first();
    channel_user.pong(&request,
                      &reply,
                      google::protobuf::NewCallback(&set_flag, &second_response_called));

    mir::protobuf::wire::Invocation wire_request;
    mir::protobuf::wire::Result wire_reply;

    wire_request.ParseFromArray(transport->sent_messages.front().data() + sizeof(uint16_t),
                                transport->sent_messages.front().size() - sizeof(uint16_t));

    transport->sent_messages.pop_front();

    wire_reply.set_id(wire_request.id());
    wire_reply.set_response(reply.SerializeAsString());

    std::vector<uint8_t> buffer(wire_reply.ByteSize() + sizeof(uint16_t));
    *reinterpret_cast<uint16_t*>(buffer.data()) = htobe16(wire_reply.ByteSize());
    ASSERT_TRUE(wire_reply.SerializeToArray(buffer.data() + sizeof(uint16_t), buffer.size() - sizeof(uint16_t)));

    transport->add_server_message(buffer);

    wire_request.ParseFromArray(transport->sent_messages.front().data() + sizeof(uint16_t),
                                transport->sent_messages.front().size() - sizeof(uint16_t));

    transport->sent_messages.pop_front();

    wire_reply.set_id(wire_request.id());
    wire_reply.set_response(reply.SerializeAsString());

    buffer.resize(wire_reply.ByteSize() + sizeof(uint16_t));
    *reinterpret_cast<uint16_t*>(buffer.data()) = htobe16(wire_reply.ByteSize());
    ASSERT_TRUE(wire_reply.SerializeToArray(buffer.data() + sizeof(uint16_t), buffer.size() - sizeof(uint16_t)));

    transport->add_server_message(buffer);

    // Read the first message; this should be queued for later processing...
    EXPECT_TRUE(mt::fd_is_readable(typed_channel->watch_fd()));
    typed_channel->dispatch(md::FdEvent::readable);

    EXPECT_FALSE(first_response_called);
    EXPECT_FALSE(second_response_called);

    // Read the second message; this should be processed immediately...
    EXPECT_TRUE(mt::fd_is_readable(typed_channel->watch_fd()));
    typed_channel->dispatch(md::FdEvent::readable);

    EXPECT_FALSE(first_response_called);
    EXPECT_TRUE(second_response_called);

    // Now, the first message should be ready to be processed...
    EXPECT_TRUE(mt::fd_is_readable(typed_channel->watch_fd()));
    typed_channel->dispatch(md::FdEvent::readable);

    EXPECT_TRUE(first_response_called);
    EXPECT_TRUE(second_response_called);
}

TEST_F(MirProtobufRpcChannelTest, delays_messages_with_fds_not_requested)
{
    using namespace ::testing;

    auto typed_channel = std::dynamic_pointer_cast<mclr::MirProtobufRpcChannel>(channel);

    mclr::DisplayServer channel_user{channel};
    mir::protobuf::PingEvent pong_request;
    mir::protobuf::Void pong_reply;

    mir::protobuf::Buffer buffer_reply;
    mir::protobuf::BufferRequest buffer_request;

    bool first_response_called{false};
    bool second_response_called{false};


    channel_user.exchange_buffer(&buffer_request,
                                 &buffer_reply,
                                 google::protobuf::NewCallback(&set_flag, &first_response_called));

    typed_channel->process_next_request_first();
    channel_user.pong(&pong_request,
                      &pong_reply,
                      google::protobuf::NewCallback(&set_flag, &second_response_called));


    std::initializer_list<mir::Fd> fds = {mir::Fd{open("/dev/null", O_RDONLY)},
                                          mir::Fd{open("/dev/null", O_RDONLY)},
                                          mir::Fd{open("/dev/null", O_RDONLY)}};

    {
        mir::protobuf::Buffer reply_message;

        for (auto fd : fds)
            reply_message.add_fd(fd);
        reply_message.set_fds_on_side_channel(fds.size());

        mir::protobuf::wire::Invocation request;
        mir::protobuf::wire::Result reply;

        request.ParseFromArray(transport->sent_messages.front().data() + sizeof(uint16_t),
                               transport->sent_messages.front().size() - sizeof(uint16_t));

        transport->sent_messages.pop_front();

        reply.set_id(request.id());
        reply.set_response(reply_message.SerializeAsString());

        ASSERT_TRUE(reply.has_id());
        ASSERT_TRUE(reply.has_response());

        std::vector<uint8_t> buffer(reply.ByteSize() + sizeof(uint16_t));
        *reinterpret_cast<uint16_t*>(buffer.data()) = htobe16(reply.ByteSize());
        ASSERT_TRUE(reply.SerializeToArray(buffer.data() + sizeof(uint16_t), buffer.size() - sizeof(uint16_t)));

        transport->add_server_message(buffer);

        // Because our protocol is a bit silly...
        std::vector<uint8_t> dummy = {1};
        transport->add_server_message(dummy, fds);
    }

    {
        mir::protobuf::Void reply;

        mir::protobuf::wire::Invocation wire_request;
        mir::protobuf::wire::Result wire_reply;

        wire_request.ParseFromArray(transport->sent_messages.front().data() + sizeof(uint16_t),
                                    transport->sent_messages.front().size() - sizeof(uint16_t));

        transport->sent_messages.pop_front();

        wire_reply.set_id(wire_request.id());
        wire_reply.set_response(reply.SerializeAsString());

        std::vector<uint8_t> buffer(wire_reply.ByteSize() + sizeof(uint16_t));
        *reinterpret_cast<uint16_t*>(buffer.data()) = htobe16(wire_reply.ByteSize());
        ASSERT_TRUE(wire_reply.SerializeToArray(buffer.data() + sizeof(uint16_t), buffer.size() - sizeof(uint16_t)));

        transport->add_server_message(buffer);
    }

    // Read the first message; this should be queued for later processing...
    EXPECT_TRUE(mt::fd_is_readable(typed_channel->watch_fd()));
    typed_channel->dispatch(md::FdEvent::readable);

    EXPECT_FALSE(first_response_called);
    EXPECT_FALSE(second_response_called);

    // Read the second message; this should be processed immediately...
    EXPECT_TRUE(mt::fd_is_readable(typed_channel->watch_fd()));
    typed_channel->dispatch(md::FdEvent::readable);

    EXPECT_FALSE(first_response_called);
    EXPECT_TRUE(second_response_called);

    // Now, the first message should be ready to be processed...
    EXPECT_TRUE(mt::fd_is_readable(typed_channel->watch_fd()));
    typed_channel->dispatch(md::FdEvent::readable);

    EXPECT_TRUE(first_response_called);
    EXPECT_TRUE(second_response_called);
}

