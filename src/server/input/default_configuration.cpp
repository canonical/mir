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
 */

#include "mir/default_server_configuration.h"

#include "android/android_input_dispatcher.h"
#include "android/android_input_targeter.h"
#include "android/android_input_reader_policy.h"
#include "android/common_input_thread.h"
#include "android/android_input_reader_policy.h"
#include "android/android_input_registrar.h"
#include "android/android_input_target_enumerator.h"
#include "android/event_filter_dispatcher_policy.h"
#include "android/input_sender.h"
#include "android/input_channel_factory.h"
#include "android/android_input_manager.h"
#include "android/input_translator.h"
#include "display_input_region.h"
#include "event_filter_chain.h"
#include "cursor_controller.h"
#include "touchspot_controller.h"
#include "null_input_manager.h"
#include "null_input_dispatcher.h"
#include "null_input_targeter.h"
#include "xcursor_loader.h"
#include "builtin_cursor_images.h"
#include "null_input_send_observer.h"
#include "null_input_channel_factory.h"

#include "mir/input/touch_visualizer.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"
#include "mir/compositor/scene.h"
#include "mir/report/legacy_input_report.h"
#include "mir/main_loop.h"

#include <InputDispatcher.h>
#include <EventHub.h>
#include <InputReader.h>


namespace mi = mir::input;
namespace mia = mi::android;
namespace mr = mir::report;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace msh = mir::shell;

std::shared_ptr<mi::InputRegion> mir::DefaultServerConfiguration::the_input_region()
{
    return input_region(
        [this]()
        {
            return std::make_shared<mi::DisplayInputRegion>(the_display());
        });
}

std::shared_ptr<mi::CompositeEventFilter>
mir::DefaultServerConfiguration::the_composite_event_filter()
{
    return composite_event_filter(
        [this]() -> std::shared_ptr<mi::CompositeEventFilter>
        {
            std::initializer_list<std::shared_ptr<mi::EventFilter> const> filter_list {default_filter};
            return std::make_shared<mi::EventFilterChain>(filter_list);
        });
}

std::shared_ptr<droidinput::InputEnumerator>
mir::DefaultServerConfiguration::the_input_target_enumerator()
{
    return input_target_enumerator(
        [this]()
        {
            return std::make_shared<mia::InputTargetEnumerator>(the_input_scene(), the_input_registrar());
        });
}


std::shared_ptr<droidinput::InputDispatcherInterface>
mir::DefaultServerConfiguration::the_android_input_dispatcher()
{
    return android_input_dispatcher(
        [this]()
        {
            auto dispatcher = std::make_shared<droidinput::InputDispatcher>(
                the_dispatcher_policy(),
                the_input_report(),
                the_input_target_enumerator());
            the_input_registrar()->set_dispatcher(dispatcher);
            return dispatcher;
        });
}

std::shared_ptr<mia::InputRegistrar>
mir::DefaultServerConfiguration::the_input_registrar()
{
    return input_registrar(
        [this]()
        {
            return std::make_shared<mia::InputRegistrar>(the_scene());
        });
}

std::shared_ptr<mi::InputSender>
mir::DefaultServerConfiguration::the_input_sender()
{
    return input_sender(
        [this]()
        {
            return std::make_shared<mia::InputSender>(the_scene(), the_main_loop(), the_input_send_observer(), the_input_report());
        });
}

std::shared_ptr<mi::InputSendObserver>
mir::DefaultServerConfiguration::the_input_send_observer()
{
    return input_send_observer(
        [this]()
        {
            return std::make_shared<mi::NullInputSendObserver>();
        });
}


std::shared_ptr<msh::InputTargeter>
mir::DefaultServerConfiguration::the_input_targeter()
{
    return input_targeter(
        [this]() -> std::shared_ptr<msh::InputTargeter>
        {
            auto const options = the_options();
            if (!options->get<bool>(options::enable_input_opt))
                return std::make_shared<mi::NullInputTargeter>();
            else
                return std::make_shared<mia::InputTargeter>(the_android_input_dispatcher(), the_input_registrar());
        });
}

std::shared_ptr<mia::InputThread>
mir::DefaultServerConfiguration::the_dispatcher_thread()
{
    return dispatcher_thread(
        [this]()
        {
            return std::make_shared<mia::CommonInputThread>("Mir/InputDisp",
                                                       new droidinput::InputDispatcherThread(the_android_input_dispatcher()));
        });
}

std::shared_ptr<droidinput::InputDispatcherPolicyInterface>
mir::DefaultServerConfiguration::the_dispatcher_policy()
{
    return android_dispatcher_policy(
        [this]()
        {
            return std::make_shared<mia::EventFilterDispatcherPolicy>(the_composite_event_filter(), is_key_repeat_enabled());
        });
}

bool mir::DefaultServerConfiguration::is_key_repeat_enabled() const
{
    return true;
}

