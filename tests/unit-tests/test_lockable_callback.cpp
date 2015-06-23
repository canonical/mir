/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 *
 */

#include "mir/lockable_callback_wrapper.h"
#include "mir/basic_callback.h"

#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_lockable_callback.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;

TEST(LockableCallbackWrapper, forwards_calls_to_wrapper)
{
    using namespace ::testing;

    mtd::MockLockableCallback mock_lockable_callback;

    bool pre_hook_called{false};
    bool post_hook_called{false};

    mir::LockableCallbackWrapper wrapper{
        mt::fake_shared(mock_lockable_callback),
        [&pre_hook_called] {pre_hook_called = true;},
        [&post_hook_called] { post_hook_called = true; }};

    EXPECT_CALL(mock_lockable_callback, lock());
    wrapper.lock();

    EXPECT_CALL(mock_lockable_callback, unlock());
    wrapper.unlock();

    EXPECT_CALL(mock_lockable_callback, functor());
    wrapper();

    EXPECT_THAT(pre_hook_called, Eq(true));
    EXPECT_THAT(post_hook_called, Eq(true));
}

TEST(BasicCallback, forwards_calls_to_provided_function)
{
    using namespace ::testing;

    bool callback_invoked{false};

    mir::BasicCallback callback{[&callback_invoked] { callback_invoked = true; }};
    callback();

    EXPECT_THAT(callback_invoked, Eq(true));
}
