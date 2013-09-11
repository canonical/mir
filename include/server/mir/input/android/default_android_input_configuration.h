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

#ifndef MIR_INPUT_ANDROID_DEFAULT_ANDROID_INPUT_CONFIGURATION_H_
#define MIR_INPUT_ANDROID_DEFAULT_ANDROID_INPUT_CONFIGURATION_H_

#include "mir/input/android/dispatcher_input_configuration.h"

namespace mir
{
namespace input
{
namespace android
{
class DefaultInputConfiguration : public DispatcherInputConfiguration
{
public:
    DefaultInputConfiguration(std::shared_ptr<EventFilter> const& event_filter,
                              std::shared_ptr<input::InputRegion> const& input_region,
                              std::shared_ptr<CursorListener> const& cursor_listener,
                              std::shared_ptr<input::InputReport> const& input_report);
    virtual ~DefaultInputConfiguration();

    std::shared_ptr<input::InputManager> the_input_manager();

protected:
    virtual droidinput::sp<droidinput::EventHubInterface> the_event_hub();
    virtual droidinput::sp<droidinput::InputReaderInterface> the_reader();

    virtual std::shared_ptr<InputThread> the_reader_thread();

    virtual droidinput::sp<droidinput::InputReaderPolicyInterface> the_reader_policy();

private:
    droidinput::sp<droidinput::InputDispatcherInterface> the_dispatcher() override;

    CachedPtr<InputThread> reader_thread;

    CachedAndroidPtr<droidinput::EventHubInterface> event_hub;
    CachedAndroidPtr<droidinput::InputReaderPolicyInterface> reader_policy;
    CachedAndroidPtr<droidinput::InputReaderInterface> reader;
    CachedAndroidPtr<droidinput::InputDispatcherInterface> dispatcher;
};
}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_DEFAULT_ANDROID_INPUT_CONFIGURATION_H_
