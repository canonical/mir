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

#include "src/server/frontend/socket_session.h"
#include "src/server/frontend/message_receiver.h"
#include "mir/frontend/message_processor.h"

#include "mir_test/fake_shared.h"

#include "mir_protobuf_wire.pb.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mfd = mir::frontend::detail;
namespace ba = boost::asio;
namespace mt = mir::test;

namespace
{
struct StubReceiver : public mfd::MessageReceiver
{
    void async_receive_msg(
        std::function<void(boost::system::error_code const&, size_t)> const& callback,
        boost::asio::streambuf& stream,
        size_t size)
    {
        read_size = size;
        pstream = &stream;
        callback_function = callback;
    }

    void fake_receive_msg(char* buffer, size_t size)
    {
        using namespace testing;
        ASSERT_NE(nullptr, callback_function);
        ASSERT_THAT(pstream, NotNull());
        ASSERT_THAT(read_size, Eq(size));

        pstream->sputn(buffer, size);
        pstream->commit(size);

        boost::system::error_code code;
        callback_function(code, size);
    }

private:
    std::function<void(boost::system::error_code const&, size_t)> callback_function;
    boost::asio::streambuf* pstream = nullptr;
    size_t read_size = 0;

    MOCK_METHOD0(client_pid, pid_t());
};

struct MockProcessor : public mfd::MessageProcessor
{
    MOCK_METHOD1(dispatch, bool(mfd::Invocation const& invocation));
};
}
struct SocketSessionTest : public ::testing::Test
{
    testing::NiceMock<MockProcessor> mock_processor;
    StubReceiver stub_receiver;
};

TEST_F(SocketSessionTest, basic_msg_is_received_and_dispatched)
{
    int const header_size = 2;
    char buffer[512];
    mir::protobuf::wire::Invocation invocation;
    invocation.set_id(1);
    invocation.set_method_name("");
    invocation.set_parameters(buffer, 0);
    invocation.set_protocol_version(1);
    auto const body_size = invocation.ByteSize();

    using namespace testing;

    std::shared_ptr<mfd::ConnectedSessions<mfd::SocketSession>> null_sessions;

    mfd::SocketSession session(mt::fake_shared(stub_receiver), 0, null_sessions, mt::fake_shared(mock_processor));

    EXPECT_CALL(mock_processor, dispatch(_)).Times(1).WillOnce(Return(true));

    session.read_next_message();

    buffer[0] = body_size / 0x100;
    buffer[1] = body_size % 0x100;
    stub_receiver.fake_receive_msg(buffer, header_size);

    invocation.SerializeToArray(buffer, sizeof buffer);
    stub_receiver.fake_receive_msg(buffer, body_size);
}
