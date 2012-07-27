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

namespace
{
struct SessionSignalCollector
{
    SessionSignalCollector() : session_count(0)
    {
    }

    SessionSignalCollector(SessionSignalCollector const &) = delete;

    void on_new_session(std::shared_ptr<mf::Session> const& new_session)
    {
        std::cout << "DEBUG on_new_session" << std::endl;
        std::unique_lock<std::mutex> ul(guard);
        session_count++;
        session = new_session;
        session_ids.insert(session->id());

        std::cout << "DEBUG session_count=" << session_count << std::endl;
        wait_condition.notify_one();
    }

    std::mutex guard;
    std::condition_variable wait_condition;
    int session_count;
    std::shared_ptr<mf::Session> session;
    std::set<int> session_ids;
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
        comm.signal_new_session().connect(
                std::bind(
                    &SessionSignalCollector::on_new_session,
                    &collector,
                    std::placeholders::_1));

        comm.start();

    }

    void TearDown()
    {

    }

    SessionSignalCollector collector;

    mf::ProtobufAsioCommunicator comm;
};
}

TEST_F(ProtobufAsioCommunicatorTestFixture, connection_results_in_a_callback)
{
    boost::asio::io_service io_service;
    boost::asio::local::stream_protocol::socket socket(io_service);

    socket.connect(socket_name());

    std::unique_lock<std::mutex> ul(collector.guard);

    while (collector.session_count == 0)
        collector.wait_condition.wait_for(ul, std::chrono::milliseconds(50));

    EXPECT_EQ(1, collector.session_count);
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
        a_connection_attempt_results_in_a_session_being_created)
{
    boost::asio::io_service io_service;
    boost::asio::local::stream_protocol::socket socket(io_service);

    socket.connect(socket_name());

    std::unique_lock<std::mutex> ul(collector.guard);

    while (collector.session_count == 0)
        collector.wait_condition.wait_for(ul, std::chrono::milliseconds(50));

    EXPECT_TRUE(collector.session.get());
}

TEST_F(ProtobufAsioCommunicatorTestFixture,
        each_connection_attempt_results_in_a_session_being_created_with_a_distinct_id)
{
    int const connections{5};

    boost::asio::io_service io_service;

    for (int i = 0; i != connections; ++i)
    {
        std::cout << "DEBUG entering loop" << std::endl;
        boost::asio::local::stream_protocol::socket socket(io_service);

        socket.connect(socket_name());

        std::unique_lock<std::mutex> ul(collector.guard);
        std::cout << "DEBUG i=" << i << std::endl;
        while (collector.session_count == i)
            collector.wait_condition.wait_for(ul, std::chrono::milliseconds(50));
    }

    EXPECT_EQ(connections, collector.session_count);
    EXPECT_EQ(connections, (int)collector.session_ids.size());
}
