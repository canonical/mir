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


#include "mir/input/android/dispatcher_input_configuration.h"
#include "event_filter_dispatcher_policy.h"
#include "android_input_thread.h"
#include "android_input_registrar.h"
#include "android_input_targeter.h"
#include "android_input_target_enumerator.h"
#include "android_input_manager.h"
#include "mir/input/event_filter.h"

#include <InputDispatcher.h>

#include <string>

namespace droidinput = android;

namespace mi = mir::input;
namespace mia = mi::android;
namespace ms = mir::surfaces;
namespace msh = mir::shell;

namespace
{
class CommonInputThread : public mia::InputThread
{
public:
    CommonInputThread(std::string const& name, droidinput::sp<droidinput::Thread> const& thread)
      : name(name),
        thread(thread)
    {
    }
    virtual ~CommonInputThread()
    {
    }

    void start()
    {
        thread->run(name.c_str(), droidinput::PRIORITY_URGENT_DISPLAY);
    }
    void request_stop()
    {
        thread->requestExit();
    }
    void join()
    {
        thread->join();
    }

private:
    CommonInputThread(const CommonInputThread&) = delete;
    CommonInputThread& operator=(const CommonInputThread&) = delete;

    std::string const name;
    droidinput::sp<droidinput::Thread> const thread;
};
}

mia::DispatcherInputConfiguration::DispatcherInputConfiguration(
    std::shared_ptr<mi::EventFilter> const& event_filter,
    std::shared_ptr<mi::InputRegion> const& input_region,
    std::shared_ptr<mi::CursorListener> const& cursor_listener,
    std::shared_ptr<mi::InputReport> const& input_report) :
    event_filter(event_filter),
    input_region(input_region),
    cursor_listener(cursor_listener),
    input_report(input_report)
{
}

mia::DispatcherInputConfiguration::~DispatcherInputConfiguration()
{
}

droidinput::sp<droidinput::InputDispatcherPolicyInterface> mia::DispatcherInputConfiguration::the_dispatcher_policy()
{
    return dispatcher_policy(
        [this]()
        {
            return new mia::EventFilterDispatcherPolicy(event_filter, is_key_repeat_enabled());
        });
}

std::shared_ptr<mia::InputThread> mia::DispatcherInputConfiguration::the_dispatcher_thread()
{
    return dispatcher_thread(
        [this]()
        {
            return std::make_shared<CommonInputThread>("InputDispatcher",
                                                       new droidinput::InputDispatcherThread(the_dispatcher()));
        });
}

std::shared_ptr<ms::InputRegistrar> mia::DispatcherInputConfiguration::the_input_registrar()
{
    return input_registrar(
        [this]()
        {
            return std::make_shared<mia::InputRegistrar>(the_dispatcher());
        });
}

std::shared_ptr<mia::WindowHandleRepository> mia::DispatcherInputConfiguration::the_window_handle_repository()
{
    return input_registrar(
        [this]()
        {
            return std::make_shared<mia::InputRegistrar>(the_dispatcher());
        });
}

std::shared_ptr<msh::InputTargeter> mia::DispatcherInputConfiguration::the_input_targeter()
{
    return input_targeter(
        [this]()
        {
            return std::make_shared<mia::InputTargeter>(the_dispatcher(), the_window_handle_repository());
        });
}

bool mia::DispatcherInputConfiguration::is_key_repeat_enabled()
{
    return true;
}

void mia::DispatcherInputConfiguration::set_input_targets(std::shared_ptr<mi::InputTargets> const& targets)
{
    the_dispatcher()->setInputEnumerator(new mia::InputTargetEnumerator(targets, the_window_handle_repository()));
}

std::shared_ptr<mi::InputManager> mia::DispatcherInputConfiguration::the_input_manager()
{
    return input_manager(
        [this]()
        {
            return std::make_shared<mia::InputDispatcherManager>(the_dispatcher(), the_dispatcher_thread());
        });
}
