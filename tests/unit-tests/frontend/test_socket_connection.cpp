/*
 * Copyright © 2013, 2014 Canonical Ltd.
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
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#include "src/server/frontend/socket_connection.h"
#include "src/server/frontend/message_receiver.h"
#include "mir/frontend/message_processor.h"
#include "mir/frontend/session_credentials.h"

#include "mir_test/fake_shared.h"

#include "mir_protobuf_wire.pb.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>

namespace mf = mir::frontend;
namespace mfd = mir::frontend::detail;
namespace ba = boost::asio;
namespace mt = mir::test;

using namespace testing;

namespace
{
struct StubReceiver : public mfd::MessageReceiver
{
    StubReceiver() : async_buffer{nullptr, 0} {}

    void async_receive_msg(
        std::function<void(boost::system::error_code const&, size_t)> const& callback,
        boost::asio::mutable_buffers_1 const& buffer) override
    {
        async_buffer = buffer;
        callback_function = callback;
    }

    boost::system::error_code receive_msg(boost::asio::mutable_buffers_1 const& buffer) override
    {
        if (ba::buffer_cast<void*>(buffer) == nullptr)
            throw std::runtime_error("StubReceiver::receive_msg got null buffer");
        if (ba::buffer_size(buffer) != message.size())
            throw std::runtime_error("StubReceiver::receive_msg buffer size not equal to message size");

        memcpy(ba::buffer_cast<void*>(buffer),
               message.data(), ba::buffer_size(buffer));

        return boost::system::error_code();
    }

    size_t available_bytes() override
    {
        return message.size();
    }

    void fake_receive_msg(char* buffer, size_t size)
    {
        message.assign(buffer, buffer + size);

        ASSERT_NE(nullptr, callback_function);
        ASSERT_THAT(ba::buffer_cast<void*>(async_buffer), NotNull());
        ASSERT_THAT(message.size(), Ge(ba::buffer_size(async_buffer)));

        memcpy(ba::buffer_cast<void*>(async_buffer),
               buffer, ba::buffer_size(async_buffer));

        message.erase(
            message.begin(),
            message.begin() + ba::buffer_size(async_buffer));

        boost::system::error_code code;
        callback_function(code, size);
    }

private:
    std::function<void(boost::system::error_code const&, size_t)> callback_function;
    boost::asio::mutable_buffers_1 async_buffer;
    std::vector<char> message;

    MOCK_METHOD0(client_creds, mf::SessionCredentials());
};

struct MockProcessor : public mfd::MessageProcessor
{
    MOCK_METHOD1(dispatch, bool(mfd::Invocation const& invocation));
};
}

struct SocketConnection : public Test
{
    NiceMock<MockProcessor> mock_processor;
    StubReceiver stub_receiver;
    std::shared_ptr<mfd::Connections<mfd::SocketConnection>> null_sessions;

    mfd::SocketConnection connection{mt::fake_shared(stub_receiver), 0, null_sessions, mt::fake_shared(mock_processor)};

    void SetUp()
    {
        connection.read_next_message();
    }

    void fake_receiving_message()
    {
        int const header_size = 2;
        char buffer[512];
        mir::protobuf::wire::Invocation invocation;
        invocation.set_id(1);
        invocation.set_method_name("");
        invocation.set_parameters(buffer, 0);
        invocation.set_protocol_version(1);
        auto const body_size = invocation.ByteSize();
        buffer[0] = body_size / 0x100;
        buffer[1] = body_size % 0x100;
        invocation.SerializeToArray(buffer + header_size, sizeof buffer - header_size);

        stub_receiver.fake_receive_msg(buffer, header_size + body_size);
    }
};

TEST_F(SocketConnection, dispatches_message_on_receipt)
{
    EXPECT_CALL(mock_processor, dispatch(_)).Times(1).WillOnce(Return(true));

    fake_receiving_message();
}

TEST_F(SocketConnection, dispatches_messages_on_receipt)
{
    auto const arbitary_no_of_messages = 5;

    EXPECT_CALL(mock_processor, dispatch(_)).Times(arbitary_no_of_messages).WillRepeatedly(Return(true));

    for (int i = 0; i != arbitary_no_of_messages; ++i)
        fake_receiving_message();
}
