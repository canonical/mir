/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include <mir/test/doubles/stub_surface.h>

namespace mtd = mir::test::doubles;

std::string mtd::StubSurface::name() const
{
    return {};
}

void mtd::StubSurface::move_to(mir::geometry::Point const& /*top_left*/)
{
}

mir::geometry::Size mtd::StubSurface::window_size() const
{
    return {};
}

mir::geometry::Size mtd::StubSurface::content_size() const
{
    return {};
}

std::shared_ptr <mir::frontend::BufferStream> mtd::StubSurface::primary_buffer_stream() const
{
    return {};
}

void mtd::StubSurface::set_streams(std::list<mir::scene::StreamInfo> const& /*streams*/)
{
}

mir::input::InputReceptionMode mtd::StubSurface::reception_mode() const
{
    return mir::input::InputReceptionMode::normal;
}

void mtd::StubSurface::set_reception_mode(mir::input::InputReceptionMode /*mode*/)
{
}

void mtd::StubSurface::set_input_region(std::vector<mir::geometry::Rectangle> const& /*input_rectangles*/)
{
}

void mtd::StubSurface::resize(mir::geometry::Size const& /*size*/)
{
}

mir::geometry::Point mtd::StubSurface::top_left() const
{
    return {};
}

mir::geometry::Rectangle mtd::StubSurface::input_bounds() const
{
    return {};
}

bool mtd::StubSurface::input_area_contains(mir::geometry::Point const& /*point*/) const
{
    return false;
}

void mtd::StubSurface::consume(MirEvent const* /*event*/)
{
}

void mtd::StubSurface::set_alpha(float /*alpha*/)
{
}

void mtd::StubSurface::set_orientation(MirOrientation /*orientation*/)
{
}

void mtd::StubSurface::set_transformation(glm::mat4 const& /*mat4*/)
{
}

bool mtd::StubSurface::visible() const
{
    return false;
}

mir::graphics::RenderableList mtd::StubSurface::generate_renderables(mir::compositor::CompositorID /*id*/) const
{
    return {};
}

int mtd::StubSurface::buffers_ready_for_compositor(void const* /*compositor_id*/) const
{
    return 0;
}

MirWindowType mtd::StubSurface::type() const
{
    return MirWindowType::mir_window_type_normal;
}

MirWindowState mtd::StubSurface::state() const
{
    return MirWindowState::mir_window_state_fullscreen;
}

int mtd::StubSurface::configure(MirWindowAttrib /*attrib*/, int value)
{
    return value;
}

int mtd::StubSurface::query(MirWindowAttrib /*attrib*/) const
{
    return 0;
}

void mtd::StubSurface::hide()
{
}

void mtd::StubSurface::show()
{
}

void mtd::StubSurface::set_cursor_image(std::shared_ptr<mir::graphics::CursorImage> const& /*image*/)
{
}

std::shared_ptr<mir::graphics::CursorImage> mtd::StubSurface::cursor_image() const
{
    return {};
}

void mtd::StubSurface::set_cursor_stream(
    std::shared_ptr<mir::frontend::BufferStream> const& /*stream*/,
    mir::geometry::Displacement const& /*hotspot*/)
{
}

void mtd::StubSurface::request_client_surface_close()
{
}

std::shared_ptr<mir::scene::Surface> mtd::StubSurface::parent() const
{
    return {};
}

void mtd::StubSurface::add_observer(std::shared_ptr<mir::scene::SurfaceObserver> const& /*observer*/)
{
}

void mtd::StubSurface::remove_observer(std::weak_ptr < mir::scene::SurfaceObserver > const& /*observer*/)
{
}

void mtd::StubSurface::set_keymap(MirInputDeviceId /*id*/, std::string const& /*model*/, std::string const& /*layout*/,
                                  std::string const& /*variant*/, std::string const& /*options*/)
{
}

void mtd::StubSurface::rename(std::string const& /*title*/)
{
}

void mtd::StubSurface::set_confine_pointer_state(MirPointerConfinementState /*state*/)
{
}

MirPointerConfinementState mtd::StubSurface::confine_pointer_state() const
{
    return {};
}

void mtd::StubSurface::placed_relative(geometry::Rectangle const& /*placement*/)
{
}

void mtd::StubSurface::start_drag_and_drop(std::vector<uint8_t> const& /*handle*/)
{
}

auto mtd::StubSurface::depth_layer() const -> MirDepthLayer
{
    return mir_depth_layer_application;
}

void mtd::StubSurface::set_depth_layer(MirDepthLayer /*depth_layer*/)
{
}

std::experimental::optional<mir::geometry::Rectangle> mtd::StubSurface::clip_area() const
{
    return {};
}
void mtd::StubSurface::set_clip_area(std::experimental::optional<mir::geometry::Rectangle> const& /*area*/)
{
}

MirWindowFocusState mtd::StubSurface::focus_state() const
{
    return mir_window_focus_state_unfocused;
}

void mtd::StubSurface::set_focus_state(MirWindowFocusState /*new_state*/)
{
}

std::string mtd::StubSurface::application_id() const
{
    return "";
}

void mtd::StubSurface::set_application_id(std::string const& /*application_id*/)
{
}

std::weak_ptr<mir::scene::Session> mtd::StubSurface::session() const
{
    return {};
}

namespace
{
// Ensure we don't accidentally have an abstract class
mtd::StubSurface instantiation_test;
}
