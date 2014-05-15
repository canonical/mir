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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 *              Andreas Pokorny <andreas.pokorny@canonical.com>
 */


#ifndef MIR_INPUT_ANDROID_DISPATCHER_INPUT_CONFIGURATION_H_
#define MIR_INPUT_ANDROID_DISPATCHER_INPUT_CONFIGURATION_H_

#include "mir/input/input_dispatcher_configuration.h"

#include "mir/cached_ptr.h"

namespace android
{
class InputReaderPolicyInterface;
class InputDispatcherPolicyInterface;
class InputDispatcherInterface;
}

namespace droidinput = android;

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

class InputDispatcherConfiguration : public input::InputDispatcherConfiguration
{
public:
    InputDispatcherConfiguration(std::shared_ptr<EventFilter> const& event_filter,
                                 std::shared_ptr<input::InputReport> const& input_report,
                                 std::shared_ptr<compositor::Scene> const& scene,
                                 std::shared_ptr<input::InputTargets> const& targets);
    virtual ~InputDispatcherConfiguration();

    std::shared_ptr<InputRegistrar> the_input_registrar();
    std::shared_ptr<shell::InputTargeter> the_input_targeter() override;
    std::shared_ptr<input::InputDispatcher> the_input_dispatcher() override;
    virtual std::shared_ptr<droidinput::InputDispatcherInterface> the_dispatcher();

    bool is_key_repeat_enabled() const override;

protected:
    virtual std::shared_ptr<InputThread> the_dispatcher_thread();

    virtual std::shared_ptr<droidinput::InputDispatcherPolicyInterface> the_dispatcher_policy();

    std::shared_ptr<EventFilter> const event_filter;
    std::shared_ptr<input::InputReport> const input_report;
    std::shared_ptr<compositor::Scene> const scene;
    std::shared_ptr<input::InputTargets> const input_targets;

private:
    CachedPtr<InputThread> dispatcher_thread;
    CachedPtr<InputRegistrar> input_registrar;

    CachedPtr<shell::InputTargeter> input_targeter;

    CachedPtr<droidinput::InputDispatcherPolicyInterface> dispatcher_policy;
    CachedPtr<droidinput::InputDispatcherInterface> dispatcher;
    CachedPtr<input::InputDispatcher> input_dispatcher;
};
}
}
} // namespace mir

#endif /* MIR_INPUT_ANDROID_DISPATCHER_INPUT_CONFIGURATION_H_ */
