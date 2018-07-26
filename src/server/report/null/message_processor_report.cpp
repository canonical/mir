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

#include "message_processor_report.h"

namespace mrn = mir::report::null;

void mrn::MessageProcessorReport::received_invocation(void const*, int, std::string const&)
{
}

void mrn::MessageProcessorReport::completed_invocation(void const*, int, bool)
{
}

void mrn::MessageProcessorReport::unknown_method(void const*, int, std::string const&)
{
}

void mrn::MessageProcessorReport::exception_handled(void const*, int, std::exception const&)
{
}

void mrn::MessageProcessorReport::exception_handled(void const*, std::exception const&)
{
}
