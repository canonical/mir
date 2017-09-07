/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include "coordinate_translator.h"

#include <mir/scene/surface.h>
#include <mir/geometry/displacement.h>

#include <boost/exception/all.hpp>

#include <stdexcept>

void miral::CoordinateTranslator::enable_debug_api()
{
    enabled = true;
}
void miral::CoordinateTranslator::disable_debug_api()
{
    enabled = false;
}

auto miral::CoordinateTranslator::surface_to_screen(std::shared_ptr<mir::frontend::Surface> surface, int32_t x, int32_t y)
-> mir::geometry::Point
{
    // On older versions of Mir this disconnects the client instead of just failing the operation. (LP:1641166)
    if (!enabled)
        BOOST_THROW_EXCEPTION(std::runtime_error{"Unsupported feature requested"});

    auto const& scene_surface = std::dynamic_pointer_cast<mir::scene::Surface>(surface);
    return scene_surface->top_left() + mir::geometry::Displacement{x, y};
}

bool miral::CoordinateTranslator::translation_supported() const
{
    return enabled;
}
