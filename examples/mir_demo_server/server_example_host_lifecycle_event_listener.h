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

#ifndef MIR_EXAMPLE_HOST_LIFECYCLE_EVENT_LISTENER_H_
#define MIR_EXAMPLE_HOST_LIFECYCLE_EVENT_LISTENER_H_

#include "mir/shell/host_lifecycle_event_listener.h"

#include <memory>

namespace mir
{
class Server;

namespace logging { class Logger; }

namespace examples
{
class HostLifecycleEventListener : public shell::HostLifecycleEventListener
{
public:
    HostLifecycleEventListener(std::shared_ptr<mir::logging::Logger> const& logger);
    void lifecycle_event_occurred(MirLifecycleState state) override;

private:
    std::shared_ptr<mir::logging::Logger> const logger;
};

void add_log_host_lifecycle_option_to(mir::Server& server);
}
}

#endif /* MIR_EXAMPLE_HOST_LIFECYCLE_EVENT_LISTENER_H_ */
