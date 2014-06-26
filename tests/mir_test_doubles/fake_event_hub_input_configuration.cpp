/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Robert Carr <robert.carr@canonical.com>
 *         Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir_test/fake_event_hub_input_configuration.h"
#include "mir_test/fake_event_hub.h"

namespace mi = mir::input;
namespace mia = mi::android;
namespace mtd = mir::test::doubles;

mtd::FakeEventHubInputConfiguration::FakeEventHubInputConfiguration(
    std::shared_ptr<mir::input::InputDispatcher> const& dispatcher,
    std::shared_ptr<input::InputRegion> const& input_region,
    std::shared_ptr<input::CursorListener> const& cursor_listener,
    std::shared_ptr<input::TouchVisualizer> const& touch_visualizer,
    std::shared_ptr<mi::InputReport> const& input_report) :
    DefaultInputConfiguration(dispatcher, input_region, cursor_listener, touch_visualizer, input_report),
    event_hub(std::make_shared<mia::FakeEventHub>())
{
}

mtd::FakeEventHubInputConfiguration::~FakeEventHubInputConfiguration()
{
}

std::shared_ptr<droidinput::EventHubInterface> mtd::FakeEventHubInputConfiguration::the_event_hub()
{
    return event_hub;
}

mia::FakeEventHub* mtd::FakeEventHubInputConfiguration::the_fake_event_hub()
{
    return event_hub.get();
}
