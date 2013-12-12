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

#include "connector_report.h"

#include "mir/logging/logger.h"

#include <boost/exception/diagnostic_information.hpp>

#include <sstream>
#include <thread>

namespace ml = mir::logging;

namespace
{
char const* const component = "frontend::Connector";
}

ml::ConnectorReport::ConnectorReport(std::shared_ptr<Logger> const& log) :
    logger(log)
{
}

void ml::ConnectorReport::thread_start()
{
    std::stringstream ss;
    ss << "thread (" << std::this_thread::get_id() << ") started.";
    logger->log(Logger::informational, ss.str(), component);
}

void ml::ConnectorReport::thread_end()
{
    std::stringstream ss;
    ss << "thread (" << std::this_thread::get_id() << ") ended.";
    logger->log(Logger::informational, ss.str(), component);
}

void ml::ConnectorReport::starting_threads(int count)
{
    std::stringstream ss;
    ss << "Starting " << count << " threads.";
    logger->log(Logger::informational, ss.str(), component);
}

void ml::ConnectorReport::stopping_threads(int count)
{
    std::stringstream ss;
    ss << "Stopping " << count << " threads.";
    logger->log(Logger::informational, ss.str(), component);
}

void ml::ConnectorReport::creating_session_for(int socket_handle)
{
    std::stringstream ss;
    ss << "thread (" << std::this_thread::get_id() << ") Creating session for socket " << socket_handle;
    logger->log(Logger::informational, ss.str(), component);
}

void ml::ConnectorReport::creating_socket_pair(int server_handle, int client_handle)
{
    std::stringstream ss;
    ss << "thread (" << std::this_thread::get_id() << ") Creating socket pair (server=" << server_handle << ", client=" << client_handle << ").";
    logger->log(Logger::informational, ss.str(), component);
}

void ml::ConnectorReport::listening_on(std::string const& endpoint)
{
    std::stringstream ss;
    ss << "Listening on endpoint: " << endpoint;
    logger->log(Logger::informational, ss.str(), component);
}

void ml::ConnectorReport::error(std::exception const& error)
{
    std::stringstream ss;
    ss << "thread (" << std::this_thread::get_id() << ") Error: " << boost::diagnostic_information(error);

    logger->log(ml::Logger::warning, ss.str(), component);
}



