/*
 * Copyright © 2016-2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include <miral/test_server.h>
#include <miral/x11_support.h>

#include <mir/test/signal.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

namespace
{
auto const a_long_time = std::chrono::seconds(1);

struct Runner : miral::TestServer
{
    void SetUp() override
    {
    }

    MOCK_METHOD0(callback, void());
};
}

TEST_F(Runner, stop_callback_is_called)
{
    add_stop_callback([this] { callback(); });
    miral::TestServer::SetUp();
    EXPECT_CALL(*this, callback());
}

TEST_F(Runner, start_callback_is_called)
{
    mir::test::Signal signal;

    add_start_callback([this] { callback(); });
    EXPECT_CALL(*this, callback())
        .WillOnce(InvokeWithoutArgs([&] { signal.raise(); }));

    miral::TestServer::SetUp();

    signal.wait_for(a_long_time);
    testing::Mock::VerifyAndClearExpectations(this);
}

TEST_F(Runner, wayland_socket_is_returned_by_default)
{
    miral::TestServer::SetUp();

    miral::TestServer::invoke_runner([](auto& runner){ EXPECT_TRUE(runner.wayland_display().is_set()); });
}

TEST_F(Runner, x11_socket_is_not_returned_by_default)
{
    miral::TestServer::SetUp();

    miral::TestServer::invoke_runner([](auto& runner){ EXPECT_FALSE(runner.x11_display().is_set()); });
}

TEST_F(Runner, x11_socket_is_returned_if_configured)
{
    miral::X11Support x11support;
    add_server_init(x11support);
    add_to_environment("MIR_SERVER_x11_DISPLAY_EXPERIMENTAL", "666");

    miral::TestServer::SetUp();

    miral::TestServer::invoke_runner([](auto& runner)
    {
        EXPECT_TRUE(runner.x11_display().is_set());
        EXPECT_THAT(runner.x11_display().value(), Eq(":666"));
    });
}
