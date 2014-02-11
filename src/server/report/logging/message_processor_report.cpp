/*
 * Copyright © 2013 Canonical Ltd.
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

#include "message_processor_report.h"
#include "mir/logging/logger.h"

#include <boost/exception/diagnostic_information.hpp>
#include <sstream>

namespace ml = mir::logging;
namespace mrl = mir::report::logging;

namespace
{
char const* const component = "frontend::MessageProcessor";
}


mrl::MessageProcessorReport::MessageProcessorReport(
    std::shared_ptr<ml::Logger> const& log,
    std::shared_ptr<time::Clock> const& clock) :
    log(log),
    clock(clock)
{
}

mrl::MessageProcessorReport::~MessageProcessorReport() noexcept(true)
{
    if (!mediators.empty())
    {
        std::ostringstream out;

        {
            out << "Calls outstanding on exit:\n";

            std::lock_guard<std::mutex> lock(mutex);

            for (auto const& med : mediators)
            {
                for (auto const & invocation: med.second.current_invocations)
                {
                    out << "mediator=" << med.first << ": "
                        << invocation.second.method << "()";

                    if (!invocation.second.exception.empty())
                        out << " ERROR=" << invocation.second.exception;

                    out << '\n';
                }
            }
        }

        log->log(ml::Logger::informational, out.str(), component);
    }
}


void mrl::MessageProcessorReport::received_invocation(void const* mediator, int id, std::string const& method)
{
    std::ostringstream out;
    out << "mediator=" << mediator << ", method=" << method << "()";
    log->log(ml::Logger::debug, out.str(), component);

    std::lock_guard<std::mutex> lock(mutex);
    auto& invocations = mediators[mediator].current_invocations;
    auto& invocation = invocations[id];

    invocation.start = clock->sample();
    invocation.method = method;
}

void mrl::MessageProcessorReport::completed_invocation(void const* mediator, int id, bool result)
{
    auto const end = clock->sample();
    std::ostringstream out;

    {
        std::lock_guard<std::mutex> lock(mutex);

        auto const pm = mediators.find(mediator);
        if (pm != mediators.end())
        {
            auto& invocations = pm->second.current_invocations;

            auto const pi = invocations.find(id);
            if (pi != invocations.end())
            {
                out << "mediator=" << mediator << ": "
                    << pi->second.method << "(), elapsed="
                    << std::chrono::duration_cast<std::chrono::microseconds>(end - pi->second.start).count()
                    << "µs";

                if (!pi->second.exception.empty())
                    out << " ERROR=" << pi->second.exception;

                if (!result)
                    out << " (disconnecting)";
            }

            invocations.erase(pi);

            if (invocations.empty())
                mediators.erase(mediator);
        }
    }

    log->log(ml::Logger::informational, out.str(), component);
}

void mrl::MessageProcessorReport::unknown_method(void const* mediator, int id, std::string const& method)
{
    std::ostringstream out;
    out << "mediator=" << mediator << ", id=" << id << ", UNKNOWN method=\"" << method << "\"";
    log->log(ml::Logger::warning, out.str(), component);

    std::lock_guard<std::mutex> lock(mutex);
    auto const pm = mediators.find(mediator);
    if (pm != mediators.end())
        mediators.erase(mediator);
}

void mrl::MessageProcessorReport::exception_handled(void const* mediator, int id, std::exception const& error)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto const pm = mediators.find(mediator);
    if (pm != mediators.end())
    {
        auto& invocations = pm->second.current_invocations;

        auto const pi = invocations.find(id);
        if (pi != invocations.end())
        {
            pi->second.exception = boost::diagnostic_information(error);
        }
    }
}

void mrl::MessageProcessorReport::exception_handled(void const* mediator, std::exception const& error)
{
    std::ostringstream out;
    out << "mediator=" << mediator << ", ERROR: " << boost::diagnostic_information(error);
    log->log(ml::Logger::informational, out.str(), component);

    std::lock_guard<std::mutex> lock(mutex);
    auto const pm = mediators.find(mediator);
    if (pm != mediators.end())
        mediators.erase(mediator);
}

void mrl::MessageProcessorReport::sent_event(void const* mediator, MirSurfaceEvent const& event)
{
    std::ostringstream out;
    out << "mediator=" << mediator << ", sent event, surface id=" << event.id;
    log->log(ml::Logger::debug, out.str(), component);
}
