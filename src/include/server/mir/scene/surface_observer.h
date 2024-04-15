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

#ifndef MIR_SCENE_SURFACE_OBSERVER_H_
#define MIR_SCENE_SURFACE_OBSERVER_H_

#include "mir_toolkit/common.h"
#include "mir_toolkit/events/event.h"

#include "mir/input/input_reception_mode.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/display_configuration.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

namespace mir
{
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
    /// damage is given in surface-local logical coordinates
    virtual void frame_posted(Surface const* surf, geometry::Rectangle const& damage) = 0;
    virtual void alpha_set_to(Surface const* surf, float alpha) = 0;
    virtual void orientation_set_to(Surface const* surf, MirOrientation orientation) = 0;
    virtual void transformation_set_to(Surface const* surf, glm::mat4 const& t) = 0;
    virtual void reception_mode_set_to(Surface const* surf, input::InputReceptionMode mode) = 0;
    virtual void cursor_image_set_to(Surface const* surf, std::weak_ptr<mir::graphics::CursorImage> const& image) = 0;
    virtual void client_surface_close_requested(Surface const* surf) = 0;
    virtual void renamed(Surface const* surf, std::string const& name) = 0;
    virtual void cursor_image_removed(Surface const* surf) = 0;
    virtual void placed_relative(Surface const* surf, geometry::Rectangle const& placement) = 0;
    virtual void input_consumed(Surface const* surf, std::shared_ptr<MirEvent const> const& event) = 0;
    virtual void application_id_set_to(Surface const* surf, std::string const& application_id) = 0;
    virtual void depth_layer_set_to(Surface const* surf, MirDepthLayer depth_layer) = 0;
    virtual void entered_output(Surface const* surf, graphics::DisplayConfigurationOutputId const& id) = 0;
    virtual void left_output(Surface const* surf, graphics::DisplayConfigurationOutputId const& id) = 0;

protected:
    SurfaceObserver() = default;
    virtual ~SurfaceObserver() = default;
    SurfaceObserver(SurfaceObserver const&) = delete;
    SurfaceObserver& operator=(SurfaceObserver const&) = delete;
};
}
}

#endif // MIR_SCENE_SURFACE_OBSERVER_H_
