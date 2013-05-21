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

#include "src/server/input/android/android_dispatcher_controller.h"

#include "mir_test_doubles/mock_input_configuration.h"
#include "mir_test_doubles/mock_input_dispatcher.h"
#include "mir_test_doubles/stub_surface_target.h"

#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <stdexcept>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{

// TODO: It would be nice if it were possible to mock the interface between
// droidinput::InputChannel and droidinput::InputDispatcher rather than use
// valid fds to allow non-throwing construction of a real input channel.
struct AndroidDispatcherControllerFdSetup : public testing::Test
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
    mtd::MockInputConfiguration config;
};

static bool
application_handle_matches_surface(droidinput::sp<droidinput::InputApplicationHandle> const& handle,
                                   std::shared_ptr<mi::SurfaceTarget> const& surface)
{
   if (handle->getName() != droidinput::String8(surface->name().c_str()))
        return false;
   return true;
}

static bool
window_handle_matches_surface(droidinput::sp<droidinput::InputWindowHandle> const& handle,
                              std::shared_ptr<mi::SurfaceTarget> const& surface)
{
    if (handle->getInputChannel()->getFd() != surface->server_input_fd())
        return false;
    return true;
}

MATCHER_P(WindowHandleFor, surface, "")
{
    return window_handle_matches_surface(arg, surface);
}

MATCHER_P(ApplicationHandleFor, surface, "")
{
    return application_handle_matches_surface(arg, surface);
}

MATCHER_P(VectorContainingWindowHandleFor, surface, "")
{
    auto i = arg.size();
    for (i = 0; i < arg.size(); i++)
    {
        if (window_handle_matches_surface(arg[i], surface))
            return true;
    }
    return false;
}

MATCHER(EmptyVector, "")
{
    return arg.size() == 0;
}

}

TEST_F(AndroidDispatcherControllerFdSetup, input_surface_opened_behavior)
{    
    using namespace ::testing;

    auto surface = std::make_shared<mtd::StubSurfaceTarget>(test_input_fd);

    EXPECT_CALL(config, the_dispatcher()).Times(1)
        .WillOnce(Return(dispatcher));
    EXPECT_CALL(*dispatcher, registerInputChannel(_, WindowHandleFor(surface), false)).Times(1)
        .WillOnce(Return(droidinput::OK));

    mia::DispatcherController controller(mt::fake_shared(config));
    
     controller.input_surface_opened(surface);
     EXPECT_THROW({
             // We can't open a surface twice
             controller.input_surface_opened(surface);
     }, std::logic_error);
}

TEST_F(AndroidDispatcherControllerFdSetup, input_surface_closed_behavior)
{
    using namespace ::testing;

    auto surface = std::make_shared<mtd::StubSurfaceTarget>(test_input_fd);

    EXPECT_CALL(config, the_dispatcher()).Times(1)
        .WillOnce(Return(dispatcher));
    EXPECT_CALL(*dispatcher, registerInputChannel(_, WindowHandleFor(surface), false)).Times(1)
        .WillOnce(Return(droidinput::OK));
    EXPECT_CALL(*dispatcher, unregisterInputChannel(_)).Times(1);
    mia::DispatcherController controller(mt::fake_shared(config));
    
    EXPECT_THROW({
            // We can't close a surface which hasn't been opened
            controller.input_surface_closed(surface);
    }, std::logic_error);
    controller.input_surface_opened(surface);
    controller.input_surface_closed(surface);
    EXPECT_THROW({
            // Nor can we close a surface twice
            controller.input_surface_closed(surface);
    }, std::logic_error);
}

TEST_F(AndroidDispatcherControllerFdSetup, on_focus_cleared)
{
    using namespace ::testing;

    EXPECT_CALL(config, the_dispatcher()).Times(1)
        .WillOnce(Return(dispatcher));

    EXPECT_CALL(*dispatcher, setFocusedApplication(droidinput::sp<droidinput::InputApplicationHandle>(0))).Times(1);
    EXPECT_CALL(*dispatcher, setInputWindows(EmptyVector())).Times(1);

    mia::DispatcherController controller(mt::fake_shared(config));
    
    controller.focus_cleared();
}

TEST_F(AndroidDispatcherControllerFdSetup, on_focus_changed)
{
    using namespace ::testing;

    auto surface = std::make_shared<mtd::StubSurfaceTarget>(test_input_fd);

    EXPECT_CALL(config, the_dispatcher()).Times(1)
        .WillOnce(Return(dispatcher));
    EXPECT_CALL(*dispatcher, registerInputChannel(_, WindowHandleFor(surface), false)).Times(1)
        .WillOnce(Return(droidinput::OK));

    {
        InSequence seq;
        EXPECT_CALL(*dispatcher, setFocusedApplication(ApplicationHandleFor(surface))).Times(1);
        EXPECT_CALL(*dispatcher, setInputWindows(VectorContainingWindowHandleFor(surface))).Times(1);
    }

    mia::DispatcherController controller(mt::fake_shared(config));
    
    controller.input_surface_opened(surface);

    controller.focus_changed(surface);
}

TEST_F(AndroidDispatcherControllerFdSetup, on_focus_changed_throw_behavior)
{
    using namespace ::testing;

    EXPECT_CALL(config, the_dispatcher()).Times(1)
        .WillOnce(Return(dispatcher));

    auto surface = std::make_shared<mtd::StubSurfaceTarget>(test_input_fd);

    mia::DispatcherController controller(mt::fake_shared(config));

    EXPECT_THROW({
            // We can't focus surfaces which never opened
            controller.focus_changed(surface);
    }, std::logic_error);
}
