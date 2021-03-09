/*
 * Copyright (C) 2021 Canonical, Ltd.
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
#include <miroil/surface.h>
#include <miroil/surface_observer.h>
#include "mir/scene/surface.h"
#include "mir/scene/surface_observer.h"

namespace miroil {

class SurfaceObserverImpl : public mir::scene::SurfaceObserver
{
public:
    SurfaceObserverImpl(std::shared_ptr<miroil::SurfaceObserver> const & wrapped);
    virtual ~SurfaceObserverImpl();
    
    void alpha_set_to(mir::scene::Surface const* surf, float alpha) override;
    void application_id_set_to(mir::scene::Surface const* surf, std::string const& application_id) override;
    void attrib_changed(mir::scene::Surface const* surf, MirWindowAttrib attrib, int value) override;
    void client_surface_close_requested(mir::scene::Surface const* surf) override;
    void content_resized_to(mir::scene::Surface const* surf, mir::geometry::Size const& content_size) override;
    void cursor_image_removed(mir::scene::Surface const* surf) override;
    void cursor_image_set_to(mir::scene::Surface const* surf, mir::graphics::CursorImage const& image) override;
    void depth_layer_set_to(mir::scene::Surface const* surf, MirDepthLayer depth_layer) override;
    void frame_posted(mir::scene::Surface const* surf, int frames_available, mir::geometry::Size const& size) override;
    void hidden_set_to(mir::scene::Surface const* surf, bool hide) override;
    void input_consumed(mir::scene::Surface const* surf, MirEvent const* event) override;
    void keymap_changed(mir::scene::Surface const* surf, MirInputDeviceId id, std::string const& model,
                                std::string const& layout, std::string const& variant, std::string const& options) override;
    void moved_to(mir::scene::Surface const* surf, mir::geometry::Point const& top_left) override;
    void orientation_set_to(mir::scene::Surface const* surf, MirOrientation orientation) override;
    void placed_relative(mir::scene::Surface const* surf, mir::geometry::Rectangle const& placement) override;
    void reception_mode_set_to(mir::scene::Surface const* /*surf*/, mir::input::InputReceptionMode /*mode*/) override {};
    void renamed(mir::scene::Surface const* surf, char const* name);    
    void start_drag_and_drop(mir::scene::Surface const* surf, std::vector<uint8_t> const& handle) override;
    void transformation_set_to(mir::scene::Surface const* surf, glm::mat4 const& t) override;
    void window_resized_to(mir::scene::Surface const* surf, mir::geometry::Size const& window_size) override;
    
private:
    std::shared_ptr<miroil::SurfaceObserver> listener;
};

SurfaceObserverImpl::SurfaceObserverImpl(std::shared_ptr<miroil::SurfaceObserver> const & wrapped)
: listener(wrapped)
{
}

SurfaceObserverImpl::~SurfaceObserverImpl() = default;

void SurfaceObserverImpl::alpha_set_to(mir::scene::Surface const* surf, float alpha)
{
    listener->alpha_set_to(surf, alpha);
}

void SurfaceObserverImpl::application_id_set_to(mir::scene::Surface const* surf, std::string const& application_id)
{
    listener->application_id_set_to(surf, application_id);
}

void SurfaceObserverImpl::attrib_changed(mir::scene::Surface const* surf, MirWindowAttrib attrib, int value)
{
    listener->attrib_changed(surf, attrib, value);
}

void SurfaceObserverImpl::client_surface_close_requested(mir::scene::Surface const* surf)
{
    listener->client_surface_close_requested(surf);
}

void SurfaceObserverImpl::content_resized_to(mir::scene::Surface const* surf, mir::geometry::Size const& content_size)
{
    listener->content_resized_to(surf, content_size);
}

void SurfaceObserverImpl::cursor_image_removed(mir::scene::Surface const* surf)
{
    listener->cursor_image_removed(surf);
}

void SurfaceObserverImpl::cursor_image_set_to(mir::scene::Surface const* surf, mir::graphics::CursorImage const& image)
{
    listener->cursor_image_set_to(surf, image);
}

void SurfaceObserverImpl::depth_layer_set_to(mir::scene::Surface const* surf, MirDepthLayer depth_layer)
{
    listener->depth_layer_set_to(surf, depth_layer);
}

