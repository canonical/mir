/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/server/scene/session_locker.h"
#include "mir/test/doubles/mock_frontend_surface_stack.h"
#include "mir/time/steady_clock.h"
#include "mir/glib_main_loop.h"
#include "mir/test/auto_unblock_thread.h"
#include "mir/executor.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
using namespace ::testing;

struct MockSessionLockObserver : public mf::SessionLockObserver
{
    MOCK_METHOD((void), on_lock, (), (override));
    MOCK_METHOD((void), on_unlock, (), (override));
};

struct StubScreenLockHandle : public mf::ScreenLockHandle
{
    void allow_to_be_dropped() override {}
};

class StubExecutor : public mir::Executor
{
public:
    void spawn(std::function<void()>&& work) override
    {
        work();
    }
};

class SessionLocker : public Test
{
public:
    std::shared_ptr<mtd::MockFrontendSurfaceStack> surface_stack;
    std::shared_ptr<ms::SessionLocker> locker;

    SessionLocker()
        : surface_stack{std::make_shared<mtd::MockFrontendSurfaceStack>()}
    {
        ON_CALL(*surface_stack, lock_screen())
            .WillByDefault(Return(ByMove(std::make_unique<StubScreenLockHandle>())));
    }

    void SetUp() override
    {
        locker = std::make_shared<ms::SessionLocker>(std::make_shared<StubExecutor>(), surface_stack);
    }
};

TEST_F(SessionLocker, calling_on_lock_results_in_surface_lock)
{
    EXPECT_CALL(*surface_stack, screen_is_locked()).Times(1);
    EXPECT_CALL(*surface_stack, lock_screen()).Times(1);
    locker->on_lock();
}

TEST_F(SessionLocker, observer_is_notified_on_lock)
{
    EXPECT_CALL(*surface_stack, screen_is_locked()).Times(1);
    EXPECT_CALL(*surface_stack, lock_screen()).Times(1);
    auto observer = std::make_shared<MockSessionLockObserver>();
    EXPECT_CALL(*observer, on_lock()).Times(1);
    locker->register_interest(observer);
    locker->on_lock();
}

TEST_F(SessionLocker, observer_is_notified_on_unlock)
{
    auto observer = std::make_shared<MockSessionLockObserver>();
    EXPECT_CALL(*surface_stack, lock_screen()).Times(1);
    EXPECT_CALL(*surface_stack, screen_is_locked()).Times(2);
    EXPECT_CALL(*observer, on_unlock()).Times(1);
    locker->on_lock();
    locker->register_interest(observer);
    ON_CALL(*surface_stack, screen_is_locked())
        .WillByDefault(Return(true));
    locker->on_unlock();
}
