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

#include "../../end-to-end-tests/mock_input_event.h"

#include "mir/input/event.h"
#include "mir/input/grab_filter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;

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
