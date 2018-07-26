/*
 * Copyright © 2013, 2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_REPORT_NULL_MESSAGE_PROCESSOR_REPORT_H_
#define MIR_REPORT_NULL_MESSAGE_PROCESSOR_REPORT_H_

#include "mir/frontend/message_processor_report.h"

namespace mir
{
namespace report
{
namespace null
{
class MessageProcessorReport : public frontend::MessageProcessorReport
{
    void received_invocation(void const*, int, std::string const&);

    void completed_invocation(void const*, int, bool);

    void unknown_method(void const*, int, std::string const&);

    void exception_handled(void const*, int, std::exception const&);

    void exception_handled(void const*, std::exception const&);
};
}
}
}

#endif /* MIR_REPORT_NULL_MESSAGE_PROCESSOR_REPORT_H_ */
