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
#include "android/common_input_thread.h"
#include "android/android_input_registrar.h"
#include "android/android_input_target_enumerator.h"
#include "android/event_filter_dispatcher_policy.h"
#include "display_input_region.h"
#include "event_filter_chain.h"
#include "nested_input_configuration.h"
#include "null_input_configuration.h"
#include "cursor_controller.h"
#include "null_input_dispatcher.h"
#include "null_input_targeter.h"

#include "mir/input/android/default_android_input_configuration.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"
#include "mir/compositor/scene.h"
#include "mir/report/legacy_input_report.h"

#include <InputDispatcher.h>


namespace mi = mir::input;
namespace mia = mi::android;
namespace mr = mir::report;
namespace ms = mir::scene;
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

std::shared_ptr<mi::InputConfiguration>
mir::DefaultServerConfiguration::the_input_configuration()
{
    return input_configuration(
    [this]() -> std::shared_ptr<mi::InputConfiguration>
    {
        auto const options = the_options();
        if (!options->get<bool>(options::enable_input_opt))
        {
            return std::make_shared<mi::NullInputConfiguration>();
        }
        else if (!options->is_set(options::host_socket_opt))
        {
            // fallback to standalone if host socket is unset
            return std::make_shared<mia::DefaultInputConfiguration>(
                the_input_dispatcher(),
                the_input_region(),
                the_cursor_listener(),
                the_input_report()
                );
        }
        else
        {
            return std::make_shared<mi::NestedInputConfiguration>();
        }
    });
}

std::shared_ptr<droidinput::InputDispatcherInterface>
mir::DefaultServerConfiguration::the_android_input_dispatcher()
{
    return android_input_dispatcher(
        [this]()
        {
            auto registrar = the_input_registrar();
            auto dispatcher = std::make_shared<droidinput::InputDispatcher>(
                the_dispatcher_policy(),
                the_input_report(),
                std::make_shared<mia::InputTargetEnumerator>(the_input_targets(), registrar));
            registrar->set_dispatcher(dispatcher);
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
            if (the_options()->get<std::string>(options::legacy_input_report_opt) == options::log_opt_value)
                    mr::legacy_input::initialize(the_logger());
            return the_input_configuration()->the_input_manager();
        });
}

std::shared_ptr<mi::InputChannelFactory> mir::DefaultServerConfiguration::the_input_channel_factory()
{
    return the_input_configuration()->the_input_channel_factory();
}

std::shared_ptr<mi::CursorListener>
mir::DefaultServerConfiguration::the_cursor_listener()
{
    return cursor_listener(
        [this]() -> std::shared_ptr<mi::CursorListener>
        {
            return std::make_shared<mi::CursorController>(the_input_targets(), 
                the_cursor(), the_default_cursor_image());
        });

}
