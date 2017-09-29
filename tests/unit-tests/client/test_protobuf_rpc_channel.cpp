/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/client/rpc/mir_protobuf_rpc_channel.h"
#include "src/client/rpc/stream_transport.h"
#include "src/client/rpc/mir_display_server.h"
#include "src/client/display_configuration.h"
#include "src/client/rpc/null_rpc_report.h"
#include "src/client/lifecycle_control.h"
#include "src/client/ping_handler.h"
#include "src/client/buffer_factory.h"

#include "mir/variable_length_array.h"
#include "mir_protobuf.pb.h"
#include "mir_protobuf_wire.pb.h"
#include "mir/client/surface_map.h"
#include "mir/input/input_devices.h"

#include "mir/input/null_input_receiver_report.h"
#include "mir/test/doubles/null_client_event_sink.h"
#include "mir/test/doubles/mock_mir_buffer_stream.h"
#include "mir/test/doubles/mock_client_buffer.h"
#include "mir/test/fd_utils.h"
#include "mir/test/gmock_fixes.h"

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
    MOCK_CONST_METHOD1(surface, std::shared_ptr<MirWindow>(mir::frontend::SurfaceId));
    MOCK_CONST_METHOD1(stream, std::shared_ptr<MirBufferStream>(mir::frontend::BufferStreamId));
    MOCK_CONST_METHOD1(with_all_streams_do,
        void(std::function<void(MirBufferStream*)> const&));
    MOCK_CONST_METHOD1(buffer, std::shared_ptr<mcl::MirBuffer>(int));
    MOCK_METHOD2(insert, void(int, std::shared_ptr<mcl::MirBuffer> const&));
    MOCK_METHOD2(insert, void(mir::frontend::BufferStreamId, std::shared_ptr<MirPresentationChain> const&));
    MOCK_METHOD1(erase, void(int));
    MOCK_CONST_METHOD1(with_all_windows_do,
        void(std::function<void(MirWindow*)> const&));
};
 
class StubSurfaceMap : public mcl::SurfaceMap
{
public:
    std::shared_ptr<MirWindow> surface(mir::frontend::SurfaceId) const override
    {
        return {};
    }
    std::shared_ptr<MirBufferStream> stream(mir::frontend::BufferStreamId) const override
    {
        return {};
    }
    void with_all_streams_do(std::function<void(MirBufferStream*)> const&) const override
    {
    }
    void insert(mir::frontend::BufferStreamId, std::shared_ptr<MirPresentationChain> const&)
    {
    }
    std::shared_ptr<mcl::MirBuffer> buffer(int) const override
    {
        return nullptr;
    }
    void insert(int, std::shared_ptr<mcl::MirBuffer> const&) override
    {
    }
    void erase(int) override
    {
    }
    void with_all_windows_do(std::function<void(MirWindow*)> const&) const override
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
          surface_map{std::make_shared<StubSurfaceMap>()},
          channel{new mclr::MirProtobufRpcChannel{
                  std::unique_ptr<MockStreamTransport>{transport},
                  surface_map,
                  std::make_shared<mcl::BufferFactory>(),
                  std::make_shared<mcl::DisplayConfiguration>(),
                  std::make_shared<mir::input::InputDevices>(surface_map),
                  std::make_shared<mclr::NullRpcReport>(),
                  std::make_shared<mir::input::receiver::NullInputReceiverReport>(),
                  lifecycle,
                  std::make_shared<mir::client::PingHandler>(),
                  std::make_shared<mir::client::ErrorHandler>(),
                  std::make_shared<mtd::NullClientEventSink>()}}
    {
    }

    MockStreamTransport* transport;
    std::shared_ptr<mcl::LifecycleControl> lifecycle;
    std::shared_ptr<mcl::SurfaceMap> surface_map;
    std::shared_ptr<mclr::MirProtobufRpcChannel> channel;
    mtd::MockMirBufferStream stream;
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

    EXPECT_EQ(transport->sent_messages.size(), 1u);
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
    mir::protobuf::PlatformOperationMessage reply;
    mir::protobuf::PlatformOperationMessage request;

    channel_user.platform_operation(&request, &reply, google::protobuf::NewCallback([](){}));

    std::initializer_list<mir::Fd> fds = {mir::Fd{open("/dev/null", O_RDONLY)},
                                          mir::Fd{open("/dev/null", O_RDONLY)},
                                          mir::Fd{open("/dev/null", O_RDONLY)}};

    ASSERT_EQ(transport->sent_messages.size(), 1u);
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

