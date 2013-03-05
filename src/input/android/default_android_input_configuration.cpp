/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "default_android_input_configuration.h"
#include "event_filter_dispatcher_policy.h"
#include "../event_filter_chain.h"

#include <EventHub.h>
#include <InputDispatcher.h>

namespace droidinput = android;

namespace mi = mir::input;
namespace mia = mi::android;

mia::DefaultInputConfiguration::DefaultInputConfiguration(std::initializer_list<std::shared_ptr<mi::EventFilter> const> const& filters)
  : filter_chain(std::make_shared<mi::EventFilterChain>(filters))
{
}

mia::DefaultInputConfiguration::~DefaultInputConfiguration()
{
}

droidinput::sp<droidinput::EventHubInterface> mia::DefaultInputConfiguration::the_event_hub()
{
    return event_hub(
        [this]()
        {
            return new droidinput::EventHub();
        });
}

droidinput::sp<droidinput::InputDispatcherPolicyInterface> mia::DefaultInputConfiguration::the_dispatcher_policy()
{
    return dispatcher_policy(
        [this]()
        {
            return new mia::EventFilterDispatcherPolicy(filter_chain);
        });
}

droidinput::sp<droidinput::InputDispatcherInterface> mia::DefaultInputConfiguration::the_dispatcher()
{
    return dispatcher(
        [this]()
        {
            return new droidinput::InputDispatcher(the_dispatcher_policy());
        });
}
