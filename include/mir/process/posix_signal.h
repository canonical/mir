/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Thomas Guest <thomas.guest@canonical.com>
 */

#ifndef MIR_PROCESS_POSIX_SIGNAL_H_
#define MIR_PROCESS_POSIX_SIGNAL_H_

#include <signal.h>

namespace mir
{
namespace process
{
namespace posix
{

enum class Signal : int
{
    unknown = _NSIG+1,
    hangup = SIGHUP,
    interrupt = SIGINT,
    quit = SIGQUIT,
    illegal_instruction = SIGILL,
    trace_trap = SIGTRAP,
    abort = SIGABRT,
    iot_trap = SIGIOT,
    bus_error = SIGBUS,
    floating_point_exception = SIGFPE,
    kill = SIGKILL,
    user1 = SIGUSR1,
    user2 = SIGUSR2,
    segmentation_violation = SIGSEGV,
    broken_pipe = SIGPIPE,
    alarm_clock = SIGALRM,
    terminate = SIGTERM,
    child_status_changed = SIGCHLD,
    cont = SIGCONT,
    stop = SIGSTOP
};

}
}
}

#endif // MIR_PROCESS_POSIX_SIGNAL_H_
