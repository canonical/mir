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

#include "mir/scene/null_surface_observer.h"

namespace ms = mir::scene;
namespace mg = mir::graphics;

void ms::NullSurfaceObserver::attrib_changed(MirSurfaceAttrib /*attrib*/, int /*value*/) {}
void ms::NullSurfaceObserver::resized_to(geometry::Size const& /*size*/) {}
void ms::NullSurfaceObserver::moved_to(geometry::Point const& /*top_left*/) {}
void ms::NullSurfaceObserver::hidden_set_to(bool /*hide*/) {}
void ms::NullSurfaceObserver::frame_posted(int /*frames_available*/) {}
void ms::NullSurfaceObserver::alpha_set_to(float /*alpha*/) {}
void ms::NullSurfaceObserver::orientation_set_to(MirOrientation /*orientation*/) {}
void ms::NullSurfaceObserver::transformation_set_to(glm::mat4 const& /*t*/) {}
void ms::NullSurfaceObserver::reception_mode_set_to(input::InputReceptionMode /*mode*/) {}
void ms::NullSurfaceObserver::cursor_image_set_to(mg::CursorImage const& /*image*/) {}
void ms::NullSurfaceObserver::client_surface_close_requested() {}
void ms::NullSurfaceObserver::keymap_changed(xkb_rule_names const& /* names */) {}
void ms::NullSurfaceObserver::renamed(char const*) {}
