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

#include "mock_input_device.h"

#include "mir/time_source.h"
#include "mir/input/device.h"
#include "mir/input/dispatcher.h"
#include "mir/input/event.h"
#include "mir/input/filter.h"
#include "mir/input/logical_device.h"
#include "mir/input/position_info.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;

namespace
{

class MockFilter : public mi::Filter
{
 public:
    MockFilter()
    {
        using namespace testing;
        using ::testing::_;
        using ::testing::Return;
        using ::testing::Invoke;
        
        ON_CALL(*this, accept(_)).WillByDefault(Return(mi::Filter::Result::continue_processing));
    }
    
    MOCK_METHOD1(accept, mi::Filter::Result(mi::Event*));
};

class MockTimeSource : public mir::TimeSource
{
 public:
    MOCK_CONST_METHOD0(sample, mir::Timestamp());
};
    
}

TEST(input_dispatch, incoming_input_triggers_filter)
{
    using namespace testing;
    using ::testing::_;
    using ::testing::Return;

    mir::Timestamp ts;
    DefaultValue<mir::Timestamp>::Set(ts);
    
    MockTimeSource time_source;
    MockFilter filter;
    mi::Dispatcher dispatcher(&time_source,
                              &filter,
                              &filter,
                              &filter);
    
    mi::MockInputDevice device(&dispatcher);
    EXPECT_CALL(device, start()).Times(AtLeast(1));
    dispatcher.register_device(&device);
    
    EXPECT_CALL(filter, accept(_)).Times(AtLeast(3));

    device.TriggerEvent();

    EXPECT_CALL(device, stop()).Times(AtLeast(1));
    dispatcher.unregister_device(&device);
}

TEST(input_dispatch, incoming_input_is_timestamped)
{
    using namespace testing;

    mir::Timestamp ts;
    DefaultValue<mir::Timestamp>::Set(ts);
    
    MockTimeSource time_source;
    MockFilter filter;
    mi::Dispatcher dispatcher(&time_source,
                              &filter,
                              &filter,
                              &filter);
    mi::MockInputDevice device(&dispatcher);

    EXPECT_CALL(device, start()).Times(AtLeast(1));
    dispatcher.register_device(&device);
    
    EXPECT_CALL(time_source, sample()).Times(AtLeast(1));
    device.TriggerEvent();

    EXPECT_CALL(device, stop()).Times(AtLeast(1));
    dispatcher.unregister_device(&device);

    EXPECT_EQ(device.event.get_system_timestamp(), ts);
}
