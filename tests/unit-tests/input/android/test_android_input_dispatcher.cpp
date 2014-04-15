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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/server/input/android/android_input_dispatcher.h"
#include "src/server/input/android/android_input_thread.h"
#include "src/server/input/android/android_input_constants.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_android_input_dispatcher.h"

#include <utils/StrongPointer.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <initializer_list>

namespace droidinput = android;

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mt = mir::test;
namespace mtd = mt::doubles;

// Mock objects
namespace
{

struct MockInputThread : public mia::InputThread
{
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(request_stop, void());
    MOCK_METHOD0(join, void());
};

}

// Test fixture
namespace
{

struct AndroidInputDispatcherTest : public testing::Test
{
    AndroidInputDispatcherTest()
    {
        dispatcher = new mtd::MockAndroidInputDispatcher();
        dispatcher_thread = std::make_shared<MockInputThread>();
    }

    droidinput::sp<mtd::MockAndroidInputDispatcher> dispatcher;
    std::shared_ptr<mi::InputDispatcher> input_dispatcher;
    std::shared_ptr<MockInputThread> dispatcher_thread;
};

}

TEST_F(AndroidInputDispatcherTest, start_starts_dispathcer_and_thread)
{
    using namespace ::testing;

    InSequence seq;

    EXPECT_CALL(*dispatcher, setInputDispatchMode(mia::DispatchEnabled, mia::DispatchUnfrozen))
        .Times(1);
    EXPECT_CALL(*dispatcher, setInputFilterEnabled(true))
        .Times(1);

    EXPECT_CALL(*dispatcher_thread, start())
        .Times(1);

    mia::AndroidInputDispatcher input_dispatcher(dispatcher, dispatcher_thread);

    input_dispatcher.start();
}

TEST_F(AndroidInputDispatcherTest, stop_stops_dispatcher_and_thread)
{
    using namespace ::testing;

    InSequence seq;

    EXPECT_CALL(*dispatcher_thread, request_stop());
    EXPECT_CALL(*dispatcher, setInputDispatchMode(mia::DispatchDisabled, mia::DispatchFrozen)).Times(1);
    EXPECT_CALL(*dispatcher_thread, join());

    mia::AndroidInputDispatcher input_dispatcher(dispatcher, dispatcher_thread);

    input_dispatcher.stop();
}
