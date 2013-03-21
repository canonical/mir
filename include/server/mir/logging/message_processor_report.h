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

#ifndef MIR_LOGGING_MESSAGE_PROCESSOR_REPORT_H_
#define MIR_LOGGING_MESSAGE_PROCESSOR_REPORT_H_

#include "mir/frontend/message_processor_report.h"

namespace mir
{
namespace logging
{
class MessageProcessorReport : public mir::frontend::MessageProcessorReport
{
    void received_invocation(void const* mediator, int id, std::string const& method);

    void completed_invocation(void const* mediator, int id, bool result);

    void unknown_method(void const* mediator, int id, std::string const& method);

    void exception_handled(void const* mediator, int id, std::exception const& error);

    void exception_handled(void const* mediator, std::exception const& error);
};
}
}

#endif /* MIR_LOGGING_MESSAGE_PROCESSOR_REPORT_H_ */