void SurfaceObserverImpl::frame_posted(mir::scene::Surface const* surf, int frames_available, mir::geometry::Size const& size)
{
    listener->frame_posted(surf, frames_available, size);
}

void SurfaceObserverImpl::hidden_set_to(mir::scene::Surface const* surf, bool hide)
{
    listener->hidden_set_to(surf, hide);
}

void SurfaceObserverImpl::input_consumed(mir::scene::Surface const* surf, MirEvent const* event)
{
    listener->input_consumed(surf, event);
}

void SurfaceObserverImpl::keymap_changed(mir::scene::Surface const* surf, MirInputDeviceId id, std::string const& model,
                            std::string const& layout, std::string const& variant, std::string const& options)
{
    listener->keymap_changed(surf, id, model, layout, variant, options);
}

void SurfaceObserverImpl::moved_to(mir::scene::Surface const* surf, mir::geometry::Point const& top_left)
{
    listener->moved_to(surf, top_left);
}

void SurfaceObserverImpl::orientation_set_to(mir::scene::Surface const* surf, MirOrientation orientation)
{
    listener->orientation_set_to(surf, orientation);
}

void SurfaceObserverImpl::placed_relative(mir::scene::Surface const* surf, mir::geometry::Rectangle const& placement)
{
    listener->placed_relative(surf, placement);
}

void SurfaceObserverImpl::renamed(mir::scene::Surface const* surf, char const* name)
{
    listener->renamed(surf, name);
}

void SurfaceObserverImpl::start_drag_and_drop(mir::scene::Surface const* surf, std::vector<uint8_t> const& handle)
{
    listener->start_drag_and_drop(surf, handle);
}

void SurfaceObserverImpl::transformation_set_to(mir::scene::Surface const* surf, glm::mat4 const& t)
{
    listener->transformation_set_to(surf, t);
}

void SurfaceObserverImpl::window_resized_to(mir::scene::Surface const* surf, mir::geometry::Size const& window_size)
{
    listener->window_resized_to(surf, window_size);
}

}

namespace miroil {
    
Surface::Surface(std::shared_ptr<mir::scene::Surface> wrapped)
: wrapped(wrapped)
{    
}

void Surface::add_observer(std::shared_ptr<miroil::SurfaceObserver> const& observer)
{
    auto it = observers.find(observer);
    if (it == observers.end()) {
        std::shared_ptr<SurfaceObserverImpl> impl = std::make_shared<SurfaceObserverImpl>(observer);
        
        wrapped->add_observer(impl);
        observers.insert({observer, impl});
    }
}

bool Surface::is_confined_to_window()
{
    return (wrapped->confine_pointer_state() == mir_pointer_confined_oneshot || wrapped->confine_pointer_state() == mir_pointer_confined_persistent);
}

void Surface::remove_observer(std::shared_ptr<miroil::SurfaceObserver> const& observer)
{
    auto it = observers.find(observer);
    if (it != observers.end()) {        
        wrapped->remove_observer(it->second);
        observers.erase(it);        
    }
}

mir::scene::Surface * Surface::getWrapped() const
{
    return wrapped.get();
}

int Surface::buffers_ready_for_compositor(void const* compositor_id) const
{
    return wrapped->buffers_ready_for_compositor(compositor_id);
}

mir::graphics::RenderableList Surface::generate_renderables(miroil::CompositorID id) const
{
    return wrapped->generate_renderables(id);
}

void Surface::set_orientation(MirOrientation orientation)
{
    wrapped->set_orientation(orientation);
}

void Surface::set_keymap(MirInputDeviceId id, std::string const& model, std::string const& layout,
                         std::string const& variant, std::string const& options)
{
    wrapped->set_keymap(id, model, layout, variant, options);
}

void Surface::set_confine_pointer_state(MirPointerConfinementState state)
{
    wrapped->set_confine_pointer_state(state);
}

std::shared_ptr<mir::scene::Surface> Surface::parent() const
{
    return wrapped->parent();
}

mir::geometry::Point Surface::top_left() const
{
    return wrapped->top_left();
}

bool Surface::visible() const
{
    return wrapped->visible();
}

int Surface::configure(MirWindowAttrib attrib, int value)
{
    return wrapped->configure(attrib, value);
}

int Surface::query(MirWindowAttrib attrib) const
{
    return wrapped->query(attrib);
}

}
