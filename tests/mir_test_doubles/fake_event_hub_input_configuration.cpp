/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir_test/fake_event_hub_input_configuration.h"
#include "mir_test/fake_event_hub.h"

namespace mi = mir::input;
namespace mia = mi::android;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

mtd::FakeEventHubInputConfiguration::FakeEventHubInputConfiguration(
    std::initializer_list<std::shared_ptr<mir::input::EventFilter> const> const& filters,
    std::shared_ptr<mir::graphics::ViewableArea> const& view_area,
    std::shared_ptr<mir::input::CursorListener> const& cursor_listener)
      : DefaultInputConfiguration(filters, view_area, cursor_listener)
{
    event_hub = new mia::FakeEventHub();
}

mtd::FakeEventHubInputConfiguration::~FakeEventHubInputConfiguration()
{
}

droidinput::sp<droidinput::EventHubInterface> mtd::FakeEventHubInputConfiguration::the_event_hub()
{
    return event_hub;
}

mia::FakeEventHub* mtd::FakeEventHubInputConfiguration::the_fake_event_hub()
{
    return event_hub.get();
}
