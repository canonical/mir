/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "../../end-to-end-tests/input_dispatch_fixture.h"
#include "../../end-to-end-tests/mock_input_device.h"

#include "mir/time_source.h"
#include "mir/input/dispatcher.h"
#include "mir/input/event.h"
#include "mir/input/filter.h"
#include "mir/input/logical_device.h"
#include "mir/input/position_info.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <thread>

namespace mi = mir::input;

using mir::input::InputDispatchFixture;

TEST_F(InputDispatchFixture, filters_are_always_invoked_in_order_and_events_are_weakly_ordered_by_their_timestamp)
{
    using namespace::testing;

    EXPECT_CALL(time_source, sample()).Times(AnyNumber());
    
    mir::Timestamp last_timestamp = time_source.sample();  
    auto is_weakly_ordered = [&](mi::Event* e) -> bool
    {
        bool result = e->get_system_timestamp() >= last_timestamp;
        last_timestamp = e->get_system_timestamp();
        return result;
    };
    
    EXPECT_CALL(*mock_null_filter, accept(Truly(is_weakly_ordered)))
            .Times(AnyNumber())
            .RetiresOnSaturation();
    EXPECT_CALL(*mock_shell_filter, accept(Truly(is_weakly_ordered)))
            .Times(AnyNumber())
            .RetiresOnSaturation();
    EXPECT_CALL(*mock_grab_filter, accept(Truly(is_weakly_ordered)))
            .Times(AnyNumber())
            .RetiresOnSaturation();
    EXPECT_CALL(*mock_app_filter, accept(Truly(is_weakly_ordered)))
            .Times(AnyNumber())
            .RetiresOnSaturation();

    mi::MockInputDevice* mock_device1 = new mi::MockInputDevice(&dispatcher);
    mi::MockInputDevice* mock_device2 = new mi::MockInputDevice(&dispatcher);
    EXPECT_CALL(*mock_device1, start()).Times(1);
    EXPECT_CALL(*mock_device1, stop()).Times(1);
    EXPECT_CALL(*mock_device2, start()).Times(1);
    EXPECT_CALL(*mock_device2, stop()).Times(1);
    std::unique_ptr<mi::LogicalDevice> device1(mock_device1);
    std::unique_ptr<mi::LogicalDevice> device2(mock_device2);

    auto token1 = dispatcher.register_device(std::move(device1));
    auto token2 = dispatcher.register_device(std::move(device2));

    auto worker = [&](mi::MockInputDevice* dev) -> void
    {
        for(int i = 0; i < 1000; i++)
            dev->trigger_event();
    };
    
    std::thread t1(worker, mock_device1);
    std::thread t2(worker, mock_device2);

    t1.join();
    t2.join();
    
    dispatcher.unregister_device(token1);
    dispatcher.unregister_device(token2);
}
