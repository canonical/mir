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
#include "src/server/input/android/input_dispatcher_configuration.h"

namespace mi = mir::input;
namespace mia = mi::android;
namespace mtd = mir::test::doubles;

class FakeEventHubInputDispatcherConfiguration : public mia::InputDispatcherConfiguration
{
public:
    FakeEventHubInputDispatcherConfiguration(std::shared_ptr<mi::EventFilter> const& ev_filter,
                                             std::shared_ptr<mi::InputReport> const& input_report)
        : InputDispatcherConfiguration(ev_filter, input_report)
    {
    }
    bool is_key_repeat_enabled() const override
    {
        return false;
    }
};

mtd::FakeEventHubInputConfiguration::FakeEventHubInputConfiguration(
    std::shared_ptr<mir::input::EventFilter> const& event_filter,
    std::shared_ptr<mi::InputRegion> const& input_region,
    std::shared_ptr<mi::CursorListener> const& cursor_listener,
    std::shared_ptr<mi::InputReport> const& input_report)
    : FakeEventHubInputConfiguration(
          std::make_shared<FakeEventHubInputDispatcherConfiguration>(event_filter, input_report), input_region,
          cursor_listener, input_report)
{
    event_hub = new mia::FakeEventHub();
}

mtd::FakeEventHubInputConfiguration::FakeEventHubInputConfiguration(
    std::shared_ptr<mir::input::InputDispatcherConfiguration> const& dispatcher_conf,
    std::shared_ptr<input::InputRegion> const& input_region,
    std::shared_ptr<input::CursorListener> const& cursor_listener,
    std::shared_ptr<mi::InputReport> const& input_report)
    : DefaultInputConfiguration(dispatcher_conf, input_region, cursor_listener, input_report)
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
