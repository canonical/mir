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

#ifndef MIR_SCENE_SURFACE_H_
#define MIR_SCENE_SURFACE_H_

#include "mir/graphics/renderable.h"
#include "mir/input/surface.h"
#include "mir/frontend/surface.h"
#include "mir/compositor/compositor_id.h"
#include "mir/optional_value.h"

#include <vector>
#include <list>

namespace mir
{
namespace shell { class InputTargeter; }
namespace geometry { struct Rectangle; }
namespace graphics { class CursorImage; }
namespace compositor { class BufferStream; }
namespace scene
{
struct StreamInfo
{
    std::shared_ptr<compositor::BufferStream> stream;
    geometry::Displacement displacement;
    optional_value<geometry::Size> size;
};

class SurfaceObserver;

class Surface :
    public input::Surface,
    public frontend::Surface
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

    virtual graphics::RenderableList generate_renderables(compositor::CompositorID id) const = 0; 
    virtual int buffers_ready_for_compositor(void const* compositor_id) const = 0;

    virtual MirWindowType type() const = 0;
    virtual MirWindowState state() const = 0;
    virtual void hide() = 0;
    virtual void show() = 0;
    virtual bool visible() const = 0;
    virtual void move_to(geometry::Point const& top_left) = 0;

    /**
     * Sets the input region for this surface.
     *
     * The input region is expressed in coordinates relative to the surface (i.e.,
     * use (0,0) for the top left point of the surface).
     *
     * By default the input region is the whole surface. To unset a custom input region
     * and revert to the default set an empty input region, i.e., set_input_region({}).
     * To disable input set a non-empty region containing an empty rectangle, i.e.,
     * set_input_region({Rectangle{}}).
     */
    virtual void set_input_region(std::vector<geometry::Rectangle> const& region) = 0;
    virtual void resize(geometry::Size const& size) = 0;
    virtual void set_transformation(glm::mat4 const& t) = 0;
    virtual void set_alpha(float alpha) = 0;
    virtual void set_orientation(MirOrientation orientation) = 0;
    
    virtual void set_cursor_image(std::shared_ptr<graphics::CursorImage> const& image) override = 0;
    virtual std::shared_ptr<graphics::CursorImage> cursor_image() const override = 0;

    virtual void add_observer(std::shared_ptr<SurfaceObserver> const& observer) = 0;
    virtual void remove_observer(std::weak_ptr<SurfaceObserver> const& observer) = 0;

    virtual void set_reception_mode(input::InputReceptionMode mode) = 0;

    virtual void request_client_surface_close() = 0;
    virtual std::shared_ptr<Surface> parent() const = 0;

    // TODO a legacy of old interactions and needs removing
    virtual int configure(MirWindowAttrib attrib, int value) = 0;
    // TODO a legacy of old interactions and needs removing
    virtual int query(MirWindowAttrib attrib) const = 0;

    virtual void set_keymap(MirInputDeviceId id, std::string const& model, std::string const& layout,
                            std::string const& variant, std::string const& options) = 0;
    virtual void rename(std::string const& title) = 0;
    virtual void set_streams(std::list<StreamInfo> const& streams) = 0;

    virtual void set_confine_pointer_state(MirPointerConfinementState state) = 0;
    virtual MirPointerConfinementState confine_pointer_state() const = 0;

    virtual void placed_relative(geometry::Rectangle const& placement) = 0;
    virtual void start_drag_and_drop(std::vector<uint8_t> const& handle) = 0;

    /// The depth layer the surface is on
    /// It will be kept above all surfaces on lower layers, and below surfaces on higher layers
    virtual auto depth_layer() const -> MirDepthLayer = 0;
    /// When the depth layer is changed, the surface becomes the top surface on that layer
    virtual void set_depth_layer(MirDepthLayer depth_layer) = 0;

    /// If the window has focus
    virtual auto focus_state() const -> MirWindowFocusState = 0;
    virtual void set_focus_state(MirWindowFocusState focus_state) = 0;

    /// Often the same as the session name, but on Wayland can be set on a per-window basis
    /// See xdg_toplevel.set_app_id and http://standards.freedesktop.org/desktop-entry-spec/ for more details
    /// Empty string if not set
    virtual auto application_id() const -> std::string = 0;
    virtual void set_application_id(std::string const& application_id) = 0;
};
}
}

#endif // MIR_SCENE_SURFACE_H_
