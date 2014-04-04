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
#include "mir/shell/surface_buffer_access.h"
#include "mir/frontend/surface.h"

#include <vector>

namespace mir
{
namespace input { class InputChannel; }
namespace shell { class InputTargeter; }
namespace geometry { class Rectangle; }

namespace scene
{
class SurfaceObserver;

class Surface :
    public graphics::Renderable,
    public input::Surface,
    public frontend::Surface,
    public shell::SurfaceBufferAccess
{
public:
    // resolve ambiguous member function names
    std::string name() const override = 0;
    geometry::Size size() const override = 0;
    geometry::Point top_left() const override = 0;
    float alpha() const override = 0;

    // member functions that don't exist in base classes
    virtual MirSurfaceType type() const = 0;
    virtual MirSurfaceState state() const = 0;
    virtual void hide() = 0;
    virtual void show() = 0;
    virtual void move_to(geometry::Point const& top_left) = 0;
    virtual void take_input_focus(std::shared_ptr<shell::InputTargeter> const& targeter) = 0;
    virtual void set_input_region(std::vector<geometry::Rectangle> const& region) = 0;
    virtual void allow_framedropping(bool) = 0;
    virtual void resize(geometry::Size const& size) = 0;
    virtual void set_transformation(glm::mat4 const& t) = 0;
    virtual void set_alpha(float alpha) = 0;
    virtual void force_requests_to_complete() = 0;

    virtual void add_observer(std::shared_ptr<SurfaceObserver> const& observer) = 0;
    virtual void remove_observer(std::shared_ptr<SurfaceObserver> const& observer) = 0;

    // TODO input_channel() relates to adding and removing the surface
    // TODO from the scene and is probably not cleanest interface for this.
    virtual std::shared_ptr<input::InputChannel> input_channel() const = 0;
};
}
}

#endif // MIR_SCENE_SURFACE_H_
