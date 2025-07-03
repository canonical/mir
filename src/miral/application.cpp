/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miral/application.h"

#include <mir/scene/session.h>
#include <mir/log.h>

#include <unistd.h>

#include <csignal>

void miral::kill(Application const& application, int sig)
{
    if (application)
    {
        auto const pid = application->process_id();

        if (pid != getpid())
            ::kill(pid, sig);
    }
}

auto miral::name_of(Application const& application) -> std::string
{
    return application->name();
}

auto miral::pid_of(Application const& application) -> pid_t
{
    return application->process_id();
}

auto miral::socket_fd_of(Application const& application) -> int
{
    return application->socket_fd();
}

void miral::apply_lifecycle_state_to(Application const& /*application*/, MirLifecycleState /*state*/)
{
    mir::log_warning("Use of unimplemented function: miral::apply_lifecycle_state_to()");
}
