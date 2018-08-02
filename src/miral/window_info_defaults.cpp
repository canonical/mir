/*
 * Copyright © 2018 Canonical Ltd.
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

#include "window_info_defaults.h"

namespace
{
unsigned clamp_dim(unsigned dim)
{
    return std::min<unsigned long>(std::numeric_limits<long>::max(), dim);
}

// For our convenience when doing calculations we clamp the dimensions of the aspect ratio
// so they will fit into a long without overflow.
miral::WindowInfo::AspectRatio clamp(miral::WindowInfo::AspectRatio const& source)
{
    return {clamp_dim(source.width), clamp_dim(source.height)};
}
}

miral::Width  const miral::default_min_width{0};
miral::Height const miral::default_min_height{0};
miral::Width  const miral::default_max_width{std::numeric_limits<int>::max()};
miral::Height const miral::default_max_height{std::numeric_limits<int>::max()};
miral::DeltaX const miral::default_width_inc{1};
miral::DeltaY const miral::default_height_inc{1};
miral::WindowInfo::AspectRatio const miral::default_min_aspect_ratio{
    clamp(WindowInfo::AspectRatio{0U, std::numeric_limits<unsigned>::max()})};
miral::WindowInfo::AspectRatio const miral::default_max_aspect_ratio{
    clamp(WindowInfo::AspectRatio{std::numeric_limits<unsigned>::max(), 0U})};
