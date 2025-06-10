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

#include "src/server/shell/basic_idle_handler.h"
#include "mir/executor.h"
#include "mir/test/doubles/mock_idle_hub.h"
#include "mir/test/doubles/mock_input_scene.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_session_lock.h"
#include "mir/shell/display_configuration_controller.h"
#include "mir/test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <map>
#include <boost/throw_exception.hpp>

#include "mir/graphics/renderable.h"

using namespace testing;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace std::chrono_literals;

namespace
{
struct MockDisplayConfigurationController: msh::DisplayConfigurationController
{
public:
    MOCK_METHOD1(set_base_configuration, void(std::shared_ptr<mg::DisplayConfiguration> const&));
    MOCK_METHOD0(base_configuration, std::shared_ptr<mg::DisplayConfiguration>());
    MOCK_METHOD1(set_power_mode, void(MirPowerMode));
};

class FakeSessionLock : public ms::SessionLock
{
public:
    void lock() override
    {
        auto config_observer = observer.lock();
        EXPECT_THAT(config_observer, testing::NotNull());
        config_observer->on_lock();
    }
    void unlock() override
    {
        auto config_observer = observer.lock();
        EXPECT_THAT(config_observer, testing::NotNull());
        config_observer->on_unlock();
    }
    void register_interest(std::weak_ptr<ms::SessionLockObserver> const& o) override
    {
        ASSERT_THAT(observer.lock(), testing::IsNull())
            << "FakeSessionLock does not support multiple observers";
        observer = o;
    }
    void register_interest(std::weak_ptr<ms::SessionLockObserver> const& o, mir::Executor&) override
    {
        register_interest(o);
    }
    void register_early_observer(std::weak_ptr<ms::SessionLockObserver> const& o, mir::Executor&) override
    {
        register_interest(o);
    }
    void unregister_interest(ms::SessionLockObserver const& o) override
    {
        ASSERT_THAT(observer.lock().get(), testing::Eq(&o));
        observer = std::weak_ptr<ms::SessionLockObserver>();
    }
private:
    std::weak_ptr<ms::SessionLockObserver> observer;
};

struct BasicIdleHandler: Test
{
    NiceMock<mtd::MockIdleHub> idle_hub;
    std::shared_ptr<mtd::MockInputScene> input_scene = std::make_shared<NiceMock<mtd::MockInputScene>>();
    mtd::StubBufferAllocator allocator;
    NiceMock<MockDisplayConfigurationController> display;
    FakeSessionLock session_lock;
    msh::BasicIdleHandler handler{
        mt::fake_shared(idle_hub),
        input_scene,
        mt::fake_shared(allocator),
        mt::fake_shared(display),
        mt::fake_shared(session_lock)};
    std::map<mir::time::Duration, std::weak_ptr<ms::IdleStateObserver>> observers;

    void SetUp()
    {
        EXPECT_CALL(idle_hub, register_interest(_, _))
            .WillRepeatedly(Invoke([this](auto observer, auto timeout)
                {
                    observers.insert(std::make_pair(timeout, observer));
                }));
    }

    auto observer_for(mir::time::Duration timeout) -> std::shared_ptr<ms::IdleStateObserver>
    {
        auto const iter = observers.find(timeout);
        if (iter == observers.end())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error(
                "no observer registered for timeout " + std::to_string(timeout.count())));
        }
        auto const locked = iter->second.lock();
        if (!locked)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error(
                "observer registered for timeout " + std::to_string(timeout.count()) + " is null"));
        }
        return locked;
    }
};
}

TEST_F(BasicIdleHandler, does_not_register_observers_by_default)
{
    EXPECT_CALL(idle_hub, register_interest(_, _))
        .Times(0);
    mtd::StubSessionLock session_lock;
    msh::BasicIdleHandler local_handler{
        mt::fake_shared(idle_hub),
        input_scene,
        mt::fake_shared(allocator),
        mt::fake_shared(display),
        mt::fake_shared(session_lock)};
}

TEST_F(BasicIdleHandler, off_timeout_when_locked_is_used_when_session_is_locked)
{
    handler.set_display_off_timeout(30s);
    handler.set_display_off_timeout_when_locked(10s);

    EXPECT_CALL(idle_hub, register_interest(_, Eq(30s)))
        .Times(0);
    EXPECT_CALL(idle_hub, register_interest(_, Eq(10s)))
        .Times(1);
    session_lock.lock();
}

TEST_F(BasicIdleHandler, off_timeout_when_locked_is_not_used_when_session_is_not_locked)
{
    EXPECT_CALL(idle_hub, register_interest(_, Eq(30s)))
        .Times(1);
    EXPECT_CALL(idle_hub, register_interest(_, Eq(10s)))
        .Times(0);
    handler.set_display_off_timeout(30s);
    handler.set_display_off_timeout_when_locked(10s);
}

TEST_F(BasicIdleHandler, off_timeout_is_restored_when_session_is_unlocked)
{
    handler.set_display_off_timeout(30s);
    handler.set_display_off_timeout_when_locked(10s);
    session_lock.lock();

    EXPECT_CALL(idle_hub, register_interest(_, Eq(30s)))
        .Times(1);
    session_lock.unlock();
}

TEST_F(BasicIdleHandler, off_timeout_set_on_session_lock_is_not_used_when_session_is_locked)
{
    handler.set_display_off_timeout_when_locked(10s);

    EXPECT_CALL(idle_hub, register_interest(_, Eq(10s)))
        .Times(1);
    EXPECT_CALL(idle_hub, register_interest(_, Eq(5s)))
        .Times(0);
    session_lock.lock();
    handler.set_display_off_timeout(5s);
}

