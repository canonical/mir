/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/input/surface.h"

#include "src/server/input/android/android_input_registrar.h"

#include "mir_test_doubles/mock_input_dispatcher.h"
#include "mir_test_doubles/stub_input_channel.h"

#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <stdexcept>

namespace mi = mir::input;
namespace mia = mi::android;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace geom = mir::geometry;

namespace
{

// TODO: It would be nice if it were possible to mock the interface between
// droidinput::InputChannel and droidinput::InputDispatcher rather than use
// valid fds to allow non-throwing construction of a real input channel.
struct AndroidInputRegistrarFdSetup : public testing::Test
{
    void SetUp() override
    {
        test_input_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);

        dispatcher = new mtd::MockInputDispatcher();
    }
    void TearDown() override
    {
        close(test_input_fd);
    }
    int test_input_fd;
    droidinput::sp<mtd::MockInputDispatcher> dispatcher;
};

struct StubInputSurface : public mi::Surface
{
    geom::Point top_left() const
    {
        return geom::Point();
    }
    geom::Size size() const
    {
        return geom::Size();
    }
    std::string const& name() const
    {
        static std::string const name;
        return name;
    }
    bool contains(geom::Point const&) const
    {
        return true;
    }
};

MATCHER_P(WindowHandleFor, channel, "")
{
    if (arg->getInputChannel()->getFd() != channel->server_fd())
        return false;
    return true;
}

}

TEST_F(AndroidInputRegistrarFdSetup, input_channel_opened_behavior)
{
    using namespace ::testing;

    auto channel = std::make_shared<mtd::StubInputChannel>(test_input_fd);
    auto surface = std::make_shared<StubInputSurface>();

    EXPECT_CALL(*dispatcher, registerInputChannel(_, WindowHandleFor(channel), _)).Times(1)
        .WillOnce(Return(droidinput::OK));

    mia::InputRegistrar registrar(dispatcher);

    registrar.input_channel_opened(channel, surface, mi::InputReceptionMode::normal);
    EXPECT_THROW({
            // We can't open a surface twice
            registrar.input_channel_opened(channel, surface, mi::InputReceptionMode::normal);
    }, std::logic_error);
}

TEST_F(AndroidInputRegistrarFdSetup, input_channel_closed_behavior)
{
    using namespace ::testing;

    auto channel = std::make_shared<mtd::StubInputChannel>(test_input_fd);
    auto surface = std::make_shared<StubInputSurface>();

    EXPECT_CALL(*dispatcher, registerInputChannel(_, WindowHandleFor(channel), _)).Times(1)
        .WillOnce(Return(droidinput::OK));
    EXPECT_CALL(*dispatcher, unregisterInputChannel(_)).Times(1);
    mia::InputRegistrar registrar(dispatcher);

    EXPECT_THROW({
            // We can't close a surface which hasn't been opened
            registrar.input_channel_closed(channel);
    }, std::logic_error);
    registrar.input_channel_opened(channel, surface, mi::InputReceptionMode::normal);
    registrar.input_channel_closed(channel);
    EXPECT_THROW({
            // Nor can we close a surface twice
            registrar.input_channel_closed(channel);
    }, std::logic_error);
}

TEST_F(AndroidInputRegistrarFdSetup, monitor_flag_is_passed_to_dispatcher)
{
    using namespace ::testing;

    auto channel = std::make_shared<mtd::StubInputChannel>(test_input_fd);
    auto surface = std::make_shared<StubInputSurface>();

    EXPECT_CALL(*dispatcher, registerInputChannel(_, _, true)).Times(1)
        .WillOnce(Return(droidinput::OK));

    mia::InputRegistrar registrar(dispatcher);

    registrar.input_channel_opened(channel, surface, mi::InputReceptionMode::receives_all_input);
}
