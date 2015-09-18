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

#include "android/android_input_reader_policy.h"
#include "android/input_sender.h"
#include "android/input_channel_factory.h"
#include "android/input_translator.h"
#include "key_repeat_dispatcher.h"
#include "android/input_reader_dispatchable.h"
#include "display_input_region.h"
#include "event_filter_chain_dispatcher.h"
#include "cursor_controller.h"
#include "touchspot_controller.h"
#include "null_input_manager.h"
#include "null_input_dispatcher.h"
#include "null_input_targeter.h"
#include "xcursor_loader.h"
#include "builtin_cursor_images.h"
#include "null_input_send_observer.h"
#include "null_input_channel_factory.h"
#include "default_input_device_hub.h"
#include "default_input_manager.h"
#include "surface_input_dispatcher.h"

#include "mir/input/touch_visualizer.h"
#include "mir/input/platform.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/compositor/scene.h"
#include "mir/emergency_cleanup.h"
#include "mir/report/legacy_input_report.h"
#include "mir/main_loop.h"
#include "mir/shared_library.h"
#include "mir/glib_main_loop.h"
#include "mir/dispatch/action_queue.h"

#include "mir_toolkit/cursors.h"

#include <EventHub.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mr = mir::report;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace md = mir::dispatch;

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
        [this]()
        {
            return the_event_filter_chain_dispatcher();
        });
}

std::shared_ptr<mi::EventFilterChainDispatcher>
mir::DefaultServerConfiguration::the_event_filter_chain_dispatcher()
{
    return event_filter_chain_dispatcher(
        [this]() -> std::shared_ptr<mi::EventFilterChainDispatcher>
        {
            std::initializer_list<std::shared_ptr<mi::EventFilter> const> filter_list {default_filter};
            return std::make_shared<mi::EventFilterChainDispatcher>(filter_list, the_surface_input_dispatcher());
        });
}

namespace
{
class NullInputSender : public mi::InputSender
{
public:
    virtual void send_event(MirEvent const&, std::shared_ptr<mi::InputChannel> const& ) {}
};

}

std::shared_ptr<mi::InputSender>
mir::DefaultServerConfiguration::the_input_sender()
{
    return input_sender(
        [this]() -> std::shared_ptr<mi::InputSender>
        {
        if (!the_options()->get<bool>(options::enable_input_opt))
            return std::make_shared<NullInputSender>();
        else
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
                return the_surface_input_dispatcher();
        });
}

std::shared_ptr<mi::SurfaceInputDispatcher>
mir::DefaultServerConfiguration::the_surface_input_dispatcher()
{
    return surface_input_dispatcher(
        [this]()
        {
            return std::make_shared<mi::SurfaceInputDispatcher>(the_input_scene());
        });
}

