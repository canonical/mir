/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_EVENT_H_
#define MIR_EVENT_H_

#include "mir_toolkit/common.h"

namespace mir
{

// Hmm, canonical in structure but also hard to read. You can't win.
struct Event
{
    typedef enum
    {
        UNKNOWN,
        SURFACE
    } Type;
    Type type;

    struct Surface
    {
        typedef enum
        {
            CHANGE
        } Type;

        int id;
        Type type;

        union
        {
            struct
            {
                MirSurfaceAttrib attrib;
                int value;
            } change;
        }; // union reserved for future expansion
    };

    union
    {
        Surface surface;
    };
};

} // namespace mir

#endif

