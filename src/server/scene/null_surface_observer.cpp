/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/scene/null_surface_observer.h"

namespace ms = mir::scene;
namespace mg = mir::graphics;

void ms::NullSurfaceObserver::attrib_changed(Surface const*, MirWindowAttrib, int) {}
void ms::NullSurfaceObserver::window_resized_to(Surface const*, geometry::Size const&) {}
void ms::NullSurfaceObserver::content_resized_to(Surface const*, geometry::Size const&) {}
void ms::NullSurfaceObserver::moved_to(Surface const*, geometry::Point const&) {}
void ms::NullSurfaceObserver::hidden_set_to(Surface const*, bool) {}
void ms::NullSurfaceObserver::frame_posted(Surface const*, int, geometry::Rectangle const&) {}
void ms::NullSurfaceObserver::alpha_set_to(Surface const*, float) {}
void ms::NullSurfaceObserver::orientation_set_to(Surface const*, MirOrientation) {}
void ms::NullSurfaceObserver::transformation_set_to(Surface const*, glm::mat4 const&) {}
void ms::NullSurfaceObserver::reception_mode_set_to(Surface const*, input::InputReceptionMode) {}
void ms::NullSurfaceObserver::cursor_image_set_to(Surface const*, std::weak_ptr<mir::graphics::CursorImage> const&) {}
void ms::NullSurfaceObserver::client_surface_close_requested(Surface const*) {}
void ms::NullSurfaceObserver::renamed(Surface const*, std::string const&) {}
void ms::NullSurfaceObserver::cursor_image_removed(Surface const*) {}
void ms::NullSurfaceObserver::placed_relative(Surface const*, geometry::Rectangle const&) {}
void ms::NullSurfaceObserver::input_consumed(Surface const*, std::shared_ptr<MirEvent const> const&) {}
void ms::NullSurfaceObserver::depth_layer_set_to(Surface const*, MirDepthLayer) {}
void ms::NullSurfaceObserver::application_id_set_to(Surface const*, std::string const&) {}
void ms::NullSurfaceObserver::entered_output(Surface const*, graphics::DisplayConfigurationOutputId const&) {};
void ms::NullSurfaceObserver::left_output(Surface const*, graphics::DisplayConfigurationOutputId const&) {};
