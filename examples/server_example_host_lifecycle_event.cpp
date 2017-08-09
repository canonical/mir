/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_host_lifecycle_event_listener.h"

#include "mir/server.h"
#include "mir/logging/logger.h"
#include "mir/options/option.h"

#include <cstdio>

namespace me = mir::examples;

me::HostLifecycleEventListener::HostLifecycleEventListener(std::shared_ptr<mir::logging::Logger> const& logger) :
    logger(logger)
{
}

void me::HostLifecycleEventListener::lifecycle_event_occurred(MirLifecycleState state)
{
    static char const* const text[] =
        {
            "mir_lifecycle_state_will_suspend",
            "mir_lifecycle_state_resumed",
            "mir_lifecycle_connection_lost"
        };

    char buffer[128];

    snprintf(buffer, sizeof(buffer), "Lifecycle event occurred : state = %s", text[state]);

    logger->log(logging::Severity::informational, buffer, "example");
}

void me::add_log_host_lifecycle_option_to(mir::Server& server)
{
    static const char* const host_lifecycle_opt = "log-host-lifecycle";
    static const char* const host_lifecycle_descr = "Write lifecycle events from host to log";

    server.add_configuration_option(host_lifecycle_opt, host_lifecycle_descr, mir::OptionType::null);

    server.override_the_host_lifecycle_event_listener([&]()
       ->std::shared_ptr<shell::HostLifecycleEventListener>
       {
           if (server.get_options()->is_set(host_lifecycle_opt))
               return std::make_shared<HostLifecycleEventListener>(server.the_logger());
           else
               return std::shared_ptr<shell::HostLifecycleEventListener>{};
       });
}
