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

#include "mir/frontend/null_message_processor_report.h"

namespace mf = mir::frontend;

void mf::NullMessageProcessorReport::received_invocation(void const*, int, std::string const&)
{
}

void mf::NullMessageProcessorReport::completed_invocation(void const*, int, bool)
{
}

void mf::NullMessageProcessorReport::unknown_method(void const*, int, std::string const&)
{
}

void mf::NullMessageProcessorReport::exception_handled(void const*, int, std::exception const&)
{
}

void mf::NullMessageProcessorReport::exception_handled(void const*, std::exception const&)
{
}

void mf::NullMessageProcessorReport::sent_event(void const*, MirSurfaceEvent const&)
{
}
