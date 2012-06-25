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

#include "mir/time_source.h"
#include "mir/input/device.h"
#include "mir/input/dispatcher.h"
#include "mir/input/event.h"
#include "mir/input/filter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;

namespace
{

class MockEvent : public mi::Event {
};

class MockFilter : public mi::Filter
{
 public:
    MOCK_METHOD1(Accept, bool(mi::Event*));
};

class MockInputDevice : public mi::Device
{
 public:
    MockInputDevice(mi::EventHandler* h) : mi::Device(h)
    {
    }

    void TriggerEvent()
    {
        handler->OnEvent(&event);
    }

    MockEvent event;
};

class MockTimeSource : public mir::TimeSource
{
 public:
    MOCK_CONST_METHOD0(Sample, mir::Timestamp());
};
    
}

TEST(input_dispatch, incoming_input_triggers_filter)
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
    
    MockInputDevice device(&dispatcher);

    EXPECT_CALL(filter, Accept(_)).Times(AtLeast(3));

    device.TriggerEvent();
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
    MockInputDevice device(&dispatcher);

    EXPECT_CALL(time_source, Sample()).Times(AtLeast(1));
    device.TriggerEvent();

    EXPECT_EQ(device.event.SystemTimestamp(), ts);
}
