/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_host_lifecycle_event_listener.h"

#include "mir/server.h"
#include "mir/logging/logger.h"
#include "mir/options/option.h"

#include <cstdio>

namespace me = mir::examples;

me::NestedLifecycleEventListener::NestedLifecycleEventListener(std::shared_ptr<mir::logging::Logger> const& logger) :
    logger(logger)
{
}

void me::NestedLifecycleEventListener::lifecycle_event_occurred(MirLifecycleState state)
{
    static char const* const text[] =
        {
            "mir_lifecycle_state_will_suspend",
            "mir_lifecycle_state_resumed",
            "mir_lifecycle_connection_lost"
        };

    char buffer[128];

    snprintf(buffer, sizeof(buffer), "Lifecycle event occurred : state = %s", text[state]);

    logger->log(logging::Severity::informational, buffer, "NestedLifecycleEventListener");
}

void me::add_log_host_lifecycle_option_to(mir::Server& server)
{
    static const char* const launch_child_opt = "log-host-lifecycle";
    static const char* const launch_client_descr = "Write lifecycle events from host to log";

    server.add_configuration_option(launch_child_opt, launch_client_descr, mir::OptionType::null);

    server.override_the_host_lifecycle_event_listener([&]()
       ->std::shared_ptr<shell::HostLifecycleEventListener>
       {
           if (server.get_options()->is_set(launch_child_opt))
               return std::make_shared<NestedLifecycleEventListener>(server.the_logger());
           else
               return std::shared_ptr<shell::HostLifecycleEventListener>{};
       });
}
