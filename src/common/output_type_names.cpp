/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/output_type_names.h"

namespace mir {

// Three enums use this :(  TODO: Deduplicate them and add type safety later.
char const* output_type_name(unsigned output_type)
{
    static const char * const name[] =
    {
        "unknown",
        "VGA",
        "DVI-I",
        "DVI-D",
        "DVI-A",
        "Composite",
        "S-video",
        "LVDS",
        "Component",
        "9-pin",
        "DisplayPort",
        "HDMI-A",
        "HDMI-B",
        "TV",
        "eDP",
        "Virtual",
        "DSI",
        "DPI",
    };
    if (output_type < sizeof(name)/sizeof(name[0]))
        return name[output_type];
    else
        return nullptr;
}

}
