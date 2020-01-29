/*
 * Copyright © 2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
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

#ifndef MIRAL_DEPRECATIONS_H_
#define MIRAL_DEPRECATIONS_H_

#ifndef MIRAL_ENABLE_DEPRECATIONS
    #if defined(__clang__) || defined(__GNUC__)
        #define MIRAL_ENABLE_DEPRECATIONS 1
    #else
        #define MIRAL_ENABLE_DEPRECATIONS 0
    #endif
#endif

#if MIRAL_ENABLE_DEPRECATIONS > 0
    #define MIRAL_FOR_REMOVAL_IN_VERSION_3(message)\
    __attribute__((deprecated(message)))
#else
    #define MIRAL_FOR_REMOVAL_IN_VERSION_3(message)
#endif

#endif //MIRAL_DEPRECATIONS_H_
