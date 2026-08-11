/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_X11_XCB_XKB_COMPAT_H_
#define MIR_X11_XCB_XKB_COMPAT_H_

// xcb/xkb.h has a struct member named "explicit", which C++ does not like.
#if defined(__clang__)
#define MIR_XKB_DIAGNOSTIC_PUSH \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wkeyword-macro\"")
#define MIR_XKB_DIAGNOSTIC_POP _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#define MIR_XKB_DIAGNOSTIC_PUSH \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wkeyword-macro\"")
#define MIR_XKB_DIAGNOSTIC_POP _Pragma("GCC diagnostic pop")
#else
#define MIR_XKB_DIAGNOSTIC_PUSH
#define MIR_XKB_DIAGNOSTIC_POP
#endif

MIR_XKB_DIAGNOSTIC_PUSH
#define explicit explicit_
#include <xcb/xkb.h>
#undef explicit
MIR_XKB_DIAGNOSTIC_POP

#undef MIR_XKB_DIAGNOSTIC_PUSH
#undef MIR_XKB_DIAGNOSTIC_POP

#endif
