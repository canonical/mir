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
    std::string name() const override;
    void move_to(geometry::Point const& top_left) override;
    float alpha() const override;
    geometry::Size size() const override;
    geometry::Size client_size() const override;
    std::shared_ptr<frontend::BufferStream> primary_buffer_stream() const override;
    void set_streams(std::list<scene::StreamInfo> const& streams) override;
    bool supports_input() const override;
    int client_input_fd() const override;
    std::shared_ptr<input::InputChannel> input_channel() const override;
    input::InputReceptionMode reception_mode() const override;
    void set_reception_mode(input::InputReceptionMode mode) override;
    void set_input_region(std::vector<geometry::Rectangle> const& input_rectangles) override;
    void resize(geometry::Size const& size) override;
    geometry::Point top_left() const override;
    geometry::Rectangle input_bounds() const override;
    bool input_area_contains(geometry::Point const& point) const override;
    void consume(MirEvent const* event) override;
    void set_alpha(float alpha) override;
    void set_orientation(MirOrientation orientation) override;
    void set_transformation(glm::mat4 const&) override;
    bool visible() const override;
    graphics::RenderableList generate_renderables(compositor::CompositorID id) const override;
    int buffers_ready_for_compositor(void const* compositor_id) const override;
    MirSurfaceType type() const override;
    MirSurfaceState state() const override;
    int configure(MirSurfaceAttrib attrib, int value) override;
    int query(MirSurfaceAttrib attrib) const override;
    void hide() override;
    void show() override;
    void set_cursor_image(std::shared_ptr<graphics::CursorImage> const& image) override;
    std::shared_ptr<graphics::CursorImage> cursor_image() const override;
    void set_cursor_stream(std::shared_ptr<frontend::BufferStream> const& stream, geometry::Displacement const& hotspot) override;
    void request_client_surface_close() override;
    std::shared_ptr<Surface> parent() const override;
    void add_observer(std::shared_ptr<scene::SurfaceObserver> const& observer) override;
    void remove_observer(std::weak_ptr<scene::SurfaceObserver> const& observer) override;
    void set_keymap(MirInputDeviceId id, std::string const& model, std::string const& layout,
                    std::string const& variant, std::string const& options) override;
    void rename(std::string const& title) override;
};
}
}
}

#endif //MIR_TEST_DOUBLES_STUB_SURFACE_H
