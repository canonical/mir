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
#include "mir_test_doubles/stub_input_channel.h"
#include "mir_test_doubles/stub_input_handles.h"
#include "mir_test_doubles/mock_window_handle_repository.h"

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

TEST(AndroidInputTargeterSetup, on_focus_cleared)
{
    using namespace ::testing;

    droidinput::sp<mtd::MockInputDispatcher> dispatcher = new mtd::MockInputDispatcher;

    EXPECT_CALL(*dispatcher, setKeyboardFocus(droidinput::sp<droidinput::InputWindowHandle>(0)))
        .Times(1);

    mtd::MockWindowHandleRepository repository;
    mia::InputTargeter targeter(dispatcher, mt::fake_shared(repository));

    targeter.focus_cleared();
}

TEST(AndroidInputTargeterSetup, on_focus_changed)
{
    using namespace ::testing;

    std::shared_ptr<mi::InputChannel const> stub_channel = std::make_shared<mtd::StubInputChannel>();
    mtd::MockWindowHandleRepository repository;

    droidinput::sp<mtd::MockInputDispatcher> dispatcher = new mtd::MockInputDispatcher;
    droidinput::sp<droidinput::InputWindowHandle> stub_window_handle = new mtd::StubWindowHandle;

    EXPECT_CALL(*dispatcher, setKeyboardFocus(stub_window_handle))
        .Times(1);
    EXPECT_CALL(repository, handle_for_channel(stub_channel))
        .Times(1)
        .WillOnce(Return(stub_window_handle));
    mia::InputTargeter targeter(dispatcher, mt::fake_shared(repository));

    targeter.focus_changed(stub_channel);
}

TEST(AndroidInputTargeterSetup, on_focus_changed_throw_behavior)
{
    using namespace ::testing;

    droidinput::sp<mtd::MockInputDispatcher> dispatcher = new mtd::MockInputDispatcher;
    mtd::MockWindowHandleRepository repository;
    mia::InputTargeter targeter(dispatcher, mt::fake_shared(repository));

    std::shared_ptr<mi::InputChannel const> stub_channel = std::make_shared<mtd::StubInputChannel>();

    EXPECT_CALL(repository, handle_for_channel(stub_channel))
        .Times(1)
        .WillOnce(Return(droidinput::sp<droidinput::InputWindowHandle>()));

    EXPECT_THROW({
            // We can't focus channels which never opened
            targeter.focus_changed(stub_channel);
    }, std::logic_error);
}
