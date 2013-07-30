/*
 * Copyright © 2013 Canonical Ltd.
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

#include "src/server/frontend/socket_session.h"
#include "src/server/frontend/message_receiver.h"
#include "src/server/frontend/message_processor.h"

#include "mir_test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mfd = mir::frontend::detail;
namespace ba = boost::asio;
namespace mt = mir::test;

namespace
{
struct MockReceiver : public mfd::MessageReceiver
{
    MOCK_METHOD3(async_receive_msg, void(std::function<void(boost::system::error_code const&, size_t)> const&,
                                         boost::asio::streambuf&, size_t)); 
    MOCK_METHOD0(client_pid, pid_t());
};

struct MockProcessor : public mfd::MessageProcessor
{
    MOCK_METHOD1(process_message, bool(std::istream&));
};
}
struct SocketSessionTest : public ::testing::Test
{
    testing::NiceMock<MockProcessor> mock_processor;
    testing::NiceMock<MockReceiver> mock_receiver;
};

TEST_F(SocketSessionTest, basic_msg)
{
    using namespace testing;

    std::shared_ptr<mfd::ConnectedSessions<mfd::SocketSession>> null_sessions;
    std::function<void(boost::system::error_code const&, size_t)> header_read, body_read;

    size_t header_size = 2;
    EXPECT_CALL(mock_receiver, async_receive_msg(_,_, header_size))
        .Times(1)
        .WillOnce(SaveArg<0>(&header_read));

    mfd::SocketSession session(mt::fake_shared(mock_receiver), 0, null_sessions, mt::fake_shared(mock_processor));

    //trigger wait for header
    session.read_next_message();
    testing::Mock::VerifyAndClearExpectations(&mock_receiver);

    //trigger body read
    EXPECT_CALL(mock_receiver, async_receive_msg(_,_,_))
        .Times(1)
        .WillOnce(SaveArg<0>(&body_read));

    boost::system::error_code code;
    header_read(code, 2);
 
    testing::Mock::VerifyAndClearExpectations(&mock_receiver);

    //trigger message process
    EXPECT_CALL(mock_processor, process_message(_))
        .Times(1)
        .WillOnce(Return(true));
    body_read(code, 9);
}
