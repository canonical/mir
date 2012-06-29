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
#include "mir/high_resolution_time_source.h"
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

namespace
{

template<typename T>
class MockFilter : public T
{
 public:
    MockFilter()
    {
        using namespace testing;
        using ::testing::_;
        using ::testing::Return;
        
        ON_CALL(*this, accept(_)).WillByDefault(Return(mi::Filter::Result::continue_processing));
    }
    
    MOCK_METHOD1(accept, mi::Filter::Result(mi::Event*));
};

class MockTimeSource : public mir::TimeSource
{
 public:
    MockTimeSource()
    {
        using namespace ::testing;

        ON_CALL(*this, sample()).WillByDefault(Invoke(this, &MockTimeSource::sample_hrc));
    }
    MOCK_CONST_METHOD0(sample, mir::Timestamp());

    mir::Timestamp sample_hrc() const
    {
        return boost::chrono::high_resolution_clock::now();
    }
};

class InputDispatchFixture : public ::testing::Test
{
 public:
    InputDispatchFixture()
            : mock_shell_filter(new MockFilter<mi::ShellFilter>()),
              mock_grab_filter(new MockFilter<mi::GrabFilter>()),
              mock_app_filter(new MockFilter<mi::ApplicationFilter>()),
              dispatcher(&time_source,
                         std::move(std::unique_ptr<mi::ShellFilter>(mock_shell_filter)),
                         std::move(std::unique_ptr<mi::GrabFilter>(mock_grab_filter)),
                         std::move(std::unique_ptr<mi::ApplicationFilter>(mock_app_filter)))
    {
        mir::Timestamp ts;
        ::testing::DefaultValue<mir::Timestamp>::Set(ts);
    }
 protected:
    MockTimeSource time_source;
    MockFilter<mi::ShellFilter>* mock_shell_filter;
    MockFilter<mi::GrabFilter>* mock_grab_filter;
    MockFilter<mi::ApplicationFilter>* mock_app_filter;
    mi::Dispatcher dispatcher;
};

}

TEST_F(InputDispatchFixture, incoming_input_triggers_filter)
{
    using namespace testing;
    using ::testing::_;
    using ::testing::Return;
    
    mi::MockInputDevice* mock_device = new mi::MockInputDevice(&dispatcher);
    std::unique_ptr<mi::LogicalDevice> device(mock_device);

    EXPECT_CALL(*mock_device, start()).Times(AtLeast(1));
    mi::Dispatcher::DeviceToken token = dispatcher.register_device(std::move(device));
    
    EXPECT_CALL(*mock_shell_filter, accept(_)).Times(AtLeast(1));
    EXPECT_CALL(*mock_grab_filter, accept(_)).Times(AtLeast(1));
    EXPECT_CALL(*mock_app_filter, accept(_)).Times(AtLeast(1));

    mock_device->trigger_event();

    EXPECT_CALL(*mock_device, stop()).Times(AtLeast(1));
    dispatcher.unregister_device(token);
}

TEST_F(InputDispatchFixture, incoming_input_is_timestamped)
{
    using namespace testing;

    mir::Timestamp ts;
    DefaultValue<mir::Timestamp>::Set(ts);
    
    mi::MockInputDevice* mock_device = new mi::MockInputDevice(&dispatcher);
    std::unique_ptr<mi::LogicalDevice> device(mock_device);

    EXPECT_CALL(*mock_device, start()).Times(AtLeast(1));
    mi::Dispatcher::DeviceToken token = dispatcher.register_device(std::move(device));
    
    EXPECT_CALL(time_source, sample()).Times(AtLeast(1));
    mock_device->trigger_event();

    EXPECT_EQ(mock_device->event.get_system_timestamp(), ts);

    EXPECT_CALL(*mock_device, stop()).Times(AtLeast(1));
    dispatcher.unregister_device(token);
}

TEST_F(InputDispatchFixture, device_registration_starts_and_stops_event_producer)
{
    using namespace ::testing;
    
    mi::MockInputDevice* mock_device = new mi::MockInputDevice(&dispatcher);
    std::unique_ptr<mi::LogicalDevice> device(mock_device);

    EXPECT_CALL(*mock_device, start()).Times(AtLeast(1));
    mi::Dispatcher::DeviceToken token = dispatcher.register_device(std::move(device));

    EXPECT_CALL(*mock_device, stop()).Times(AtLeast(1));
    dispatcher.unregister_device(token);
}

TEST_F(InputDispatchFixture, filters_are_always_invoked_in_order_and_events_are_weakly_ordered_by_their_timestamp)
{
    using namespace::testing;

    EXPECT_CALL(time_source, sample()).Times(AnyNumber());
    
    //mir::HighResolutionTimeSource time_source;    
    mir::Timestamp last_timestamp = time_source.sample();  
    auto is_weakly_ordered = [&](mi::Event* e) -> bool
    {
        bool result = e->get_system_timestamp() >= last_timestamp;
        last_timestamp = e->get_system_timestamp();
        return result;
    };
    
    EXPECT_CALL(*mock_shell_filter, accept(Truly(is_weakly_ordered)))
            .Times(AnyNumber())
            .RetiresOnSaturation();
    EXPECT_CALL(*mock_grab_filter, accept(Truly(is_weakly_ordered)))
            .Times(AnyNumber())
            .RetiresOnSaturation();
    EXPECT_CALL(*mock_app_filter, accept(Truly(is_weakly_ordered)))
            .Times(AnyNumber())
            .RetiresOnSaturation();

    mi::MockInputDevice* mock_device = new mi::MockInputDevice(&dispatcher);
    EXPECT_CALL(*mock_device, start()).Times(1);
    EXPECT_CALL(*mock_device, stop()).Times(1);
    std::unique_ptr<mi::LogicalDevice> device(mock_device);

    auto token = dispatcher.register_device(std::move(device));

    auto worker = [&]() -> void
    {
        for(int i = 0; i < 1000; i++)
            mock_device->trigger_event();
    };
    
    std::thread t1(worker);
    std::thread t2(worker);

    t1.join();
    t2.join();
    
    dispatcher.unregister_device(token);
}