std::shared_ptr<mi::InputDispatcher>
mir::DefaultServerConfiguration::the_input_dispatcher()
{
    return input_dispatcher(
        [this]() -> std::shared_ptr<mi::InputDispatcher>
        {
            auto const options = the_options();
            if (!options->get<bool>(options::enable_input_opt))
                return std::make_shared<mi::NullInputDispatcher>();
            else
            {
                return std::make_shared<mia::AndroidInputDispatcher>(the_android_input_dispatcher(), the_dispatcher_thread());
            }
        });
}

std::shared_ptr<mi::InputManager>
mir::DefaultServerConfiguration::the_input_manager()
{
    return input_manager(
        [&, this]() -> std::shared_ptr<mi::InputManager>
        {
            auto const options = the_options();
            bool input_reading_required =
                options->get<bool>(options::enable_input_opt) &&
                !options->is_set(options::host_socket_opt);

            if (input_reading_required)
            {
                if (options->get<std::string>(options::legacy_input_report_opt) == options::log_opt_value)
                        mr::legacy_input::initialize(the_logger());

                return std::make_shared<mia::InputManager>(
                    the_event_hub(),
                    the_input_reader_thread());
            }
            else
                return std::make_shared<mi::NullInputManager>();
        });
}

std::shared_ptr<droidinput::EventHubInterface>
mir::DefaultServerConfiguration::the_event_hub()
{
    return event_hub(
        [this]()
        {
            return std::make_shared<droidinput::EventHub>(the_input_report());
        });
}

std::shared_ptr<droidinput::InputReaderPolicyInterface>
mir::DefaultServerConfiguration::the_input_reader_policy()
{
    return input_reader_policy(
        [this]()
        {
            return std::make_shared<mia::InputReaderPolicy>(the_input_region(), the_cursor_listener(), the_touch_visualizer());
        });
}

std::shared_ptr<droidinput::InputReaderInterface>
mir::DefaultServerConfiguration::the_input_reader()
{
    return input_reader(
        [this]()
        {
            return std::make_shared<droidinput::InputReader>(the_event_hub(), the_input_reader_policy(), the_input_translator());
        });
}

std::shared_ptr<mia::InputThread>
mir::DefaultServerConfiguration::the_input_reader_thread()
{
    return input_reader_thread(
        [this]()
        {
            return std::make_shared<mia::CommonInputThread>("Mir/InputReader", new droidinput::InputReaderThread(the_input_reader()));
        });
}

std::shared_ptr<droidinput::InputListenerInterface>
mir::DefaultServerConfiguration::the_input_translator()
{
    return input_translator(
        [this]()
        {
            return std::make_shared<mia::InputTranslator>(the_input_dispatcher());
        });
}

std::shared_ptr<mi::InputChannelFactory> mir::DefaultServerConfiguration::the_input_channel_factory()
{
    auto const options = the_options();
    if (!options->get<bool>(options::enable_input_opt))
        return std::make_shared<mi::NullInputChannelFactory>();
    else
        return std::make_shared<mia::InputChannelFactory>();
}

std::shared_ptr<mi::CursorListener>
mir::DefaultServerConfiguration::the_cursor_listener()
{
    return cursor_listener(
        [this]() -> std::shared_ptr<mi::CursorListener>
        {
            return wrap_cursor_listener(std::make_shared<mi::CursorController>(
                    the_input_scene(),
                    the_cursor(),
                    the_default_cursor_image()));
        });

}

std::shared_ptr<mi::CursorListener>
mir::DefaultServerConfiguration::wrap_cursor_listener(
    std::shared_ptr<mi::CursorListener> const& wrapped)
{
    return wrapped;
}

std::shared_ptr<mi::TouchVisualizer>
mir::DefaultServerConfiguration::the_touch_visualizer()
{
    return touch_visualizer(
        [this]() -> std::shared_ptr<mi::TouchVisualizer>
        {
            auto visualizer = std::make_shared<mi::TouchspotController>(the_buffer_allocator(), the_buffer_writer(),
                the_input_scene());

            // The visualizer is disabled by default and can be enabled statically via
            // the MIR_SERVER_ENABLE_TOUCHSPOTS option. In the USC/unity8/autopilot case
            // it will be toggled at runtime via com.canonical.Unity.Screen DBus interface
            if (the_options()->is_set(options::touchspots_opt))
            {
                visualizer->enable();
            }
            
            return visualizer;
        });
}

std::shared_ptr<mg::CursorImage>
mir::DefaultServerConfiguration::the_default_cursor_image()
{
    return default_cursor_image(
        [this]()
        {
            return the_cursor_images()->image(mir_default_cursor_name, mi::default_cursor_size);
        });
}

namespace
{
bool has_default_cursor(mi::CursorImages& images)
{
    if (images.image(mir_default_cursor_name, mi::default_cursor_size))
        return true;
    return false;
}
}

std::shared_ptr<mi::CursorImages>
mir::DefaultServerConfiguration::the_cursor_images()
{
    return cursor_images(
        [this]() -> std::shared_ptr<mi::CursorImages>
        {
            auto xcursor_loader = std::make_shared<mi::XCursorLoader>();
            if (has_default_cursor(*xcursor_loader))
                return xcursor_loader;
            else
                return std::make_shared<mi::BuiltinCursorImages>();
        });
}
