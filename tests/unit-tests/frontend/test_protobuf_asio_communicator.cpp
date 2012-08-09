/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/chrono/chrono.h"
#include "mir/frontend/protobuf_asio_communicator.h"
#include "mir/thread/all.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace mf = mir::frontend;

namespace ba = boost::asio;
namespace bs = boost::system;

using ba::local::stream_protocol;

namespace mir
{
struct SessionStateCollector
{
    SessionStateCollector()
        : session_count(0)
        , error_count(0)
    {
    }

    SessionStateCollector(SessionStateCollector const &) = delete;

    void on_session_state_change(std::shared_ptr<mf::Session> const& session, mf::SessionState state)
    {
      std::unique_lock<std::mutex> ul(guard);
        switch (state)
        {
        case mf::SessionState::connected:
            ++session_count;
            sessions.insert(session);
            break;
        case mf::SessionState::disconnected:
            --session_count;
            EXPECT_EQ(sessions.erase(session), 1u);
        case mf::SessionState::error:
            ++error_count;
            break;
        default:
            FAIL() << "unknown session state!";
        }
        wait_condition.notify_one();
    }

    std::mutex guard;
    std::condition_variable wait_condition;
    int session_count;
    int error_count;
    std::set<std::shared_ptr<mf::Session>> sessions;
};

struct ProtobufAsioCommunicatorTestFixture : public ::testing::Test
{
    static const std::string& socket_name()
    {
        static std::string socket_name("./mir_test_pb_asio_socket");
        return socket_name;
    }

    ProtobufAsioCommunicatorTestFixture() : comm(socket_name())
    {
    }

    void SetUp()
    {
        comm.signal_session_state().connect(
                boost::bind(
                    &SessionStateCollector::on_session_state_change,
                    &collector, _1, _2));

        comm.start();
    }

    void expect_session_count(int expected_count)
    {
	std::unique_lock<std::mutex> ul(collector.guard);
        for (int ntries = 20;
             ntries-- != 0 && collector.session_count != expected_count; )
        {
	    // This might wait for an infinite amount of time but
	    // we capture this case in terms of specifying a timeout
	    // on the tests using the fixture.
            collector.wait_condition.wait(ul);
        }
        EXPECT_EQ(collector.session_count, expected_count);
    }

    ba::io_service io_service;
    mf::ProtobufAsioCommunicator comm;
    SessionStateCollector collector;
};

TEST_F(ProtobufAsioCommunicatorTestFixture, connection_results_in_a_callback)
{
    stream_protocol::socket socket(io_service);

    socket.connect(socket_name());
    expect_session_count(1);
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
        a_connection_attempt_results_in_a_session_being_created)
{
    stream_protocol::socket socket(io_service);

    socket.connect(socket_name());
    expect_session_count(1);

    EXPECT_FALSE(collector.sessions.empty());
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
       each_connection_attempt_results_in_a_new_session_being_created)
{
    int const connection_count{5};

    for (int i = 0; i != connection_count; ++i)
    {
        stream_protocol::socket socket(io_service);
        socket.connect(socket_name());
    }

    expect_session_count(connection_count);
    EXPECT_EQ(connection_count, (int)collector.sessions.size());
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
       connect_then_disconnect_a_session)
{
    stream_protocol::socket socket(io_service);

    socket.connect(socket_name());

    expect_session_count(1);

    bs::error_code error;
    ba::write(socket, ba::buffer(std::string("disconnect\n")), error);
    EXPECT_FALSE(error);

    expect_session_count(0);
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
       double_disconnection_attempt_has_no_effect)
{
    stream_protocol::socket socket(io_service);
    socket.connect(socket_name());
    expect_session_count(1);

    bs::error_code error;
    ba::write(socket, ba::buffer(std::string("disconnect\n")), error);
    EXPECT_FALSE(error);
    expect_session_count(0);

    ba::write(socket, ba::buffer(std::string("disconnect\n")), error);
    expect_session_count(0);
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
       connect_then_disconnect_multiple_sessions)
{
    typedef std::unique_ptr<stream_protocol::socket> SocketPtr;
    typedef std::vector<SocketPtr> Sockets;

    Sockets sockets;
    int const connection_count{5};

    for (int i = 0; i != connection_count; ++i)
    {
        sockets.push_back(SocketPtr(new stream_protocol::socket(io_service)));
        sockets.back()->connect(socket_name());
    }

    expect_session_count(connection_count);

    for (Sockets::iterator s = sockets.begin(); s != sockets.end(); ++s)
    {
        bs::error_code error;
        ba::write(**s, ba::buffer(std::string("disconnect\n")), error);
        EXPECT_FALSE(error);
    }
    sockets.clear();

    expect_session_count(0);
}
}

namespace
{
// Synchronously writes the message to the socket one character at a time.
void write_fragmented_message(stream_protocol::socket & socket, std::string const & message)
{
    bs::error_code error;

    for (size_t i = 0, n = message.size(); i != n; ++i)
    {
        ba::write(socket, ba::buffer(message.substr(i, 1)), error);
        EXPECT_FALSE(error);
    }
}
}

namespace mir
{
TEST_F(ProtobufAsioCommunicatorTestFixture,
       connect_then_disconnect_a_session_with_a_fragmented_message)
{
    stream_protocol::socket socket(io_service);

    socket.connect(socket_name());
    expect_session_count(1);
    write_fragmented_message(socket, "disconnect\n");
    expect_session_count(0);
}
}
