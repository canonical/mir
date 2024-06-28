/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/default_server_configuration.h"
#include "mir/fatal.h"
#include "mir/options/default_configuration.h"
#include "mir/glib_main_loop.h"
#include "mir/default_server_status_listener.h"
#include "mir/emergency_cleanup.h"
#include "mir/frontend/wayland.h"

#include "mir/logging/dumb_console_logger.h"
#include "mir/options/program_option.h"
#include "mir/frontend/session_credentials.h"
#include "mir/frontend/session_authorizer.h"
#include "mir/graphics/cursor.h"
#include "mir/scene/null_session_listener.h"
#include "mir/graphics/display.h"
#include "mir/input/cursor_listener.h"
#include "mir/input/vt_filter.h"
#include "mir/input/input_manager.h"
#include "mir/time/steady_clock.h"
#include "mir/geometry/rectangles.h"
#include "mir/scene/null_prompt_session_listener.h"
#include "default_emergency_cleanup.h"
#include "mir/graphics/platform.h"
#include "mir/console_services.h"
#include "mir/misc_options.h"
#include <memory>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace ml = mir::logging;
namespace mo = mir::options;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mi = mir::input;

mir::DefaultServerConfiguration::DefaultServerConfiguration(int argc, char const* argv[]) :
        DefaultServerConfiguration(std::make_shared<mo::DefaultConfiguration>(argc, argv))
{
}

mir::DefaultServerConfiguration::DefaultServerConfiguration(std::shared_ptr<mo::Configuration> const& configuration_options) :
    configuration_options(configuration_options),
    enabled_wayland_extensions(frontend::get_standard_extensions())
{
}

auto mir::DefaultServerConfiguration::the_options() const
->std::shared_ptr<options::Option>
{
    return configuration_options->the_options();
}

std::shared_ptr<ms::SessionListener>
mir::DefaultServerConfiguration::the_session_listener()
{
    return session_listener(
        []
        {
            return std::make_shared<ms::NullSessionListener>();
        });
}

std::shared_ptr<ms::PromptSessionListener>
mir::DefaultServerConfiguration::the_prompt_session_listener()
{
    return prompt_session_listener(
        []
        {
            return std::make_shared<ms::NullPromptSessionListener>();
        });
}

std::shared_ptr<mf::SessionAuthorizer>
mir::DefaultServerConfiguration::the_session_authorizer()
{
    struct DefaultSessionAuthorizer : public mf::SessionAuthorizer
    {
        bool connection_is_allowed(mf::SessionCredentials const& /* creds */) override
        {
            return true;
        }

        bool configure_display_is_allowed(mf::SessionCredentials const& /* creds */) override
        {
            return true;
        }

        bool set_base_display_configuration_is_allowed(mf::SessionCredentials const& /* creds */) override
        {
            return true;
        }

        bool screencast_is_allowed(mf::SessionCredentials const& /* creds */) override
        {
            return true;
        }

        bool prompt_session_is_allowed(mf::SessionCredentials const& /* creds */) override
        {
            return true;
        }

        bool configure_input_is_allowed(mf::SessionCredentials const& /* creds */) override
        {
            return true;
        }

        bool set_base_input_configuration_is_allowed(mf::SessionCredentials const& /* creds */) override
        {
            return true;
        }

    };
    return session_authorizer(
        [&]()
        {
            return std::make_shared<DefaultSessionAuthorizer>();
        });
}

std::shared_ptr<mir::time::Clock> mir::DefaultServerConfiguration::the_clock()
{
    return clock(
        []()
        {
            return std::make_shared<mir::time::SteadyClock>();
        });
}

std::shared_ptr<mir::MainLoop> mir::DefaultServerConfiguration::the_main_loop()
{
    return main_loop(
        [this]() -> std::shared_ptr<mir::MainLoop>
        {
            return std::make_shared<mir::GLibMainLoop>(the_clock());
        });
}

std::shared_ptr<mir::ServerActionQueue> mir::DefaultServerConfiguration::the_server_action_queue()
{
    return the_main_loop();
}

std::shared_ptr<mir::ServerStatusListener> mir::DefaultServerConfiguration::the_server_status_listener()
{
    return server_status_listener(
        []()
        {
            return std::make_shared<mir::DefaultServerStatusListener>();
        });
}

std::shared_ptr<mir::EmergencyCleanup> mir::DefaultServerConfiguration::the_emergency_cleanup()
{
    return emergency_cleanup(
        []()
        {
            return std::make_shared<DefaultEmergencyCleanup>();
        });
}

std::function<void()> mir::DefaultServerConfiguration::the_stop_callback()
{
    return []{};
}

auto mir::DefaultServerConfiguration::the_fatal_error_strategy()
-> void (*)(char const* reason, ...)
{
    if (the_options()->is_set(options::fatal_except_opt))
        return &fatal_error_except;
    else
        return fatal_error;
}

auto mir::DefaultServerConfiguration::the_logger()
    -> std::shared_ptr<ml::Logger>
{
    return logger(
        []() -> std::shared_ptr<ml::Logger>
        {
            return std::make_shared<ml::DumbConsoleLogger>();
        });
}

std::shared_ptr<mir::MiscOptions>
mir::DefaultServerConfiguration::the_misc_options()
{
    if(!misc_options)
    {
        misc_options = std::make_shared<MiscOptions>();
    }

    return misc_options;
}
