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


#ifndef MIR_INPUT_ANDROID_DISPATCHER_INPUT_CONFIGURATION_H_
#define MIR_INPUT_ANDROID_DISPATCHER_INPUT_CONFIGURATION_H_

#include "mir/input/input_configuration.h"

#include "mir/cached_ptr.h"

#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

#include <functional>

namespace droidinput = android;

namespace android
{
class InputReaderInterface;
class InputReaderPolicyInterface;
class InputDispatcherPolicyInterface;
class InputDispatcherInterface;
class EventHubInterface;
}

namespace mir
{
namespace shell
{
class InputTargeter;
}
namespace input
{
class EventFilter;
class CursorListener;
class InputReport;
class InputRegion;

namespace android
{
class InputRegistrar;
class WindowHandleRepository;
class InputThread;

template <typename Type>
class CachedAndroidPtr
{
    droidinput::wp<Type> cache;

    CachedAndroidPtr(CachedAndroidPtr const&) = delete;
    CachedAndroidPtr& operator=(CachedAndroidPtr const&) = delete;

public:
    CachedAndroidPtr() = default;

    droidinput::sp<Type> operator()(std::function<droidinput::sp<Type>()> make)
    {
        auto result = cache.promote();
        if (!result.get())
        {
            cache = result = make();
        }
        return result;
    }
};

class DispatcherInputConfiguration : public input::InputConfiguration
{
public:
    DispatcherInputConfiguration(std::shared_ptr<EventFilter> const& event_filter,
                              std::shared_ptr<input::InputRegion> const& input_region,
                              std::shared_ptr<CursorListener> const& cursor_listener,
                              std::shared_ptr<input::InputReport> const& input_report);
    virtual ~DispatcherInputConfiguration();

    std::shared_ptr<scene::InputRegistrar> the_input_registrar();
    std::shared_ptr<shell::InputTargeter> the_input_targeter();
    std::shared_ptr<input::InputManager> the_input_manager();

    void set_input_targets(std::shared_ptr<input::InputTargets> const& targets);

    virtual bool is_key_repeat_enabled();

protected:
    virtual droidinput::sp<droidinput::InputDispatcherInterface> the_dispatcher() = 0;

    virtual std::shared_ptr<InputThread> the_dispatcher_thread();

    virtual droidinput::sp<droidinput::InputDispatcherPolicyInterface> the_dispatcher_policy();

    std::shared_ptr<WindowHandleRepository> the_window_handle_repository();

    std::shared_ptr<EventFilter> const event_filter;
    std::shared_ptr<input::InputRegion> const input_region;
    std::shared_ptr<CursorListener> const cursor_listener;
    std::shared_ptr<input::InputReport> const input_report;

    CachedPtr<input::InputManager> input_manager;

private:
    CachedPtr<InputThread> dispatcher_thread;
    CachedPtr<InputRegistrar> input_registrar;

    CachedPtr<shell::InputTargeter> input_targeter;

    CachedAndroidPtr<droidinput::InputDispatcherPolicyInterface> dispatcher_policy;
};
}
}
} // namespace mir

#endif /* MIR_INPUT_ANDROID_DISPATCHER_INPUT_CONFIGURATION_H_ */
