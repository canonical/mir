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
#include "mock_input_event.h"

#include "mir/time_source.h"
#include "mir/input/dispatcher.h"
#include "mir/input/event.h"
#include "mir/input/filter.h"
#include "mir/input/grab_filter.h"
#include "mir/input/logical_device.h"
#include "mir/input/position_info.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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
    MOCK_CONST_METHOD0(sample, mir::Timestamp());
};

class InputDispatchFixture : public ::testing::Test
{
 public:
    InputDispatchFixture()
            : mock_shell_filter(new MockFilter<mi::Dispatcher::ShellFilter>()),
              mock_grab_filter(new MockFilter<mi::Dispatcher::GrabFilter>()),
              mock_app_filter(new MockFilter<mi::Dispatcher::ApplicationFilter>()),
              dispatcher(&time_source,
                         std::move(std::unique_ptr<mi::Dispatcher::ShellFilter>(mock_shell_filter)),
                         std::move(std::unique_ptr<mi::Dispatcher::GrabFilter>(mock_grab_filter)),
                         std::move(std::unique_ptr<mi::Dispatcher::ApplicationFilter>(mock_app_filter)))
    {
        mir::Timestamp ts;
        ::testing::DefaultValue<mir::Timestamp>::Set(ts);
    }
 protected:
    MockTimeSource time_source;
    MockFilter<mi::Dispatcher::ShellFilter>* mock_shell_filter;
    MockFilter<mi::Dispatcher::GrabFilter>* mock_grab_filter;
    MockFilter<mi::Dispatcher::ApplicationFilter>* mock_app_filter;
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

namespace
{

class MockApplication : public mir::Application
{
 public:
    MockApplication(mir::ApplicationManager* manager) : Application(manager)
    {
    }

    MOCK_METHOD1(on_event, void(mi::Event*));
};

struct MockApplicationManager : public mir::ApplicationManager
{
    MOCK_METHOD0(get_grabbing_application, std::weak_ptr<mir::Application>());
};

}

TEST(GrabFilter, grab_filter_accepts_event_stops_event_processing_if_grab_is_active)
{
    using namespace ::testing;

    MockApplicationManager appManager;
    std::weak_ptr<mir::Application> grabbing_application;
    ON_CALL(appManager, get_grabbing_application()).WillByDefault(Return(grabbing_application));
    
    MockApplication* app = new MockApplication(&appManager);
    std::shared_ptr<mir::Application> app_ptr(app);
    mi::GrabFilter grab_filter(&appManager);

    EXPECT_CALL(appManager, get_grabbing_application()).Times(AtLeast(1));
    
    mi::MockInputEvent e;
    EXPECT_EQ(mi::Filter::Result::continue_processing, grab_filter.accept(&e));
    grabbing_application = app_ptr;
    ON_CALL(appManager, get_grabbing_application()).WillByDefault(Return(grabbing_application));
    EXPECT_CALL(*app, on_event(_)).Times(AtLeast(1));

    EXPECT_EQ(mi::Filter::Result::stop_processing, grab_filter.accept(&e));
}
