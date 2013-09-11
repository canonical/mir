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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_INPUT_NESTED_INPUT_CONFIGURATION_H_
#define MIR_INPUT_NESTED_INPUT_CONFIGURATION_H_

#include "mir/input/android/dispatcher_input_configuration.h"

namespace mir
{
namespace input
{
class NestedInputRelay;

class NestedInputConfiguration : public android::DispatcherInputConfiguration
{
public:
    NestedInputConfiguration(
        std::shared_ptr<NestedInputRelay> const& input_relay,
        std::shared_ptr<EventFilter> const& event_filter,
        std::shared_ptr<InputRegion> const& input_region,
        std::shared_ptr<CursorListener> const& cursor_listener,
        std::shared_ptr<InputReport> const& input_report);
    virtual ~NestedInputConfiguration();

private:
    droidinput::sp<droidinput::InputDispatcherInterface> the_dispatcher() override;

    std::shared_ptr<NestedInputRelay> const input_relay;

    android::CachedAndroidPtr<droidinput::InputDispatcherInterface> dispatcher;
};
}
}

#endif /* MIR_INPUT_NESTED_INPUT_CONFIGURATION_H_ */
