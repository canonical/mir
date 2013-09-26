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
 * Authored by: Alan Griffiths <alan@octopull.co.uk
 */

#ifndef MIR_FRONTEND_CONNECTOR_REPORT_H_
#define MIR_FRONTEND_CONNECTOR_REPORT_H_

#include <stdexcept>

namespace mir
{
namespace frontend
{

class ConnectorReport
{
public:

    virtual void error(std::exception const& error) = 0;

protected:
    virtual ~ConnectorReport() = default;
    ConnectorReport() = default;
    ConnectorReport(const ConnectorReport&) = delete;
    ConnectorReport& operator=(const ConnectorReport&) = delete;
};

class NullConnectorReport : public ConnectorReport
{
public:

    void error(std::exception const& error);
};
}
}

#endif // MIR_FRONTEND_CONNECTOR_REPORT_H_
