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
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_TEST_CLIENT_H_
#define MIR_TEST_TEST_CLIENT_H_

#include "mir_test/mock_logger.h"

namespace mir
{
namespace test
{
struct TestClient
{
    TestClient(std::string socket_file) :
        logger(std::make_shared<MockLogger>()),
        channel(socket_file, logger),
        display_server(&channel),
        connect_done_called(false),
        create_surface_called(false),
        next_buffer_called(false),
        disconnect_done_called(false),
        tfd_done_called(false),
        connect_done_count(0),
        create_surface_done_count(0),
        disconnect_done_count(0)
    {
        surface_parameters.set_width(640);
        surface_parameters.set_height(480);
        surface_parameters.set_pixel_format(0);

        ON_CALL(*this, connect_done()).WillByDefault(testing::Invoke(this, &TestClient::on_connect_done));
        ON_CALL(*this, create_surface_done()).WillByDefault(testing::Invoke(this, &TestClient::on_create_surface_done));
        ON_CALL(*this, next_buffer_done()).WillByDefault(testing::Invoke(this, &TestClient::on_next_buffer_done));
        ON_CALL(*this, disconnect_done()).WillByDefault(testing::Invoke(this, &TestClient::on_disconnect_done));
    }

    std::shared_ptr<MockLogger> logger;
    mir::client::MirRpcChannel channel;
    mir::protobuf::DisplayServer::Stub display_server;
    mir::protobuf::ConnectParameters connect_parameters;
    mir::protobuf::SurfaceParameters surface_parameters;
    mir::protobuf::Surface surface;
    mir::protobuf::Void ignored;
    mir::protobuf::Connection connection;

    MOCK_METHOD0(connect_done, void ());
    MOCK_METHOD0(create_surface_done, void ());
    MOCK_METHOD0(next_buffer_done, void ());
    MOCK_METHOD0(disconnect_done, void ());

    void on_connect_done()
    {
        connect_done_called.store(true);

        auto old = connect_done_count.load();

        while (!connect_done_count.compare_exchange_weak(old, old+1));
    }

    void on_create_surface_done()
    {
        create_surface_called.store(true);

        auto old = create_surface_done_count.load();

        while (!create_surface_done_count.compare_exchange_weak(old, old+1));
    }

    void on_next_buffer_done()
    {
        next_buffer_called.store(true);
    }

    void on_disconnect_done()
    {
        disconnect_done_called.store(true);

        auto old = disconnect_done_count.load();

        while (!disconnect_done_count.compare_exchange_weak(old, old+1));
    }

    void wait_for_connect_done()
    {
        for (int i = 0; !connect_done_called.load() && i < 100; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::this_thread::yield();
        }
        connect_done_called.store(false);
    }

    void wait_for_create_surface()
    {
        for (int i = 0; !create_surface_called.load() && i < 100; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::this_thread::yield();
        }
        create_surface_called.store(false);
    }

    void wait_for_next_buffer()
    {
        for (int i = 0; !next_buffer_called.load() && i < 100; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::this_thread::yield();
        }
        next_buffer_called.store(false);
    }

    void wait_for_disconnect_done()
    {
        for (int i = 0; !disconnect_done_called.load() && i < 100; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::this_thread::yield();
        }
        disconnect_done_called.store(false);
    }

    void wait_for_surface_count(int count)
    {
        for (int i = 0; count != create_surface_done_count.load() && i < 10000; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::this_thread::yield();
        }
    }
    void wait_for_disconnect_count(int count)
    {
        for (int i = 0; count != disconnect_done_count.load() && i < 10000; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::this_thread::yield();
        }
    }

    void tfd_done()
    {
        tfd_done_called.store(true);
    }

    void wait_for_tfd_done()
    {
        for (int i = 0; !tfd_done_called.load() && i < 100; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        tfd_done_called.store(false);
    }

    std::atomic<bool> connect_done_called;
    std::atomic<bool> create_surface_called;
    std::atomic<bool> next_buffer_called;
    std::atomic<bool> disconnect_done_called;
    std::atomic<bool> tfd_done_called;

    std::atomic<int> connect_done_count;
    std::atomic<int> create_surface_done_count;
    std::atomic<int> disconnect_done_count;
};
}
}
#endif /* MIR_TEST_TEST_CLIENT_H_ */
