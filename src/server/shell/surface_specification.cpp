/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com
 */

#include "mir/shell/surface_specification.h"

namespace msh = mir::shell;

bool msh::SurfaceSpecification::is_empty() const
{
    // You know, compile-time reflection would be pretty
    // useful here :)
    return !width.is_set() &&
        !height.is_set() &&
        !pixel_format.is_set() &&
        !buffer_usage.is_set() &&
        !name.is_set() &&
        !output_id.is_set() &&
        !type.is_set() &&
        !state.is_set() &&
        !preferred_orientation.is_set() &&
        !parent_id.is_set() &&
        !aux_rect.is_set() &&
        !edge_attachment.is_set() &&
        !min_width.is_set() &&
        !max_width.is_set() &&
        !max_height.is_set() &&
        !width_inc.is_set() &&
        !height_inc.is_set() &&
        !min_aspect.is_set() &&
        !max_aspect.is_set() &&
        !streams.is_set() &&
        !parent.is_set() &&
        !input_shape.is_set();
}
