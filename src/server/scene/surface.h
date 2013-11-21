/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_SCENE_SURFACE_H_
#define MIR_SCENE_SURFACE_H_

#include "basic_surface.h"

namespace mir
{
namespace scene
{
class SurfaceState;
class SceneReport;

class Surface : public BasicSurface
{
public:
    Surface(std::shared_ptr<SurfaceState> const& surface_state,
            std::shared_ptr<compositor::BufferStream> const& buffer_stream,
            std::shared_ptr<input::InputChannel> const& input_channel,
            std::shared_ptr<SceneReport> const& report);

    ~Surface();

    std::string const& name() const;
    void move_to(geometry::Point const& top_left);
    void set_rotation(float degrees, glm::vec3 const& axis);
    void set_alpha(float alpha);
    void set_hidden(bool is_hidden);

    geometry::Point top_left() const;
    geometry::Size size() const;

    geometry::PixelFormat pixel_format() const;

    std::shared_ptr<graphics::Buffer> snapshot_buffer() const;
    std::shared_ptr<graphics::Buffer> advance_client_buffer();
    void force_requests_to_complete();
    void flag_for_render();

    bool supports_input() const;
    int client_input_fd() const;
    void allow_framedropping(bool);
    std::shared_ptr<input::InputChannel> input_channel() const;

    void set_input_region(std::vector<geometry::Rectangle> const& input_rectangles);

    std::shared_ptr<compositor::CompositingCriteria> compositing_criteria();

    std::shared_ptr<compositor::BufferStream> buffer_stream() const;

    std::shared_ptr<input::Surface> input_surface() const;

    void resize(geometry::Size const& size);

private:
    std::shared_ptr<SurfaceState> surface_state;
    std::shared_ptr<compositor::BufferStream> surface_buffer_stream;
    std::shared_ptr<input::InputChannel> const server_input_channel;
    std::shared_ptr<SceneReport> const report;
    bool surface_in_startup;
};
}
}
#endif /* MIR_SCENE_SURFACE_H_ */
