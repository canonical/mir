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
 * Authored By: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "raise_surface_policy.h"
#include "mir/cookie_factory.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"

namespace ms = mir::scene;
namespace msh = mir::shell;

bool msh::RaiseSurfacePolicy::should_raise_surface(
    std::shared_ptr<ms::Surface> const& focused_surface,
    uint64_t timestamp) const
{
    return timestamp >= focused_surface->last_input_event_timestamp();
}
