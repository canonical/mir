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

#include "default_android_input_configuration.h"
#include "event_filter_dispatcher_policy.h"
#include "android_input_reader_policy.h"
#include "android_input_thread.h"
#include "android_input_registrar.h"
#include "android_input_targeter.h"
#include "android_input_target_enumerator.h"
#include "android_input_manager.h"
#include "mir/input/event_filter.h"

#include <EventHub.h>
#include <InputDispatcher.h>
#include <InputReader.h>

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

protected:
    CommonInputThread(const CommonInputThread&) = delete;
    CommonInputThread& operator=(const CommonInputThread&) = delete;

private:
    std::string const name;
    droidinput::sp<droidinput::Thread> const thread;
};
}

mia::DefaultInputConfiguration::DefaultInputConfiguration(std::shared_ptr<mi::EventFilter> const& event_filter,
                                                          std::shared_ptr<mi::InputRegion> const& input_region,
                                                          std::shared_ptr<mi::CursorListener> const& cursor_listener,
                                                          std::shared_ptr<mi::InputReport> const& input_report)
  : event_filter(event_filter),
    input_region(input_region),
    cursor_listener(cursor_listener),
    input_report(input_report)
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
            return new droidinput::EventHub(input_report);
        });
}

droidinput::sp<droidinput::InputDispatcherPolicyInterface> mia::DefaultInputConfiguration::the_dispatcher_policy()
{
    return dispatcher_policy(
        [this]()
        {
            return new mia::EventFilterDispatcherPolicy(event_filter, is_key_repeat_enabled());
        });
}

droidinput::sp<droidinput::InputDispatcherInterface> mia::DefaultInputConfiguration::the_dispatcher()
{
    return dispatcher(
        [this]() -> droidinput::sp<droidinput::InputDispatcherInterface>
        {
            return new droidinput::InputDispatcher(the_dispatcher_policy(), input_report);
        });
}

droidinput::sp<droidinput::InputReaderPolicyInterface> mia::DefaultInputConfiguration::the_reader_policy()
{
    return reader_policy(
        [this]()
        {
            return new mia::InputReaderPolicy(input_region, cursor_listener);
        });
}


droidinput::sp<droidinput::InputReaderInterface> mia::DefaultInputConfiguration::the_reader()
{
    return reader(
        [this]()
        {
            return new droidinput::InputReader(the_event_hub(), the_reader_policy(), the_dispatcher());
        });
}

std::shared_ptr<mia::InputThread> mia::DefaultInputConfiguration::the_dispatcher_thread()
{
    return dispatcher_thread(
        [this]()
        {
            return std::make_shared<CommonInputThread>("InputDispatcher",
                                                       new droidinput::InputDispatcherThread(the_dispatcher()));
        });
}

std::shared_ptr<mia::InputThread> mia::DefaultInputConfiguration::the_reader_thread()
{
    return reader_thread(
        [this]()
        {
            return std::make_shared<CommonInputThread>("InputReader",
                                                       new droidinput::InputReaderThread(the_reader()));
        });
}

std::shared_ptr<ms::InputRegistrar> mia::DefaultInputConfiguration::the_input_registrar()
{
    return input_registrar(
        [this]()
        {
            return std::make_shared<mia::InputRegistrar>(the_dispatcher());
        });
}

std::shared_ptr<mia::WindowHandleRepository> mia::DefaultInputConfiguration::the_window_handle_repository()
{
    return input_registrar(
        [this]()
        {
            return std::make_shared<mia::InputRegistrar>(the_dispatcher());
        });
}

std::shared_ptr<msh::InputTargeter> mia::DefaultInputConfiguration::the_input_targeter()
{
    return input_targeter(
        [this]()
        {
            return std::make_shared<mia::InputTargeter>(the_dispatcher(), the_window_handle_repository());
        });
}

std::shared_ptr<mi::InputManager> mia::DefaultInputConfiguration::the_input_manager()
{
    return input_manager(
        [this]()
        {
            return std::make_shared<mia::InputManager>(the_event_hub(), the_dispatcher(),
                                                       the_reader_thread(), the_dispatcher_thread());
        });
}

bool mia::DefaultInputConfiguration::is_key_repeat_enabled()
{
    return true;
}

void mia::DefaultInputConfiguration::set_input_targets(std::shared_ptr<mi::InputTargets> const& targets)
{
    the_dispatcher()->setInputEnumerator(new mia::InputTargetEnumerator(targets, the_window_handle_repository()));
}