    ASSERT_EQ(static_cast<size_t>(reply.fd_size()), fds.size());
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
    EXPECT_CALL(stream, buffer_unavailable()).Times(AtLeast(1));
    EXPECT_CALL(*stream_map, with_all_streams_do(_)).Times(AtLeast(1))
       .WillRepeatedly(InvokeArgument<0>(&stream));

    mclr::MirProtobufRpcChannel channel{
                  std::make_unique<NiceMock<MockStreamTransport>>(),
                  stream_map,
                  std::make_shared<mcl::BufferFactory>(),
                  std::make_shared<mcl::DisplayConfiguration>(),
                  std::make_shared<mir::input::InputDevices>(stream_map),
                  std::make_shared<mclr::NullRpcReport>(),
                  std::make_shared<mir::input::receiver::NullInputReceiverReport>(),
                  lifecycle,
                  std::make_shared<mir::client::PingHandler>(),
                  std::make_shared<mir::client::ErrorHandler>(),
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
    mir::protobuf::Void reply;
    mir::protobuf::BufferRequest request;

    EXPECT_THROW(
        channel_user.submit_buffer(&request, &reply, google::protobuf::NewCallback([](){})),
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
    mir::protobuf::Void reply;
    mir::protobuf::BufferRequest request;

    EXPECT_THROW(
        channel_user.submit_buffer(&request, &reply, google::protobuf::NewCallback([](){})),
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

    mir::protobuf::PlatformOperationMessage buffer_reply;
    mir::protobuf::PlatformOperationMessage buffer_request;

    bool first_response_called{false};
    bool second_response_called{false};

    channel_user.platform_operation(&buffer_request, &buffer_reply,
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

struct MockBufferFactory : mcl::AsyncBufferFactory
{
    MOCK_METHOD1(cancel_requests_with_context, void(void*));
    MOCK_METHOD1(generate_buffer, std::unique_ptr<mcl::MirBuffer>(mir::protobuf::Buffer const&));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MOCK_METHOD7(expect_buffer, void(
        std::shared_ptr<mcl::ClientBufferFactory> const&,
        MirConnection*,
        mir::geometry::Size, MirPixelFormat, MirBufferUsage,
        MirBufferCallback, void*));
#pragma GCC diagnostic pop
    MOCK_METHOD7(expect_buffer, void(
        std::shared_ptr<mcl::ClientBufferFactory> const&,
        MirConnection*, mir::geometry::Size, uint32_t, uint32_t,
        MirBufferCallback, void*));
};

namespace
{
void buffer_cb(MirBuffer*, void*)
{
}

void set_async_buffer_message(
    mir::protobuf::EventSequence& seq,
    MockStreamTransport& transport)
{
    std::vector<uint8_t> send_buffer(static_cast<size_t>(seq.ByteSize()));
    seq.SerializeToArray(send_buffer.data(), send_buffer.size());
    mir::protobuf::wire::Result result;
    result.add_events(send_buffer.data(), send_buffer.size());
    send_buffer.resize(result.ByteSize());
    result.SerializeToArray(send_buffer.data(), send_buffer.size());

    size_t header_size = 2u;
    std::vector<uint8_t> header(header_size);
    header.data()[0] = static_cast<unsigned char>((result.ByteSize() >> 8) & 0xff);
    header.data()[1] = static_cast<unsigned char>((result.ByteSize() >> 0) & 0xff);
    transport.add_server_message(header);
    transport.add_server_message(send_buffer);
}

}
TEST_F(MirProtobufRpcChannelTest, creates_buffer_if_not_in_map)
{
    using namespace testing;
    int buffer_id(3);
    auto stream_map = std::make_shared<MockSurfaceMap>();
    auto mock_buffer_factory = std::make_shared<MockBufferFactory>();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    EXPECT_CALL(*mock_buffer_factory, generate_buffer(_))
        .WillOnce(InvokeWithoutArgs([&]{
            return std::make_unique<mcl::Buffer>(
                buffer_cb, nullptr, buffer_id, nullptr, nullptr, mir_buffer_usage_software);
            }));
#pragma GCC diagnostic pop
    EXPECT_CALL(*stream_map, insert(buffer_id, _));

    auto transport = std::make_unique<NiceMock<MockStreamTransport>>();
    mir::protobuf::EventSequence seq;
    auto request = seq.mutable_buffer_request();
    request->set_operation(mir::protobuf::BufferOperation::add);
    request->mutable_buffer()->set_buffer_id(buffer_id);
    set_async_buffer_message(seq, *transport);

    mclr::MirProtobufRpcChannel channel{
                  std::move(transport),
                  stream_map,
                  mock_buffer_factory,
                  std::make_shared<mcl::DisplayConfiguration>(),
                  std::make_shared<mir::input::InputDevices>(stream_map),
                  std::make_shared<mclr::NullRpcReport>(),
                  std::make_shared<mir::input::receiver::NullInputReceiverReport>(),
                  lifecycle,
                  std::make_shared<mir::client::PingHandler>(),
                  std::make_shared<mir::client::ErrorHandler>(),
                  std::make_shared<mtd::NullClientEventSink>()};

    channel.on_data_available();
}

TEST_F(MirProtobufRpcChannelTest, reuses_buffer_if_in_map)
{
    using namespace testing;
    int buffer_id(3);
    auto stream_map = std::make_shared<MockSurfaceMap>();
    auto mock_buffer_factory = std::make_shared<MockBufferFactory>();
    auto mock_client_buffer = std::make_shared<mtd::MockClientBuffer>();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto buf = std::make_shared<mcl::Buffer>(buffer_cb, nullptr, buffer_id, mock_client_buffer, nullptr, mir_buffer_usage_software);
#pragma GCC diagnostic pop
    EXPECT_CALL(*stream_map, buffer(buffer_id)).Times(1)
       .WillOnce(Return(buf));
    EXPECT_CALL(*mock_client_buffer, update_from(_))
        .Times(1);

    auto transport = std::make_unique<NiceMock<MockStreamTransport>>();
    mir::protobuf::EventSequence seq;
    auto request = seq.mutable_buffer_request();
    request->set_operation(mir::protobuf::BufferOperation::update);
    request->mutable_buffer()->set_buffer_id(buffer_id);
    set_async_buffer_message(seq, *transport);

    mclr::MirProtobufRpcChannel channel{
                  std::move(transport),
                  stream_map,
                  mock_buffer_factory,
                  std::make_shared<mcl::DisplayConfiguration>(),
                  std::make_shared<mir::input::InputDevices>(stream_map),
                  std::make_shared<mclr::NullRpcReport>(),
                  std::make_shared<mir::input::receiver::NullInputReceiverReport>(),
                  lifecycle,
                  std::make_shared<mir::client::PingHandler>(),
                  std::make_shared<mir::client::ErrorHandler>(),
                  std::make_shared<mtd::NullClientEventSink>()};
    channel.on_data_available();
}

TEST_F(MirProtobufRpcChannelTest, sends_incoming_buffer_to_stream_if_stream_id_present)
{
    using namespace testing;
    int buffer_id(3);
    int stream_id = 331;
    auto stream_map = std::make_shared<MockSurfaceMap>();
    auto mock_buffer_factory = std::make_shared<MockBufferFactory>();
    auto mock_client_buffer = std::make_shared<mtd::MockClientBuffer>();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto buf = std::make_shared<mcl::Buffer>(buffer_cb, nullptr, buffer_id, mock_client_buffer, nullptr, mir_buffer_usage_software);
#pragma GCC diagnostic pop
    EXPECT_CALL(*stream_map, stream(mir::frontend::BufferStreamId{stream_id}))
        .Times(1);

    auto transport = std::make_unique<NiceMock<MockStreamTransport>>();
    mir::protobuf::EventSequence seq;
    auto request = seq.mutable_buffer_request();
    request->mutable_id()->set_value(stream_id); 
    request->mutable_buffer()->set_buffer_id(buffer_id);
    set_async_buffer_message(seq, *transport);

    mclr::MirProtobufRpcChannel channel{
                  std::move(transport),
                  stream_map,
                  mock_buffer_factory,
                  std::make_shared<mcl::DisplayConfiguration>(),
                  std::make_shared<mir::input::InputDevices>(stream_map),
                  std::make_shared<mclr::NullRpcReport>(),
                  std::make_shared<mir::input::receiver::NullInputReceiverReport>(),
                  lifecycle,
                  std::make_shared<mir::client::PingHandler>(),
                  std::make_shared<mir::client::ErrorHandler>(),
                  std::make_shared<mtd::NullClientEventSink>()};
    channel.on_data_available();
}

TEST_F(MirProtobufRpcChannelTest, ignores_update_message_for_unknown_buffer)
{
    mir::protobuf::EventSequence seq;
    auto request = seq.mutable_buffer_request();
    request->mutable_buffer()->set_buffer_id(42);
    request->set_operation(mir::protobuf::BufferOperation::update);

    set_async_buffer_message(seq, *transport);

    channel->on_data_available();
}

TEST_F(MirProtobufRpcChannelTest, ignores_delete_message_for_unknown_buffer)
{
    mir::protobuf::EventSequence seq;
    auto request = seq.mutable_buffer_request();
    request->mutable_buffer()->set_buffer_id(42);
    request->set_operation(mir::protobuf::BufferOperation::remove);

    set_async_buffer_message(seq, *transport);

    channel->on_data_available();
}
