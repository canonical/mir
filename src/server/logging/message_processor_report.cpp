/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/logging/message_processor_report.h"
#include "mir/logging/logger.h"

#include <boost/exception/diagnostic_information.hpp>
#include <sstream>

namespace ml = mir::logging;

namespace
{
char const* const component = "frontend::MessageProcessor";
}


ml::MessageProcessorReport::MessageProcessorReport(std::shared_ptr<Logger> const& log) :
    log(log)
{
}

void ml::MessageProcessorReport::received_invocation(void const* mediator, int id, std::string const& method)
{
    std::ostringstream out;
    out << "mediator=" << mediator << ", id=" << id << ", method=\"" << method << "\"";
    log->log<Logger::informational>(out.str(), component);
}

void ml::MessageProcessorReport::completed_invocation(void const* mediator, int id, bool result)
{
    std::ostringstream out;
    out << "mediator=" << mediator << ", id=" << id << " - completed " <<(result ? "continue" : "disconnect");
    log->log<Logger::informational>(out.str(), component);
}

void ml::MessageProcessorReport::unknown_method(void const* mediator, int id, std::string const& method)
{
    std::ostringstream out;
    out << "mediator=" << mediator << ", id=" << id << ", UNKNOWN method=\"" << method << "\"";
    log->log<Logger::informational>(out.str(), component);
}

void ml::MessageProcessorReport::exception_handled(void const* mediator, int id, std::exception const& error)
{
    std::ostringstream out;
    out << "mediator=" << mediator << ", id=" << id << ", ERROR: " << boost::diagnostic_information(error);
    log->log<Logger::informational>(out.str(), component);
}

void ml::MessageProcessorReport::exception_handled(void const* mediator, std::exception const& error)
{
    std::ostringstream out;
    out << "mediator=" << mediator << ", ERROR: " << boost::diagnostic_information(error);
    log->log<Logger::informational>(out.str(), component);
}




