/*
 * Copyright (C) Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MIROIL_SURFACE_H
#define MIROIL_SURFACE_H
#include <memory>
#include <unordered_map>
#include <mir_toolkit/mir_input_device_types.h>
#include <mir_toolkit/common.h>
#include <mir/graphics/renderable.h>

namespace mir { 
    namespace scene { class Surface; } 
    namespace graphics { class CursorImage; }
    namespace compositor { class BufferStream; }
}

namespace miroil {
    
class SurfaceObserver;
class SurfaceObserverImpl;

using CompositorID = void const*;
    
class Surface
{
public:
    Surface(std::shared_ptr<mir::scene::Surface> wrapped);
    ~Surface() = default;
    
    mir::scene::Surface *get_wrapped() const;
    void add_observer(std::shared_ptr<miroil::SurfaceObserver> const& observer);    
    void remove_observer(std::shared_ptr<miroil::SurfaceObserver> const& observer);
    
    mir::graphics::RenderableList generate_renderables(miroil::CompositorID id) const; 

    
    bool is_confined_to_window();
    void set_orientation(MirOrientation orientation);

    void set_confine_pointer_state(MirPointerConfinementState state);                        
    std::shared_ptr<mir::scene::Surface> parent() const;
    /// Top-left corner (of the window frame if present)
    mir::geometry::Point top_left() const;
    bool visible() const;

    // TODO a legacy of old interactions and needs removing
    int configure(MirWindowAttrib attrib, int value);
    // TODO a legacy of old interactions and needs removing
    int query(MirWindowAttrib attrib) const;
    // TODO a legacy of old interactions and needs removing
    void set_keymap(MirInputDeviceId id, std::string const& model, std::string const& layout,
                            std::string const& variant, std::string const& options);

private:
    std::shared_ptr<mir::scene::Surface> wrapped;
    std::unordered_map<std::shared_ptr<miroil::SurfaceObserver>, std::shared_ptr<miroil::SurfaceObserverImpl>> observers;
};

}

#endif
