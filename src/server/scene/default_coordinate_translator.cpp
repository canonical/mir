/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "default_coordinate_translator.h"
#include "mir/scene/surface.h"
#include "mir/geometry/displacement.h"

namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace ms = mir::scene;

geom::Point ms::DefaultCoordinateTranslator::surface_to_screen(std::shared_ptr<mf::Surface> surface, int32_t x,
                                                               int32_t y)
{
    auto const scene_surface = std::dynamic_pointer_cast<ms::Surface>(surface);

    return scene_surface->top_left() + geom::Displacement{x, y};
}
