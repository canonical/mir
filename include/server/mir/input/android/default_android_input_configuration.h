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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_ANDROID_DEFAULT_ANDROID_INPUT_CONFIGURATION_H_
#define MIR_INPUT_ANDROID_DEFAULT_ANDROID_INPUT_CONFIGURATION_H_

#include "mir/input/input_configuration.h"

#include "mir/cached_ptr.h"

namespace android
{
class InputReaderInterface;
class InputReaderPolicyInterface;
class InputListenerInterface;
class EventHubInterface;
}

namespace droidinput = android;

namespace mir
{
namespace input
{
class InputRegion;
class CursorListener;
class TouchVisualizer;
class InputReport;
class InputDispatcher;

namespace android
{
class InputThread;

class DefaultInputConfiguration : public mir::input::InputConfiguration
{
public:
    DefaultInputConfiguration(std::shared_ptr<input::InputDispatcher> const& dispatcher,
                              std::shared_ptr<input::InputRegion> const& input_region,
                              std::shared_ptr<CursorListener> const& cursor_listener,
                              std::shared_ptr<TouchVisualizer> const& touch_visualizer,
                              std::shared_ptr<input::InputReport> const& input_report);
    virtual ~DefaultInputConfiguration();

    std::shared_ptr<input::InputManager> the_input_manager() override;

protected:
    virtual std::shared_ptr<droidinput::EventHubInterface> the_event_hub();
    virtual std::shared_ptr<droidinput::InputReaderInterface> the_reader();
    virtual std::shared_ptr<droidinput::InputListenerInterface> the_input_translator();

    virtual std::shared_ptr<InputThread> the_reader_thread();

    virtual std::shared_ptr<droidinput::InputReaderPolicyInterface> the_reader_policy();

private:
    CachedPtr<input::InputManager> input_manager;
    CachedPtr<InputThread> reader_thread;

    CachedPtr<droidinput::EventHubInterface> event_hub;
    CachedPtr<droidinput::InputReaderPolicyInterface> reader_policy;
    CachedPtr<droidinput::InputReaderInterface> reader;

    std::shared_ptr<input::InputDispatcher> const input_dispatcher;
    std::shared_ptr<input::InputRegion> const input_region;
    std::shared_ptr<CursorListener> const cursor_listener;
    std::shared_ptr<TouchVisualizer> const touch_visualizer;
    std::shared_ptr<input::InputReport> const input_report;
};
}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_DEFAULT_ANDROID_INPUT_CONFIGURATION_H_
