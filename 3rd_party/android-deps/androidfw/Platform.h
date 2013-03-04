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


#ifndef MIR_ANDROID_PLATFORM_H_
#define MIR_ANDROID_PLATFORM_H_

#if !defined(ANDROID_USE_STD)
    // use the android headers
    #ifndef ANDROIDFW_UTILS
        #define ANDROIDFW_UTILS(name) <utils/name>
    #endif
    #ifndef ANDROIDFW_CUTILS
        #define ANDROIDFW_CUTILS(name) <cutils/name>
    #endif
#else
    // use the standard library
    #ifndef ANDROIDFW_UTILS
        #define ANDROIDFW_UTILS(name) <std/name>
    #endif
    #ifndef ANDROIDFW_CUTILS
        #define ANDROIDFW_CUTILS(name) <std/name>
    #endif
#endif

#endif /* MIR_ANDROID_PLATFORM_H_ */
