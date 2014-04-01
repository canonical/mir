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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/scene/surface_observer.h"

namespace ms = mir::scene;

void ms::SurfaceObserver::attrib_change(MirSurfaceAttrib /*attrib*/, int /*value*/) {}
void ms::SurfaceObserver::resize(geometry::Size const& /*size*/) {}
void ms::SurfaceObserver::move_to(geometry::Point const& /*top_left*/) {}
void ms::SurfaceObserver::set_hidden(bool /*hide*/) {}
void ms::SurfaceObserver::frame_posted() {}
void ms::SurfaceObserver::set_alpha(float /*alpha*/) {}
void ms::SurfaceObserver::set_transformation(glm::mat4 const& /*t*/) {}
