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

#include "mir/application.h"
#include "mir/application_manager.h"
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

class MockEventHandler : public mi::EventHandler
{
 public:

    MOCK_METHOD1(on_event, void(mi::Event*));
};

struct MockApplicationManager : public mir::ApplicationManager
{
    MOCK_METHOD0(get_grabbing_application, std::weak_ptr<mir::Application>());
};

}


TEST(GrabFilter, register_and_deregister_a_grab)
{
    mi::GrabFilter grab_filter{std::make_shared<mi::NullFilter>()};

    std::shared_ptr<mi::EventHandler> event_handler{std::make_shared<MockEventHandler>()};

    mi::GrabHandle grab_handle(grab_filter.push_grab(event_handler));

	grab_filter.release_grab(grab_handle);
}
