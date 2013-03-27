/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "default_android_input_configuration.h"
#include "event_filter_dispatcher_policy.h"
#include "android_input_reader_policy.h"
#include "android_input_thread.h"
#include "../event_filter_chain.h"

#include <EventHub.h>
#include <InputDispatcher.h>
#include <InputReader.h>

#include <string>

namespace droidinput = android;

namespace mi = mir::input;
namespace mia = mi::android;
namespace mg = mir::graphics;

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

mia::DefaultInputConfiguration::DefaultInputConfiguration(std::initializer_list<std::shared_ptr<mi::EventFilter> const> const& filters,
                                                          std::shared_ptr<mg::ViewableArea> const& view_area,
                                                          std::shared_ptr<mi::CursorListener> const& cursor_listener)
  : filter_chain(std::make_shared<mi::EventFilterChain>(filters)),
    view_area(view_area),
    cursor_listener(cursor_listener)
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

droidinput::sp<droidinput::InputReaderPolicyInterface> mia::DefaultInputConfiguration::the_reader_policy()
{
    return reader_policy(
        [this]()
        {
            return new mia::InputReaderPolicy(view_area, cursor_listener);
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
