/*
 * Copyright © 2013-2016 Canonical Ltd.
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

#include "android/input_sender.h"
#include "android/input_channel_factory.h"
#include "key_repeat_dispatcher.h"
#include "display_input_region.h"
#include "event_filter_chain_dispatcher.h"
#include "cursor_controller.h"
#include "touchspot_controller.h"
#include "null_input_manager.h"
#include "null_input_dispatcher.h"
#include "null_input_targeter.h"
#include "builtin_cursor_images.h"
#include "null_input_channel_factory.h"
#include "default_input_device_hub.h"
#include "default_input_manager.h"
#include "surface_input_dispatcher.h"
#include "basic_seat.h"
#include "../graphics/nested/mir_client_host_connection.h"

#include "mir/input/touch_visualizer.h"
#include "mir/input/input_probe.h"
#include "mir/input/platform.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/compositor/scene.h"
#include "mir/emergency_cleanup.h"
#include "mir/main_loop.h"
#include "mir/abnormal_exit.h"
#include "mir/glib_main_loop.h"
#include "mir/log.h"
#include "mir/dispatch/action_queue.h"

#include "mir_toolkit/cursors.h"

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
            return std::make_shared<mi::DisplayInputRegion>();
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
            return std::make_shared<mia::InputSender>(the_scene(), the_main_loop(), the_input_report());
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
                the_event_filter_chain_dispatcher(), the_main_loop(), the_cookie_authority(),
                enable_repeat, key_repeat_timeout, key_repeat_delay);
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

std::shared_ptr<mi::CursorImages>
mir::DefaultServerConfiguration::the_cursor_images()
{
    return cursor_images(
        [this]() -> std::shared_ptr<mi::CursorImages>
        {
            return std::make_shared<mi::BuiltinCursorImages>();
        });
}

std::shared_ptr<mi::InputManager>
mir::DefaultServerConfiguration::the_input_manager()
{
    return input_manager(
        [this]() -> std::shared_ptr<mi::InputManager>
        {
            auto const options = the_options();
            bool input_opt = options->get<bool>(options::enable_input_opt);

            if (!input_opt)
            {
                return std::make_shared<mi::NullInputManager>();
            }
            else if (options->is_set(options::host_socket_opt))
            {
                // TODO nested input handling (== host_socket) should fold into a platform
                return std::make_shared<mi::NullInputManager>();
            }
            else
            {
                auto const emergency_cleanup = the_emergency_cleanup();
                auto const device_registry = the_input_device_registry();
                auto const input_report = the_input_report();

                // Maybe the graphics platform also supplies input (e.g. mesa-x11 or nested)
                // NB this makes the (valid) assumption that graphics initializes before input
                auto platform = mi::input_platform_from_graphics_module(
                    *the_graphics_platform(), *options, emergency_cleanup, device_registry, input_report);

                // otherwise (usually) we probe for it
                if (!platform)
                {
                    platform = probe_input_platforms(*options, emergency_cleanup, device_registry,
                                                     input_report, *the_shared_library_prober_report());
                }

                return std::make_shared<mi::DefaultInputManager>(the_input_reading_multiplexer(), std::move(platform));
            }
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

std::shared_ptr<mi::Seat> mir::DefaultServerConfiguration::the_seat()
{
    return seat(
        [this]()
        {
            return std::make_shared<mi::BasicSeat>(
                    the_input_dispatcher(),
                    the_touch_visualizer(),
                    the_cursor_listener(),
                    the_input_region());
        });
}

std::shared_ptr<mi::InputDeviceRegistry> mir::DefaultServerConfiguration::the_input_device_registry()
{
    return default_input_device_hub(
        [this]()
        {
            auto input_dispatcher = the_input_dispatcher();
            auto key_repeater = std::dynamic_pointer_cast<mi::KeyRepeatDispatcher>(input_dispatcher);
            auto hub = std::make_shared<mi::DefaultInputDeviceHub>(
                the_global_event_sink(),
                the_seat(),
                the_input_reading_multiplexer(),
                the_main_loop(),
                the_cookie_authority());

            if (key_repeater)
                key_repeater->set_input_device_hub(hub);
            return hub;
        });
}

std::shared_ptr<mi::InputDeviceHub> mir::DefaultServerConfiguration::the_input_device_hub()
{
    auto options = the_options();
    if (options->is_set(options::host_socket_opt))
    {
        return the_mir_client_host_connection();
    }
    else
    {
        return default_input_device_hub(
            [this]()
            {
                auto input_dispatcher = the_input_dispatcher();
                auto key_repeater = std::dynamic_pointer_cast<mi::KeyRepeatDispatcher>(input_dispatcher);
                auto hub = std::make_shared<mi::DefaultInputDeviceHub>(
                    the_global_event_sink(),
                    the_seat(),
                    the_input_reading_multiplexer(),
                    the_main_loop(),
                    the_cookie_authority());

                if (key_repeater)
                    key_repeater->set_input_device_hub(hub);
                return hub;
            });
    }
}
