/*
 * Copyright © 2013-2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Thomi Richards <thomi.richards@canonical.com>
 */

#include "threading.h"

#include "client.h"
#include "results.h"

#include <sstream>
#include <thread>


std::string get_application_unique_name()
{
    std::stringstream strstr;
    strstr << std::this_thread::get_id();
    return strstr.str();
}

ThreadResults run_mir_test(std::chrono::seconds for_seconds)
{
    std::string thread_name(get_application_unique_name());
    ThreadResults results(thread_name);
    auto const end_time = std::chrono::steady_clock::now() + for_seconds;

    while (!terminate_signalled.load() && (end_time > std::chrono::steady_clock::now()))
    {
        ClientStateMachine::Ptr p = ClientStateMachine::Create();
        if (!p->connect(thread_name))
        {
            results.addTestResult(false);
        }
        else
        {
            if (!p->create_surface())
            {
                results.addTestResult(false);
            }
            else
            {
                p->release_surface();
            }
            p->disconnect();
            results.addTestResult(true);
        }
    }
    return results;
}
