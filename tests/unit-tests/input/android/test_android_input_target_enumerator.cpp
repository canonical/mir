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

#include "src/server/input/android/android_input_target_enumerator.h"

#include "mir/input/input_channel.h"
#include "mir/input/input_targets.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/stub_input_channel.h"
#include "mir_test_doubles/stub_input_handles.h"
#include "mir_test_doubles/mock_window_handle_repository.h"

#include <InputDispatcher.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <initializer_list>
#include <map>

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

struct StubInputTargets : public mi::InputTargets
{
    StubInputTargets(std::initializer_list<std::shared_ptr<mi::InputChannel>> const& target_list)
        : targets(target_list.begin(), target_list.end())
    {
    }

    void for_each(std::function<void(std::shared_ptr<mi::InputChannel> const&)> const& callback) override
    {
        for (auto target : targets)
            callback(target);
    }

    std::vector<std::shared_ptr<mi::InputChannel>> targets;
};

}

TEST(AndroidInputTargetEnumerator, enumerates_registered_handles_for_surfaces)
{
    using namespace ::testing;
    std::shared_ptr<mi::InputChannel> t1, t2;
    t1 = std::make_shared<mtd::StubInputChannel>();
    t2 = std::make_shared<mtd::StubInputChannel>();
    mtd::MockWindowHandleRepository repository;
    droidinput::sp<droidinput::InputWindowHandle> stub_window_handle1 = new mtd::StubWindowHandle;
    droidinput::sp<droidinput::InputWindowHandle> stub_window_handle2 = new mtd::StubWindowHandle;
    StubInputTargets targets({t1, t2});

    Sequence seq2;
    EXPECT_CALL(repository, handle_for_channel(
        std::const_pointer_cast<mi::InputChannel const>(t1)))
        .InSequence(seq2)
        .WillOnce(Return(stub_window_handle1));
    EXPECT_CALL(repository, handle_for_channel(
        std::const_pointer_cast<mi::InputChannel const>(t2)))
        .InSequence(seq2)
        .WillOnce(Return(stub_window_handle2));

    struct MockTargetObserver
    {
        MOCK_METHOD1(see, void(droidinput::sp<droidinput::InputWindowHandle> const&));
    } observer;
    Sequence seq;
    EXPECT_CALL(observer, see(stub_window_handle1))
        .InSequence(seq);
    EXPECT_CALL(observer, see(stub_window_handle2))
        .InSequence(seq);

    // The InputTargetEnumerator only holds a weak reference to the targets so we need to hold a shared pointer.
    auto shared_targets = mt::fake_shared(targets);
    auto shared_handles = mt::fake_shared(repository);
    mia::InputTargetEnumerator enumerator(shared_targets, shared_handles);
    enumerator.for_each([&](droidinput::sp<droidinput::InputWindowHandle> const& handle)
        {
            observer.see(handle);
        });
}
