/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/in_process_server.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <algorithm>
#include <condition_variable>
#include <mutex>

namespace
{
using DemoServerConfiguration = mir_test_framework::StubbedServerConfiguration;

struct TrustSessionHelper : mir_test_framework::InProcessServer
{
    DemoServerConfiguration my_server_config;

    mir::DefaultServerConfiguration& server_config() override { return my_server_config; }

    void SetUp()
    {
        mir_test_framework::InProcessServer::SetUp();
        connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    }

    void TearDown()
    {
        mir_connection_release(connection);
        mir_test_framework::InProcessServer::TearDown();
    }

    MirConnection* connection = nullptr;

    static std::size_t const arbritary_fd_request_count = 3;

    std::mutex mutex;
    std::size_t actual_fd_count = 0;
    int actual_fds[arbritary_fd_request_count] = {};
    std::condition_variable cv;
    bool called_back = false;

    bool wait_for_callback(std::chrono::milliseconds timeout)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        return cv.wait_for(lock, timeout, [this]{ return called_back; });
    }
};

void client_fd_callback(MirConnection*, size_t count, int const* fds, void* context)
{
    auto const self = static_cast<TrustSessionHelper*>(context);

    std::unique_lock<decltype(self->mutex)> lock(self->mutex);
    self->actual_fd_count = count;

    std::copy(fds, fds+count, self->actual_fds);
    self->called_back = true;
    self->cv.notify_one();
}
}

using namespace testing;

TEST_F(TrustSessionHelper, gets_fds_for_trusted_clients)
{
    mir_connection_new_fds_for_trusted_clients(connection, arbritary_fd_request_count, &client_fd_callback, this);
    EXPECT_TRUE(wait_for_callback(std::chrono::milliseconds(500)));

    EXPECT_THAT(actual_fd_count, Eq(arbritary_fd_request_count));
}
