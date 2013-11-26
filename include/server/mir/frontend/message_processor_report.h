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


#ifndef MIR_FRONTEND_MESSAGE_PROCESSOR_REPORT_H_
#define MIR_FRONTEND_MESSAGE_PROCESSOR_REPORT_H_

#include "mir_toolkit/event.h"

#include <string>

namespace mir
{
namespace frontend
{

class MessageProcessorReport
{
public:
    MessageProcessorReport() {}
    virtual ~MessageProcessorReport() = default;

    virtual void received_invocation(void const* mediator, int id, std::string const& method) = 0;

    virtual void completed_invocation(void const* mediator, int id, bool result) = 0;

    virtual void unknown_method(void const* mediator, int id, std::string const& method) = 0;

    virtual void exception_handled(void const* mediator, int id, std::exception const& error) = 0;

    virtual void exception_handled(void const* mediator, std::exception const& error) = 0;

    virtual void sent_event(void const* mediator, MirSurfaceEvent const& ev) = 0;

private:
    MessageProcessorReport(MessageProcessorReport const&) = delete;
    MessageProcessorReport& operator=(MessageProcessorReport const&) = delete;
};
}
}

#endif /* MIR_FRONTEND_MESSAGE_PROCESSOR_REPORT_H_ */
