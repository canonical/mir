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

#ifndef MIR_SCENE_SURFACE_H_
#define MIR_SCENE_SURFACE_H_

#include "mir/graphics/renderable.h"
#include "mir/input/surface.h"
#include "mir/scene/surface_buffer_access.h"
#include "mir/frontend/surface.h"

#include <vector>

namespace mir
{
namespace input { class InputChannel; }
namespace shell { class InputTargeter; }
namespace geometry { struct Rectangle; }
namespace graphics { class CursorImage; }

namespace scene
{
class SurfaceObserver;

class Surface :
    public input::Surface,
    public frontend::Surface,
    public SurfaceBufferAccess
{
public:
    // resolve ambiguous member function names

    std::string name() const override = 0;
    geometry::Size client_size() const override = 0;
    geometry::Rectangle input_bounds() const override = 0;

    // member functions that don't exist in base classes

    /// Top-left corner (of the window frame if present)
    virtual geometry::Point top_left() const = 0;
    /// Size of the surface including window frame (if any)
    virtual geometry::Size size() const = 0;

    virtual std::unique_ptr<graphics::Renderable> compositor_snapshot(void const* compositor_id) const = 0;

    virtual float alpha() const = 0; //only used in examples/
    virtual MirSurfaceType type() const = 0;
    virtual MirSurfaceState state() const = 0;
    virtual void hide() = 0;
    virtual void show() = 0;
    virtual bool visible() const = 0;
    virtual void move_to(geometry::Point const& top_left) = 0;
    virtual void take_input_focus(std::shared_ptr<shell::InputTargeter> const& targeter) = 0;

    /**
     * Sets the input region for this surface.
     *
     * The input region is expressed in coordinates relative to the surface (i.e.,
     * use (0,0) for the top left point of the surface).
     *
     * By default the input region is the whole surface. To unset a custom input region
     * and revert to the default set an empty input region, i.e., set_input_region({}).
     * To disable input set a non-empty region containing an empty rectangle, i.e.,
     * set_input_region({geom::Rectangle{}}).
     */
    virtual void set_input_region(std::vector<geometry::Rectangle> const& region) = 0;
    virtual void allow_framedropping(bool) = 0;
    virtual void resize(geometry::Size const& size) = 0;
    virtual void set_transformation(glm::mat4 const& t) = 0;
    virtual void set_alpha(float alpha) = 0;
    virtual void set_orientation(MirOrientation orientation) = 0;
    virtual void force_requests_to_complete() = 0;
    
    virtual void set_cursor_image(std::shared_ptr<graphics::CursorImage> const& image) override = 0;
    virtual std::shared_ptr<graphics::CursorImage> cursor_image() const override = 0;

    virtual void add_observer(std::shared_ptr<SurfaceObserver> const& observer) = 0;
    virtual void remove_observer(std::weak_ptr<SurfaceObserver> const& observer) = 0;

    // TODO input_channel() relates to adding and removing the surface
    // TODO from the scene and is probably not cleanest interface for this.
    virtual std::shared_ptr<input::InputChannel> input_channel() const override = 0;
    virtual void set_reception_mode(input::InputReceptionMode mode) = 0;

    virtual void request_client_surface_close() = 0;
};
}
}

#endif // MIR_SCENE_SURFACE_H_
