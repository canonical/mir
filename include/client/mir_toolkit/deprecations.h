/*
 * Copyright © 2017 Canonical Ltd.
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

#ifndef MIR_DEPRECATIONS_H_
#define MIR_DEPRECATIONS_H_

#ifdef MIR_ENABLE_DEPRECATIONS
    #define MIR_FOR_REMOVAL_IN_VERSION_1(message)\
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirWindow instead")
#else
#define MIR_FOR_REMOVAL_IN_VERSION_1(message)
#endif

#endif //MIR_DEPRECATIONS_H_
