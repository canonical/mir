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
#include <miroil/surface.h>
#include <miroil/surface_observer.h>
#include "mir/scene/surface.h"
#include "mir/scene/surface_observer.h"
#include "mir/log.h"
#include "mir/executor.h"

class miroil::SurfaceObserverImpl : public mir::scene::SurfaceObserver
{
public:
  SurfaceObserverImpl(std::shared_ptr<miroil::SurfaceObserver> const &wrapped);
  virtual ~SurfaceObserverImpl();

  void alpha_set_to(mir::scene::Surface const *surf, float alpha) override;
  void application_id_set_to(mir::scene::Surface const *surf,
                             std::string const &application_id) override;
  void attrib_changed(mir::scene::Surface const *surf, MirWindowAttrib attrib,
                      int value) override;
  void client_surface_close_requested(mir::scene::Surface const *surf) override;
  void content_resized_to(mir::scene::Surface const *surf,
                          mir::geometry::Size const &content_size) override;
  void cursor_image_removed(mir::scene::Surface const *surf) override;
  void cursor_image_set_to(mir::scene::Surface const *surf,
                           std::weak_ptr<mir::graphics::CursorImage> const& image) override;
  void depth_layer_set_to(mir::scene::Surface const *surf,
                          MirDepthLayer depth_layer) override;
  void frame_posted(mir::scene::Surface const *surf, mir::geometry::Rectangle const& area) override;
  void hidden_set_to(mir::scene::Surface const *surf, bool hide) override;
  void input_consumed(mir::scene::Surface const *surf,
                      std::shared_ptr<MirEvent const> const& event) override;
  void moved_to(mir::scene::Surface const *surf,
                mir::geometry::Point const &top_left) override;
  void orientation_set_to(mir::scene::Surface const *surf,
                          MirOrientation orientation) override;
  void mirror_mode_set_to(mir::scene::Surface const* surf, MirMirrorMode mirror_mode) override;
  void placed_relative(mir::scene::Surface const *surf,
                       mir::geometry::Rectangle const &placement) override;
  void
  reception_mode_set_to(mir::scene::Surface const * /*surf*/,
                        mir::input::InputReceptionMode /*mode*/) override{};
  void renamed(mir::scene::Surface const *surf, std::string const& name) override;
  void transformation_set_to(mir::scene::Surface const *surf,
                             glm::mat4 const &t) override;
  void window_resized_to(mir::scene::Surface const *surf,
                         mir::geometry::Size const &window_size) override;
  void entered_output(mir::scene::Surface const* surf, mir::graphics::DisplayConfigurationOutputId const& id) override;
  void left_output(mir::scene::Surface const* surf, mir::graphics::DisplayConfigurationOutputId const& id) override;
  void rescale_output(mir::scene::Surface const* surf, mir::graphics::DisplayConfigurationOutputId const& id) override;
  void tiled_edges(mir::scene::Surface const* surf, mir::Flags<MirTiledEdge> edges) override;

private:
  std::shared_ptr<miroil::SurfaceObserver> listener;
};

miroil::SurfaceObserverImpl::SurfaceObserverImpl(std::shared_ptr<miroil::SurfaceObserver> const & wrapped)
: listener(wrapped)
{
}

miroil::SurfaceObserverImpl::~SurfaceObserverImpl() = default;

void miroil::SurfaceObserverImpl::alpha_set_to(mir::scene::Surface const* surf, float alpha)
{
    listener->alpha_set_to(surf, alpha);
}

void miroil::SurfaceObserverImpl::application_id_set_to(mir::scene::Surface const* surf, std::string const& application_id)
{
    listener->application_id_set_to(surf, application_id);
}

void miroil::SurfaceObserverImpl::attrib_changed(mir::scene::Surface const* surf, MirWindowAttrib attrib, int value)
{
    listener->attrib_changed(surf, attrib, value);
}

void miroil::SurfaceObserverImpl::client_surface_close_requested(mir::scene::Surface const* surf)
{
    listener->client_surface_close_requested(surf);
}

void miroil::SurfaceObserverImpl::content_resized_to(mir::scene::Surface const* surf, mir::geometry::Size const& content_size)
{
    listener->content_resized_to(surf, content_size);
}

void miroil::SurfaceObserverImpl::cursor_image_removed(mir::scene::Surface const* surf)
{
    listener->cursor_image_removed(surf);
}

void miroil::SurfaceObserverImpl::cursor_image_set_to(mir::scene::Surface const* surf, std::weak_ptr<mir::graphics::CursorImage> const& image)
{
    if (auto const locked = image.lock())
    {
        listener->cursor_image_set_to(surf, *locked);
    }
}

void miroil::SurfaceObserverImpl::depth_layer_set_to(mir::scene::Surface const* surf, MirDepthLayer depth_layer)
{
    listener->depth_layer_set_to(surf, depth_layer);
}

void miroil::SurfaceObserverImpl::frame_posted(mir::scene::Surface const* surf, mir::geometry::Rectangle const& area)
{
    listener->frame_posted(surf, 1, area.size);
}

