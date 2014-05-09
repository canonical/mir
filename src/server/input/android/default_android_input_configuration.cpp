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
                Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/input/android/default_android_input_configuration.h"
#include "event_filter_dispatcher_policy.h"
#include "android_input_reader_policy.h"
#include "android_input_thread.h"
#include "android_input_registrar.h"
#include "android_input_targeter.h"
#include "android_input_target_enumerator.h"
#include "android_input_manager.h"
#include "input_dispatcher_configuration.h"
#include "input_translator.h"
#include "input_channel_factory.h"

#include "mir/input/event_filter.h"
#include "mir/input/input_dispatcher_configuration.h"

#include <EventHub.h>
#include <InputDispatcher.h>

#include "mir/input/event_filter.h"
#include "mir/input/input_dispatcher_configuration.h"

#include <EventHub.h>
#include <InputDispatcher.h>
#include <InputReader.h>

#include <string>

namespace droidinput = android;

namespace mi = mir::input;
namespace mia = mi::android;
namespace ms = mir::scene;
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

mia::DefaultInputConfiguration::DefaultInputConfiguration(
    std::shared_ptr<mi::InputDispatcherConfiguration> const& input_dispatcher_config,
    std::shared_ptr<mi::InputRegion> const& input_region,
    std::shared_ptr<CursorListener> const& cursor_listener,
    std::shared_ptr<mi::InputReport> const& input_report) :
    input_dispatcher_config(input_dispatcher_config),
    input_region(input_region),
    cursor_listener(cursor_listener),
    input_report(input_report)
{
}

mia::DefaultInputConfiguration::~DefaultInputConfiguration()
{
}

std::shared_ptr<mi::InputChannelFactory> mia::DefaultInputConfiguration::the_input_channel_factory()
{
    return std::make_shared<mia::InputChannelFactory>();
}

droidinput::sp<droidinput::EventHubInterface> mia::DefaultInputConfiguration::the_event_hub()
{
    return event_hub(
        [this]()
        {
            return new droidinput::EventHub(input_report);
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
            auto android_conf = std::dynamic_pointer_cast<mia::InputDispatcherConfiguration>(
                input_dispatcher_config);

            if (android_conf)
                return new droidinput::InputReader(
                    the_event_hub(), the_reader_policy(), android_conf->the_dispatcher());
            else
                return new droidinput::InputReader(
                    the_event_hub(), the_reader_policy(), the_input_translator());
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

droidinput::sp<droidinput::InputListenerInterface> mia::DefaultInputConfiguration::the_input_translator()
{
    return new mia::InputTranslator(input_dispatcher_config->the_input_dispatcher());
}

std::shared_ptr<mi::InputManager> mia::DefaultInputConfiguration::the_input_manager()
{
    return input_manager(
        [this]() -> std::shared_ptr<mi::InputManager>
        {
            return std::make_shared<mia::InputManager>(
                the_event_hub(),
                the_reader_thread());
        });
}

