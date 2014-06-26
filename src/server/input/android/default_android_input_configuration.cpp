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
#include "input_translator.h"
#include "common_input_thread.h"

#include "mir/input/event_filter.h"

#include <EventHub.h>
#include <InputDispatcher.h>

#include "mir/input/event_filter.h"

#include <EventHub.h>
#include <InputDispatcher.h>
#include <InputReader.h>

#include <string>

namespace droidinput = android;

namespace mi = mir::input;
namespace mia = mi::android;
namespace ms = mir::scene;
namespace msh = mir::shell;

mia::DefaultInputConfiguration::DefaultInputConfiguration(
    std::shared_ptr<mi::InputDispatcher> const& input_dispatcher,
    std::shared_ptr<mi::InputRegion> const& input_region,
    std::shared_ptr<CursorListener> const& cursor_listener,
    std::shared_ptr<TouchVisualizer> const& touch_visualizer,
    std::shared_ptr<mi::InputReport> const& input_report) :
    input_dispatcher(input_dispatcher),
    input_region(input_region),
    cursor_listener(cursor_listener),
    touch_visualizer(touch_visualizer),
    input_report(input_report)
{
}

mia::DefaultInputConfiguration::~DefaultInputConfiguration()
{
}

std::shared_ptr<droidinput::EventHubInterface> mia::DefaultInputConfiguration::the_event_hub()
{
    return event_hub(
        [this]()
        {
            return std::make_shared<droidinput::EventHub>(input_report);
        });
}

std::shared_ptr<droidinput::InputReaderPolicyInterface> mia::DefaultInputConfiguration::the_reader_policy()
{
    return reader_policy(
        [this]()
        {
            return std::make_shared<mia::InputReaderPolicy>(input_region, cursor_listener, touch_visualizer);
        });
}


std::shared_ptr<droidinput::InputReaderInterface> mia::DefaultInputConfiguration::the_reader()
{
    return reader(
        [this]()
        {
            return std::make_shared<droidinput::InputReader>(
                the_event_hub(), the_reader_policy(), the_input_translator());
        });
}

std::shared_ptr<mia::InputThread> mia::DefaultInputConfiguration::the_reader_thread()
{
    return reader_thread(
        [this]()
        {
            return std::make_shared<CommonInputThread>("Mir/InputReader",
                                                       new droidinput::InputReaderThread(the_reader()));
        });
}

std::shared_ptr<droidinput::InputListenerInterface> mia::DefaultInputConfiguration::the_input_translator()
{
    return std::make_shared<mia::InputTranslator>(input_dispatcher);
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

