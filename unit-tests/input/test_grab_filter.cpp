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

#include "mir/frontend/application.h"
#include "mir/frontend/services/input_grab_controller.h"
#include "mir/input/event.h"
#include "mir/input/grab_filter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mfs = mir::frontend::services;
namespace mi = mir::input;

namespace
{

class MockApplication : public mf::Application
{
 public:
    MOCK_METHOD1(on_event, void(mi::Event*));
};

class MockInputGrabController : public mfs::InputGrabController
{
 public:
    MOCK_METHOD1(grab_input_for_application, void(std::weak_ptr<mf::Application>));
    MOCK_METHOD0(get_grabbing_application, std::weak_ptr<mf::Application>());
    MOCK_METHOD0(release_grab, void());
};

}

TEST(GrabFilter, grab_filter_accepts_event_stops_event_processing_if_grab_is_active)
{
    using namespace ::testing;

    std::weak_ptr<mf::Application> grabbing_application;
    MockInputGrabController grab_controller;
    ON_CALL(grab_controller, get_grabbing_application()).WillByDefault(Return(grabbing_application));
        
    MockApplication* app = new MockApplication();
    std::shared_ptr<mf::Application> app_ptr(app);
    mi::GrabFilter grab_filter(&grab_controller);

    EXPECT_CALL(grab_controller, get_grabbing_application()).Times(AtLeast(1));
    
    mi::MockInputEvent e;
    EXPECT_EQ(mi::Filter::Result::continue_processing, grab_filter.accept(&e));
    grabbing_application = app_ptr;
    ON_CALL(grab_controller, get_grabbing_application()).WillByDefault(Return(grabbing_application));
    
    EXPECT_CALL(*app, on_event(_)).Times(AtLeast(1));

    EXPECT_EQ(mi::Filter::Result::stop_processing, grab_filter.accept(&e));
}
