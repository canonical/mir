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

#include "src/server/input/android/android_input_targeter.h"
#include "src/server/input/android/android_input_registrar.h"
#include "src/server/input/android/android_window_handle_repository.h"

#include "mir/input/input_channel.h"

#include "mir_test_doubles/mock_input_dispatcher.h"
#include "mir_test_doubles/stub_input_channel.h"

#include "mir_test/fake_shared.h"

#include <InputWindow.h>
#include <InputApplication.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <stdexcept>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace geom = mir::geometry;

namespace
{

struct StubInputApplicationHandle : public droidinput::InputApplicationHandle
{
    StubInputApplicationHandle()
    {
        if (mInfo == NULL)
            mInfo = new droidinput::InputApplicationInfo;
    }
    bool updateInfo()
    {
        return true;
    }
};

struct StubWindowHandle : public droidinput::InputWindowHandle
{
    StubWindowHandle()
      : droidinput::InputWindowHandle(make_app_handle())
    {
        if (!mInfo)
        {
            mInfo = new droidinput::InputWindowInfo();
        }
    }

    droidinput::sp<droidinput::InputApplicationHandle> make_app_handle()
    {
        return new StubInputApplicationHandle;
    }

    bool updateInfo()
    {
        return true;
    }
};

struct MockWindowHandleRepository : public mia::WindowHandleRepository
{
    ~MockWindowHandleRepository() noexcept(true) {};
    MOCK_METHOD1(handle_for_surface, droidinput::sp<droidinput::InputWindowHandle>(std::shared_ptr<mi::InputChannel const> const& surface));
};

struct StubInputChannel : public mi::InputChannel
{
    StubInputChannel()
    {
    }

    int client_fd() const override
    {
        return 0;
    }
    int server_fd() const override
    {
        return 0;
    }

};

MATCHER_P(WindowHandleFor, surface, "")
{
    if (arg->channel != surface)
        return false;
    return true;
}

//struct StubInputApplicationHandle : public droidinput::InputApplicationHandle
//{
//};
}

#if 0
TEST(AndroidInputTargeterSetup, on_focus_cleared)
{
    using namespace ::testing;

    droidinput::sp<mtd::MockInputDispatcher> dispatcher = new mtd::MockInputDispatcher;

    EXPECT_CALL(*dispatcher, setKeyboardFocus(droidinput::sp<droidinput::InputWindowHandle>(0))).Times(1);

    StubWindowHandleRepository repository;
    mia::InputTargeter targeter(dispatcher, mt::fake_shared(repository));
    
    targeter.focus_cleared();
}
#endif

TEST(AndroidInputTargeterSetup, on_focus_changed)
{
    using namespace ::testing;

    std::shared_ptr<mi::InputChannel const> stub_surface = std::make_shared<StubInputChannel>();
    MockWindowHandleRepository repository;

    droidinput::sp<mtd::MockInputDispatcher> dispatcher = new mtd::MockInputDispatcher;
    droidinput::sp<droidinput::InputWindowHandle> stub_window_handle = new StubWindowHandle;

    EXPECT_CALL(*dispatcher, setKeyboardFocus(stub_window_handle))
        .Times(1);
    EXPECT_CALL(repository, handle_for_surface(stub_surface))
        .Times(1)
        .WillOnce(Return(stub_window_handle));
    mia::InputTargeter targeter(dispatcher, mt::fake_shared(repository));
    
    targeter.focus_changed(stub_surface);
}

#if 0

TEST(AndroidInputTargeterSetup, on_focus_changed_throw_behavior)
{
    using namespace ::testing;

    droidinput::sp<mtd::MockInputDispatcher> dispatcher = new mtd::MockInputDispatcher;
    MockWindowHandleRepository repository;
    mia::InputTargeter targeter(dispatcher, mt::fake_shared(repository));

    auto surface = std::make_shared<StubInputChannel>("test_focus_changed_throw");

    EXPECT_CALL(repository, handle_for_surface(_)).Times(1)
        .WillOnce(Return(droidinput::sp<droidinput::InputWindowHandle>()));
    EXPECT_THROW({
            // We can't focus surfaces which never opened
            targeter.focus_changed(surface);
    }, std::logic_error);
}
#endif
