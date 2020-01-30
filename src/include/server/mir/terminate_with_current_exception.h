/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TERMINATE_WITH_CURRENT_EXCEPTION_H_
#define MIR_TERMINATE_WITH_CURRENT_EXCEPTION_H_

namespace mir
{
void terminate_with_current_exception();

/// called by main thread to rethrow any termination exception
void check_for_termination_exception();

void clear_termination_exception();
}

#endif /* MIR_TERMINATE_WITH_CURRENT_EXCEPTION_H_ */
