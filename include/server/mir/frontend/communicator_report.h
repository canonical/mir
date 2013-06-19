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

#ifndef MIR_FRONTEND_COMMUNICATOR_REPORT_H_
#define MIR_FRONTEND_COMMUNICATOR_REPORT_H_

#include <stdexcept>

namespace mir
{
namespace frontend
{

class CommunicatorReport
{
public:

    virtual void error(std::exception const& error) = 0;

protected:
    virtual ~CommunicatorReport() = default;
    CommunicatorReport() = default;
    CommunicatorReport(const CommunicatorReport&) = delete;
    CommunicatorReport& operator=(const CommunicatorReport&) = delete;
};

class NullCommunicatorReport : public CommunicatorReport
{
public:

    void error(std::exception const& error);
};
}
}

#endif // MIR_FRONTEND_COMMUNICATOR_REPORT_H_
