/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk
 */

#ifndef MIR_FRONTEND_CONNECTOR_REPORT_H_
#define MIR_FRONTEND_CONNECTOR_REPORT_H_

#include <stdexcept>
#include <string>

namespace mir
{
namespace frontend
{

class ConnectorReport
{
public:

    virtual void thread_start() = 0;
    virtual void thread_end() = 0;

    virtual void creating_session_for(int socket_handle) = 0;
    virtual void creating_socket_pair(int server_handle, int client_handle) = 0;

    virtual void listening_on(std::string const& endpoint) = 0;

    virtual void error(std::exception const& error) = 0;
    virtual void warning(std::string  const& error) = 0;

protected:
    virtual ~ConnectorReport() = default;
    ConnectorReport() = default;
    ConnectorReport(const ConnectorReport&) = delete;
    ConnectorReport& operator=(const ConnectorReport&) = delete;
};

}
}

#endif // MIR_FRONTEND_CONNECTOR_REPORT_H_
