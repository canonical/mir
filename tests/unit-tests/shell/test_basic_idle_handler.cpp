/*
 * Copyright © 2022 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "src/server/shell/basic_idle_handler.h"
#include "mir/test/doubles/mock_idle_hub.h"
#include "mir/test/doubles/stub_input_scene.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/shell/display_configuration_controller.h"
#include "mir/test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
struct MockDisplayConfigurationController: msh::DisplayConfigurationController
{
public:
    MOCK_METHOD1(set_base_configuration, void(std::shared_ptr<mg::DisplayConfiguration> const&));
    MOCK_METHOD0(base_configuration, std::shared_ptr<mg::DisplayConfiguration>());
    MOCK_METHOD1(set_power_mode, void(MirPowerMode));
};

struct BasicIdleHandler: Test
{
    NiceMock<mtd::MockIdleHub> idle_hub;
    mtd::StubInputScene input_scene;
    mtd::StubBufferAllocator allocator;
    MockDisplayConfigurationController display;
};
}

TEST_F(BasicIdleHandler, turns_display_off_when_idle)
{
    std::weak_ptr<ms::IdleStateObserver> observer;
    EXPECT_CALL(idle_hub, register_interest(_, _))
        .Times(AnyNumber());
    EXPECT_CALL(idle_hub, register_interest(_, Eq(msh::BasicIdleHandler::display_off_timeout)))
        .WillOnce(SaveArg<0>(&observer));
    msh::BasicIdleHandler handler{
        mt::fake_shared(idle_hub),
        mt::fake_shared(input_scene),
        mt::fake_shared(allocator),
        mt::fake_shared(display)};
    auto const locked = observer.lock();
    ASSERT_THAT(locked, Ne(nullptr));
    EXPECT_CALL(display, set_power_mode(mir_power_mode_off));
    locked->idle();
}

TEST_F(BasicIdleHandler, turns_display_back_on_when_active)
{
    std::weak_ptr<ms::IdleStateObserver> observer;
    EXPECT_CALL(idle_hub, register_interest(_, _))
        .Times(AnyNumber());
    EXPECT_CALL(idle_hub, register_interest(_, Eq(msh::BasicIdleHandler::display_off_timeout)))
        .WillOnce(SaveArg<0>(&observer));
    msh::BasicIdleHandler handler{
        mt::fake_shared(idle_hub),
        mt::fake_shared(input_scene),
        mt::fake_shared(allocator),
        mt::fake_shared(display)};
    auto const locked = observer.lock();
    ASSERT_THAT(locked, Ne(nullptr));
    locked->idle();
    EXPECT_CALL(display, set_power_mode(mir_power_mode_on));
    locked->active();
}
