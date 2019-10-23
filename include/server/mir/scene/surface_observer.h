/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SCENE_SURFACE_OBSERVER_H_
#define MIR_SCENE_SURFACE_OBSERVER_H_

#include "mir_toolkit/common.h"
#include "mir_toolkit/events/event.h"

#include "mir/input/input_reception_mode.h"
#include "mir/geometry/rectangle.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace mir
{
namespace geometry
{
struct Size;
struct Point;
}
namespace graphics
{
class CursorImage;
}

namespace scene
{
class Surface;

class SurfaceObserver
{
public:
    virtual void attrib_changed(Surface const* surf, MirWindowAttrib attrib, int value) = 0;
    virtual void window_resized_to(Surface const* surf, geometry::Size const& window_size) = 0;
    virtual void content_resized_to(Surface const* surf, geometry::Size const& content_size) = 0;
    virtual void moved_to(Surface const* surf, geometry::Point const& top_left) = 0;
    virtual void hidden_set_to(Surface const* surf, bool hide) = 0;
    virtual void frame_posted(Surface const* surf, int frames_available, geometry::Size const& size) = 0;
    virtual void alpha_set_to(Surface const* surf, float alpha) = 0;
    virtual void orientation_set_to(Surface const* surf, MirOrientation orientation) = 0;
    virtual void transformation_set_to(Surface const* surf, glm::mat4 const& t) = 0;
    virtual void reception_mode_set_to(Surface const* surf, input::InputReceptionMode mode) = 0;
    virtual void cursor_image_set_to(Surface const* surf, graphics::CursorImage const& image) = 0;
    virtual void client_surface_close_requested(Surface const* surf) = 0;
    virtual void keymap_changed(
        Surface const* surf,
        MirInputDeviceId id,
        std::string const& model,
        std::string const& layout,
        std::string const& variant,
        std::string const& options) = 0;
    virtual void renamed(Surface const* surf, char const* name) = 0;
    virtual void cursor_image_removed(Surface const* surf) = 0;
    virtual void placed_relative(Surface const* surf, geometry::Rectangle const& placement) = 0;
    virtual void input_consumed(Surface const* surf, MirEvent const* event) = 0;
    virtual void start_drag_and_drop(Surface const* surf, std::vector<uint8_t> const& handle) = 0;
    virtual void depth_layer_set_to(Surface const* surf, MirDepthLayer depth_layer) = 0;
    virtual void application_id_set_to(Surface const* surf, std::string const& application_id) = 0;

protected:
    SurfaceObserver() = default;
    virtual ~SurfaceObserver() = default;
    SurfaceObserver(SurfaceObserver const&) = delete;
    SurfaceObserver& operator=(SurfaceObserver const&) = delete;
};
}
}

#endif // MIR_SCENE_SURFACE_OBSERVER_H_