TEST_F(BasicIdleHandler, off_timeout_set_on_session_lock_is_used_when_session_unlocks)
{
    handler.set_display_off_timeout_when_locked(10s);

    EXPECT_CALL(idle_hub, register_interest(_, Eq(10s)))
        .Times(1);
    EXPECT_CALL(idle_hub, register_interest(_, Eq(5s)))
        .Times(1);
    session_lock.lock();
    handler.set_display_off_timeout(5s);
    session_lock.unlock();
}

TEST_F(BasicIdleHandler, off_timeout_when_locked_set_on_session_lock_is_used_when_session_is_locked)
{
    handler.set_display_off_timeout_when_locked(10s);

    EXPECT_CALL(idle_hub, register_interest(_, Eq(10s)))
        .Times(1);
    EXPECT_CALL(idle_hub, register_interest(_, Eq(5s)))
        .Times(1);
    session_lock.lock();
    handler.set_display_off_timeout_when_locked(5s);
}

TEST_F(BasicIdleHandler, off_timeout_when_locked_is_not_used_if_equal_to_off_timeout)
{
    EXPECT_CALL(idle_hub, register_interest(_, Eq(30s)))
        .Times(1);
    handler.set_display_off_timeout(30s);
    handler.set_display_off_timeout_when_locked(30s);
    session_lock.lock();
}

TEST_F(BasicIdleHandler, off_timeout_when_locked_can_be_disabled)
{
    handler.set_display_off_timeout_when_locked(30s);
    session_lock.lock();
    auto const observer = observer_for(30s);

    EXPECT_CALL(idle_hub, register_interest(_, _))
        .Times(0);
    EXPECT_CALL(idle_hub, unregister_interest(Not(Ref(*observer))))
        .Times(1);
    EXPECT_CALL(idle_hub, unregister_interest(Ref(*observer)))
        .Times(1);
    handler.set_display_off_timeout_when_locked(std::nullopt);
    session_lock.unlock();
    session_lock.lock();
}

TEST_F(BasicIdleHandler, turns_display_off_when_idle)
{
    handler.set_display_off_timeout(30s);
    auto const observer = observer_for(30s);

    EXPECT_CALL(display, set_power_mode(mir_power_mode_off));
    observer->idle();
    Mock::VerifyAndClearExpectations(&display);
}

TEST_F(BasicIdleHandler, turns_display_back_on_when_active)
{
    handler.set_display_off_timeout(30s);
    auto const observer = observer_for(30s);
    observer->idle();

    EXPECT_CALL(display, set_power_mode(mir_power_mode_on));
    observer->active();
    Mock::VerifyAndClearExpectations(&display);
}

TEST_F(BasicIdleHandler, off_timeout_is_disabled_on_destruction)
{
    handler.set_display_off_timeout(30s);
    auto const observer = observer_for(30s);

    EXPECT_CALL(idle_hub, unregister_interest(Not(Ref(*observer))))
        .Times(AnyNumber());
    EXPECT_CALL(idle_hub, unregister_interest(Ref(*observer)))
        .Times(1);
}

TEST_F(BasicIdleHandler, off_timeout_can_be_disabled)
{
    handler.set_display_off_timeout(30s);
    auto const observer = observer_for(30s);

    EXPECT_CALL(idle_hub, unregister_interest(Not(Ref(*observer))))
        .Times(AnyNumber());
    EXPECT_CALL(idle_hub, unregister_interest(Ref(*observer)))
        .Times(1);
    handler.set_display_off_timeout(std::nullopt);
    Mock::VerifyAndClearExpectations(&display);
}

TEST_F(BasicIdleHandler, registers_new_off_timeout)
{
    handler.set_display_off_timeout(30s);
    auto const observer = observer_for(30s);

    EXPECT_CALL(idle_hub, register_interest(_, Eq(99s)))
        .Times(1);
    handler.set_display_off_timeout(99s);
}

TEST_F(BasicIdleHandler, unregisters_old_off_timeout)
{
    handler.set_display_off_timeout(30s);
    auto const observer = observer_for(30s);

    EXPECT_CALL(idle_hub, unregister_interest(Not(Ref(*observer))))
        .Times(AnyNumber());
    EXPECT_CALL(idle_hub, unregister_interest(Ref(*observer)))
        .Times(1);
    handler.set_display_off_timeout(99s);
}

TEST_F(BasicIdleHandler, display_turned_back_on_when_off_timeout_disabled)
{
    handler.set_display_off_timeout(30s);
    auto const observer = observer_for(30s);
    observer->idle();

    EXPECT_CALL(display, set_power_mode(mir_power_mode_on));
    handler.set_display_off_timeout(std::nullopt);
    Mock::VerifyAndClearExpectations(&display);
}

TEST_F(BasicIdleHandler, renderable_added_to_scene_on_idle)
{
    handler.set_display_off_timeout(30s);
    auto const observer = observer_for(20s);
    EXPECT_CALL(*input_scene, add_input_visualization(testing::_));
    observer->idle();
}

TEST_F(BasicIdleHandler, renderable_added_to_has_normal_orientation)
{
    handler.set_display_off_timeout(30s);
    auto const observer = observer_for(20s);
    std::shared_ptr<mg::Renderable> renderable;
    EXPECT_CALL(*input_scene, add_input_visualization(testing::_))
        .WillOnce(SaveArg<0>(&renderable));
    observer->idle();
    EXPECT_THAT(renderable->orientation(), Eq(mir_orientation_normal));
}

TEST_F(BasicIdleHandler, renderable_removed_from_scene_on_active)
{
    handler.set_display_off_timeout(30s);
    auto const observer = observer_for(20s);
    observer->idle();
    EXPECT_CALL(*input_scene, remove_input_visualization(testing::_));
    observer->active();
}