void miroil::SurfaceObserverImpl::hidden_set_to(mir::scene::Surface const* surf, bool hide)
{
    listener->hidden_set_to(surf, hide);
}

void miroil::SurfaceObserverImpl::input_consumed(mir::scene::Surface const* surf, std::shared_ptr<MirEvent const> const& event)
{
    listener->input_consumed(surf, event.get());
}

void miroil::SurfaceObserverImpl::moved_to(mir::scene::Surface const* surf, mir::geometry::Point const& top_left)
{
    listener->moved_to(surf, top_left);
}

void miroil::SurfaceObserverImpl::orientation_set_to(mir::scene::Surface const* surf, MirOrientation orientation)
{
    listener->orientation_set_to(surf, orientation);
}

void miroil::SurfaceObserverImpl::mirror_mode_set_to(mir::scene::Surface const* surf, MirMirrorMode mirror_mode)
{
    listener->mirror_mode_set_to(surf, mirror_mode);
}

void miroil::SurfaceObserverImpl::placed_relative(mir::scene::Surface const* surf, mir::geometry::Rectangle const& placement)
{
    listener->placed_relative(surf, placement);
}

void miroil::SurfaceObserverImpl::renamed(mir::scene::Surface const* surf, std::string const& name)
{
    listener->renamed(surf, name.c_str());
}

void miroil::SurfaceObserverImpl::transformation_set_to(mir::scene::Surface const* surf, glm::mat4 const& t)
{
    listener->transformation_set_to(surf, t);
}

void miroil::SurfaceObserverImpl::window_resized_to(mir::scene::Surface const* surf, mir::geometry::Size const& window_size)
{
    listener->window_resized_to(surf, window_size);
}

void miroil::SurfaceObserverImpl::entered_output(
    mir::scene::Surface const* /*surf*/,
    mir::graphics::DisplayConfigurationOutputId const& /*id*/)
{
}

void miroil::SurfaceObserverImpl::left_output(
    mir::scene::Surface const* /*surf*/,
    mir::graphics::DisplayConfigurationOutputId const& /*id*/)
{
}

void miroil::SurfaceObserverImpl::rescale_output(
    mir::scene::Surface const* /*surf*/,
    mir::graphics::DisplayConfigurationOutputId const& /*id*/)
{
}

void miroil::SurfaceObserverImpl::tiled_edges(
    mir::scene::Surface const* /*surf*/,
    mir::Flags<MirTiledEdge> /*edges*/)
{
}

miroil::Surface::Surface(std::shared_ptr<mir::scene::Surface> wrapped) :
     wrapped(wrapped)
{
}

void miroil::Surface::add_observer(std::shared_ptr<SurfaceObserver> const& observer)
{
    auto it = observers.find(observer);
    if (it == observers.end()) {
        std::shared_ptr<SurfaceObserverImpl> impl = std::make_shared<SurfaceObserverImpl>(observer);

        wrapped->register_interest(impl, mir::immediate_executor);
        observers.insert({observer, impl});
    }
}

bool miroil::Surface::is_confined_to_window()
{
    return (wrapped->confine_pointer_state() == mir_pointer_confined_oneshot ||
            wrapped->confine_pointer_state() == mir_pointer_confined_persistent);
}

void miroil::Surface::remove_observer(std::shared_ptr<miroil::SurfaceObserver> const& observer)
{
    auto it = observers.find(observer);
    if (it != observers.end()) {
        wrapped->unregister_interest(*it->second);
        observers.erase(it);
    }
}

auto miroil::Surface::get_wrapped() const -> mir::scene::Surface*
{
    return wrapped.get();
}

mir::graphics::RenderableList miroil::Surface::generate_renderables(miroil::CompositorID id) const
{
    return wrapped->generate_renderables(id);
}

void miroil::Surface::set_orientation(MirOrientation orientation)
{
    wrapped->set_orientation(orientation);
}

void miroil::Surface::set_mirror_mode(MirMirrorMode mirror_mode)
{
    wrapped->set_mirror_mode(mirror_mode);
}

void miroil::Surface::set_confine_pointer_state(MirPointerConfinementState state)
{
    wrapped->set_confine_pointer_state(state);
}

std::shared_ptr<mir::scene::Surface> miroil::Surface::parent() const
{
    return wrapped->parent();
}

mir::geometry::Point miroil::Surface::top_left() const
{
    return wrapped->top_left();
}

bool miroil::Surface::visible() const
{
    return wrapped->visible();
}

int miroil::Surface::configure(MirWindowAttrib attrib, int value)
{
    return wrapped->configure(attrib, value);
}

int miroil::Surface::query(MirWindowAttrib attrib) const
{
    return wrapped->query(attrib);
}

void miroil::Surface::set_keymap(MirInputDeviceId /*id*/, const std::string& /*model*/,
                         const std::string& /*layout*/, const std::string& /*variant*/,
                         const std::string& /*options*/)
{
    mir::log_warning("Stubbed function miroil::Surface::set_keymap() called - per-surface keymaps no longer supported");
}
