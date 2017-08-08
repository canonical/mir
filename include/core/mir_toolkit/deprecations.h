/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_DEPRECATIONS_H_
#define MIR_DEPRECATIONS_H_

#ifndef MIR_ENABLE_DEPRECATIONS
    // use __GNUC__ < 6 as a proxy for building on Ubunutu 16.04LTS ("Xenial")
    #if defined(__clang__) || !defined(__GNUC__) || (__GNUC__ >= 6)
        #define MIR_ENABLE_DEPRECATIONS 1
    #else
        #define MIR_ENABLE_DEPRECATIONS 0
    #endif
#endif

#if MIR_ENABLE_DEPRECATIONS > 0
    #define MIR_FOR_REMOVAL_IN_VERSION_1(message)\
    __attribute__((deprecated(message)))
#else
    #define MIR_FOR_REMOVAL_IN_VERSION_1(message)
#endif

#endif //MIR_DEPRECATIONS_H_
