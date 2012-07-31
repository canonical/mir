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

#include "mir/frontend/protobuf_asio_communicator.h"
#include <gtest/gtest.h>

namespace mf = mir::frontend;

namespace ba = boost::asio;
namespace bs = boost::system;

namespace
{
struct SessionEventCollector
{
    SessionEventCollector() : session_count(0)
    {
    }

    SessionEventCollector(SessionEventCollector const &) = delete;

    void on_session_event(std::shared_ptr<mf::Session> const& session, mf::SessionEvent event)
    {
        std::unique_lock<std::mutex> ul(guard);
        if (event == mf::SessionEvent::connected)
        {
            ++session_count;
            sessions.insert(session);
        }
        else if (event == mf::SessionEvent::disconnected)
        {
            --session_count;
            EXPECT_EQ(sessions.erase(session), 1u);
        }
        else
        {
            FAIL() << "unknown session event!";
        }
        wait_condition.notify_one();
    }

    std::mutex guard;
    std::condition_variable wait_condition;
    int session_count;
    std::set<std::shared_ptr<mf::Session>> sessions;
};

struct ProtobufAsioCommunicatorTestFixture : public ::testing::Test
{
    static const std::string& socket_name()
    {
        static std::string socket_name("/tmp/mir_test_pb_asio_socket");
        return socket_name;
    }

    ProtobufAsioCommunicatorTestFixture() : comm(socket_name())
    {
    }

    void SetUp()
    {
        comm.signal_session_event().connect(
                boost::bind(
                    &SessionEventCollector::on_session_event,
                    &collector, _1, _2));

        comm.start();
    }

    mf::ProtobufAsioCommunicator comm;
    SessionEventCollector collector;
};
}

TEST_F(ProtobufAsioCommunicatorTestFixture, connection_results_in_a_callback)
{
    ba::io_service io_service;
    ba::local::stream_protocol::socket socket(io_service);

    socket.connect(socket_name());

    std::unique_lock<std::mutex> ul(collector.guard);

    while (collector.session_count == 0)
        collector.wait_condition.wait_for(ul, std::chrono::milliseconds(50));

    EXPECT_EQ(1, collector.session_count);
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
        a_connection_attempt_results_in_a_session_being_created)
{
    ba::io_service io_service;
    ba::local::stream_protocol::socket socket(io_service);

    socket.connect(socket_name());

    std::unique_lock<std::mutex> ul(collector.guard);

    while (collector.session_count == 0)
        collector.wait_condition.wait_for(ul, std::chrono::milliseconds(50));

    EXPECT_FALSE(collector.sessions.empty());
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
       each_connection_attempt_results_in_a_new_session_being_created)
{
    int const connection_count{5};

    ba::io_service io_service;

    for (int i = 0; i != connection_count; ++i)
    {
        ba::local::stream_protocol::socket socket(io_service);

        socket.connect(socket_name());

        std::unique_lock<std::mutex> ul(collector.guard);
        while (collector.session_count == i)
            collector.wait_condition.wait_for(ul, std::chrono::milliseconds(50));
    }

    EXPECT_EQ(connection_count, collector.session_count);
    EXPECT_EQ(connection_count, (int)collector.sessions.size());
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
       connect_then_disconnect_a_session)
{
    ba::io_service io_service;
    ba::local::stream_protocol::socket socket(io_service);

    socket.connect(socket_name());
    
    std::unique_lock<std::mutex> ul(collector.guard);
    while (collector.session_count == 0)
        collector.wait_condition.wait_for(ul, std::chrono::milliseconds(50));

    EXPECT_EQ(collector.session_count, 1);

    bs::error_code error;
    ba::write(socket, ba::buffer("disconnect\n"), error);

    EXPECT_FALSE(error);
    int ntries = 20;
    while (ntries-- != 0 && collector.session_count == 1)
        collector.wait_condition.wait_for(ul, std::chrono::milliseconds(50));

    EXPECT_EQ(collector.session_count, 0);
}

