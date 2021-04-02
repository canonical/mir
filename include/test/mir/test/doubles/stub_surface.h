/*
 * Copyright © 2015-2019 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_STUB_SURFACE_H
#define MIR_TEST_DOUBLES_STUB_SURFACE_H

#include <mir/scene/surface.h>

namespace mir
{
namespace test
{
namespace doubles
{
// scene::Surface is a horribly wide interface to expose from Mir
struct StubSurface : scene::Surface
{
    std::string name() const override { return ""; }
    void move_to(geometry::Point const&) override {}
    geometry::Size window_size() const override { return {}; }
    geometry::Displacement content_offset() const override { return {}; }
    geometry::Size content_size() const override { return {}; }
    std::shared_ptr<frontend::BufferStream> primary_buffer_stream() const override { return nullptr; }
    void set_streams(std::list<scene::StreamInfo> const&) override {}
    input::InputReceptionMode reception_mode() const override { return input::InputReceptionMode::normal; }
    void set_reception_mode(input::InputReceptionMode) override {}
    void set_input_region(std::vector<geometry::Rectangle> const&) override {}
    void resize(geometry::Size const&) override {}
    geometry::Point top_left() const override { return {}; }
    geometry::Rectangle input_bounds() const override { return {}; }
    bool input_area_contains(geometry::Point const&) const override { return false; }
    void consume(MirEvent const*) override {}
    void set_alpha(float) override {}
    void set_orientation(MirOrientation) override {}
    void set_transformation(glm::mat4 const&) override {}
    bool visible() const override { return false; }
    graphics::RenderableList generate_renderables(compositor::CompositorID) const override { return {}; }
    int buffers_ready_for_compositor(void const*) const override { return 0; }
    MirWindowType type() const override { return mir_window_type_normal; }
    MirWindowState state() const override { return mir_window_state_fullscreen; }
    int configure(MirWindowAttrib, int value) override { return value; }
    int query(MirWindowAttrib) const override { return 0; }
    void hide() override {}
    void show() override {}
    void set_cursor_image(std::shared_ptr<graphics::CursorImage> const&) override {}
    std::shared_ptr<graphics::CursorImage> cursor_image() const override { return nullptr; }
    void set_cursor_stream(std::shared_ptr<frontend::BufferStream> const&, geometry::Displacement const&) override {}
    void request_client_surface_close() override {}
    std::shared_ptr<Surface> parent() const override { return nullptr; }
    void add_observer(std::shared_ptr<scene::SurfaceObserver> const&) override {}
    void remove_observer(std::weak_ptr<scene::SurfaceObserver> const&) override {}
    void set_keymap(MirInputDeviceId, std::string const&, std::string const&, std::string const&,
                    std::string const&) override {}
    void rename(std::string const&) override {}
    void set_confine_pointer_state(MirPointerConfinementState) override {}
    MirPointerConfinementState confine_pointer_state() const override { return mir_pointer_unconfined; }
    void placed_relative(geometry::Rectangle const&) override {}
    void start_drag_and_drop(std::vector<uint8_t> const&) override {}
    MirDepthLayer depth_layer() const override { return mir_depth_layer_application; }
    void set_depth_layer(MirDepthLayer) override {}
    std::experimental::optional<geometry::Rectangle> clip_area() const override { return std::experimental::nullopt; }
    void set_clip_area(std::experimental::optional<geometry::Rectangle> const&) override {}
    MirWindowFocusState focus_state() const override { return mir_window_focus_state_unfocused; }
    void set_focus_state(MirWindowFocusState) override {}
    std::string application_id() const override { return ""; }
    void set_application_id(std::string const&) override {}
    std::weak_ptr<scene::Session> session() const override { return {}; }
    void set_window_margins(
        geometry::DeltaY,
        geometry::DeltaX,
        geometry::DeltaY,
        geometry::DeltaX) override {}
};
}
}
}

#endif //MIR_TEST_DOUBLES_STUB_SURFACE_H
