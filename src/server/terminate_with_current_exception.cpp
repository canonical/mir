/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mir/terminate_with_current_exception.h"

#include <unistd.h>
#include <csignal>

#include <exception>
#include <mutex>

namespace
{
std::exception_ptr termination_exception;
std::mutex termination_exception_mutex;
}

void mir::clear_termination_exception()
{
    std::lock_guard<std::mutex> lock{termination_exception_mutex};
    termination_exception = nullptr;
}

void mir::check_for_termination_exception()
{
    std::lock_guard<std::mutex> lock{termination_exception_mutex};
    if (termination_exception)
        std::rethrow_exception(termination_exception);
}

void mir::terminate_with_current_exception()
{
    std::lock_guard<std::mutex> lock{termination_exception_mutex};
    if (!termination_exception)
    {
        termination_exception = std::current_exception();
        kill(getpid(), SIGTERM);
    }
}