std::shared_ptr<mi::InputDispatcher>
mir::DefaultServerConfiguration::the_input_dispatcher()
{
    return input_dispatcher(
        [this]()
        {
            std::chrono::milliseconds const key_repeat_timeout{500};
            std::chrono::milliseconds const key_repeat_delay{50};

            auto const options = the_options();
            auto enable_repeat = options->get<bool>(options::enable_key_repeat_opt);

            return std::make_shared<mi::KeyRepeatDispatcher>(
                the_event_filter_chain_dispatcher(), the_main_loop(), the_cookie_factory(),
                enable_repeat, key_repeat_timeout, key_repeat_delay);
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

std::shared_ptr<mir::input::LegacyInputDispatchable>
mir::DefaultServerConfiguration::the_legacy_input_dispatchable()
{
    return legacy_input_dispatchable(
        [this]()
        {
            return std::make_shared<mia::InputReaderDispatchable>(the_event_hub(), the_input_reader());
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

std::shared_ptr<droidinput::InputListenerInterface>
mir::DefaultServerConfiguration::the_input_translator()
{
    return input_translator(
        [this]()
        {
            return std::make_shared<mia::InputTranslator>(the_input_dispatcher(), the_cookie_factory());
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
            auto visualizer = std::make_shared<mi::TouchspotController>(the_buffer_allocator(),
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

std::shared_ptr<mi::Platform>
mir::DefaultServerConfiguration::the_input_platform()
{
    return input_platform(
        [this]() -> std::shared_ptr<mi::Platform>
        {
            auto options = the_options();

            if (!options->is_set(options::platform_input_lib))
                return nullptr;

            auto lib = std::make_shared<mir::SharedLibrary>(
                options->get<std::string>(options::platform_input_lib));
            auto create = lib->load_function<mi::CreatePlatform>(
                "create_input_platform",
                MIR_SERVER_INPUT_PLATFORM_VERSION);
            return create(the_options(), the_emergency_cleanup(), the_input_device_registry(), the_input_report());
        });
}

namespace
{
class NullLegacyInputDispatchable : public mi::LegacyInputDispatchable
{
public:
    void start() override {};
    mir::Fd watch_fd() const override { return aq.watch_fd();};
    bool dispatch(md::FdEvents events) override { return aq.dispatch(events); }
    md::FdEvents relevant_events() const override{ return aq.relevant_events(); }

private:
    md::ActionQueue aq;
};
}

std::shared_ptr<mi::InputManager>
mir::DefaultServerConfiguration::the_input_manager()
{
    // As the input configuration is structured now, if there is no
    // InputReader (as in the nested case) there will be nothing to instate
    // and keep alive the cursor and its controller.
    // We use the CursorControllingInputManager for this purpose.
    struct CursorControllingInputManager : public mi::NullInputManager
    {
        CursorControllingInputManager(
            std::shared_ptr<mi::CursorListener> const& cursor_listener)
            : cursor_listener(cursor_listener)
        {
        }

        std::shared_ptr<mi::CursorListener> const cursor_listener;
    };

    return input_manager(
        [this]() -> std::shared_ptr<mi::InputManager>
        {
            auto const options = the_options();
            bool input_opt = options->get<bool>(options::enable_input_opt);
            bool host_platform = input_opt && !options->is_set(options::host_socket_opt);
            // TODO nested input handling (== host_socket) should fold into a platform

            if (host_platform)
            {
                if (options->get<std::string>(options::legacy_input_report_opt) == options::log_opt_value)
                    mr::legacy_input::initialize(the_logger());

                std::shared_ptr<mi::InputManager> ret;

                auto platform = the_input_platform();
                if (platform)
                {
                    ret = std::make_shared<mi::DefaultInputManager>(the_input_reading_multiplexer(),
                                                                    std::make_shared<NullLegacyInputDispatchable>());
                    ret->add_platform(platform);
                }
                else
                {
                    ret = std::make_shared<mi::DefaultInputManager>(
                        the_input_reading_multiplexer(), the_legacy_input_dispatchable());
                }
                return ret;
            }
            else
                return std::make_shared<mi::NullInputManager>();
        }
    );
}

std::shared_ptr<mir::dispatch::MultiplexingDispatchable>
mir::DefaultServerConfiguration::the_input_reading_multiplexer()
{
    return input_reading_multiplexer(
        [this]() -> std::shared_ptr<mir::dispatch::MultiplexingDispatchable>
        {
            return std::make_shared<mir::dispatch::MultiplexingDispatchable>();
        }
    );
}

std::shared_ptr<mi::InputDeviceRegistry> mir::DefaultServerConfiguration::the_input_device_registry()
{
    return default_input_device_hub([this]()
                                    {
                                        return std::make_shared<mi::DefaultInputDeviceHub>(
                                            the_input_dispatcher(),
                                            the_input_reading_multiplexer(),
                                            the_main_loop(),
                                            the_touch_visualizer(),
                                            the_cursor_listener(),
                                            the_input_region(),
                                            the_cookie_factory());
                                    });
}

std::shared_ptr<mi::InputDeviceHub> mir::DefaultServerConfiguration::the_input_device_hub()
{
    return default_input_device_hub([this]()
                                    {
                                        return std::make_shared<mi::DefaultInputDeviceHub>(
                                            the_input_dispatcher(),
                                            the_input_reading_multiplexer(),
                                            the_main_loop(),
                                            the_touch_visualizer(),
                                            the_cursor_listener(),
                                            the_input_region(),
                                            the_cookie_factory());
                                    });
}
