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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_REPORT_EXCEPTION_H_
#define MIR_REPORT_EXCEPTION_H_

#include <iosfwd>

namespace mir
{
/**
 *  Call this from a catch block (and only from a catch block)
 *  to write error information to an output stream.
 */
void report_exception(std::ostream& out);

/**
 *  Call this from a catch block (and only from a catch block)
 *  to write error information to std:cerr.
 */
void report_exception();
}



#endif /* MIR_REPORT_EXCEPTION_H_ */
