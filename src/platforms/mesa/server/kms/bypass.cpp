/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/graphics/renderable.h"
#include "mir/graphics/display_buffer.h"
#include "bypass.h"

using namespace mir;
namespace mgm = mir::graphics::mesa;

mgm::BypassMatch::BypassMatch(geometry::Rectangle const& rect)
    : view_area(rect),
      bypass_is_feasible(true),
      identity()
{
}

bool mgm::BypassMatch::operator()(std::shared_ptr<graphics::Renderable> const& renderable)
{
    //we've already eliminated bypass as a possibility
    if (!bypass_is_feasible)
        return false;

    //offscreen surfaces don't affect if bypass is possible
    if (!view_area.overlaps(renderable->screen_position()))
        return false;

    auto const is_opaque = !((renderable->alpha() != 1.0f) || renderable->shaped());
    auto const fits = (renderable->screen_position() == view_area);
    auto const is_orthogonal = (renderable->transformation() == identity);
    bypass_is_feasible = (is_opaque && fits && is_orthogonal);
    return bypass_is_feasible;
}
