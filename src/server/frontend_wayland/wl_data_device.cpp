/*
 * Copyright © Canonical Ltd.
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
 */

#include "wl_data_device.h"
#include "wl_data_source.h"
#include "wl_surface.h"

#include <mir/events/pointer_event.h>
#include <mir/executor.h>
#include <mir/frontend/drag_icon_controller.h>
#include <mir/frontend/pointer_input_dispatcher.h>
#include <mir/log.h>
#include <mir/scene/clipboard.h>
#include <mir/scene/session.h>
#include <mir/scene/surface.h>
#include <mir/shell/surface_specification.h>
#include "client.h"
#include "protocol_error.h"
#include "weak.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland_rs;
using namespace mir::geometry;

class mf::WlDataDevice::ClipboardObserver : public ms::ClipboardObserver
{
public:
    explicit ClipboardObserver(std::shared_ptr<WlDataDevice> const& device) : device{device}
    {
    }

private:
    void paste_source_set(std::shared_ptr<ms::DataExchangeSource> const& source) override
    {
        if (device)
        {
            device.value().paste_source_set(source);
        }
    }

    void drag_n_drop_source_set(std::shared_ptr<ms::DataExchangeSource> const& source) override
    {
        if (device)
        {
            device.value().drag_n_drop_source_set(source);
        }
    }

    void drag_n_drop_source_cleared(std::shared_ptr<ms::DataExchangeSource> const& /*source*/) override
    {
        if (device)
        {
            device.value().drag_n_drop_source_cleared();
        }
    }

    void end_of_dnd_gesture() override
    {
        if (device)
        {
            device.value().end_of_dnd_gesture();
        }
    }

    wayland_rs::Weak<WlDataDevice> const device;
};

class mf::WlDataDevice::Offer : public wayland_rs::WlDataOfferImpl, public std::enable_shared_from_this<mf::WlDataDevice::Offer>
{
public:
    Offer(std::shared_ptr<WlDataDevice> const& device, std::shared_ptr<ms::DataExchangeSource> const& source);

    auto associate(rust::Box<wayland_rs::WlDataOfferExt> instance, uint32_t object_id) -> void override;

    auto accept(uint32_t, rust::String mime_type, bool has_mime_type) -> void override
    {
        accepted_mime_type = has_mime_type ? std::string(mime_type.c_str()) : std::optional<std::string>();
        source->offer_accepted(accepted_mime_type);
    }

    auto receive(rust::String mime_type, int32_t fd) -> void override;

    void finish() override
    {
        source->dnd_finished();
    }

    void set_actions(uint32_t dnd_actions, uint32_t preferred_action) override
    {
        const auto action = source->offer_set_actions(dnd_actions, preferred_action);

        if (!dnd_action || dnd_action.value() != action)
        {
            dnd_action = action;
            send_action_event(action);
        }
    }

private:
    friend mf::WlDataDevice;
    mw::Weak<WlDataDevice> const device;
    std::shared_ptr<ms::DataExchangeSource> const source;
    std::optional<std::string> accepted_mime_type;
    std::optional<uint32_t> dnd_action;
};

mf::WlDataDevice::Offer::Offer(std::shared_ptr<WlDataDevice> const& device, std::shared_ptr<ms::DataExchangeSource> const& source) :
    device{device},
    source{source}
{
}

auto mf::WlDataDevice::Offer::associate(rust::Box<mw::WlDataOfferExt> instance, uint32_t object_id) -> void
{
    device.value().send_data_offer_event(instance);
    WlDataOfferImpl::associate(std::move(instance), object_id);
    for (auto const& type : source->mime_types())
    {
        send_offer_event(type);
    }
}

auto mf::WlDataDevice::Offer::receive(rust::String mime_type, int32_t fd) -> void
{
    if (device && device.value().current_offer.is(*this))
    {
        source->initiate_send(mime_type.c_str(), Fd{IntOwnedFd{fd}});
    }
}

mf::WlDataDevice::WlDataDevice(
    Executor& wayland_executor,
    scene::Clipboard& clipboard,
    WlSeat& seat,
    std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher,
    std::shared_ptr<DragIconController> drag_icon_controller,
    std::shared_ptr<wayland_rs::Client> const& client)
    : clipboard{clipboard},
      seat{seat},
      clipboard_observer{std::make_shared<ClipboardObserver>(shared_from_this())},
      pointer_input_dispatcher{std::move(pointer_input_dispatcher)},
      drag_icon_controller{std::move(drag_icon_controller)},
      client{client},
      end_of_gesture_callback{[this, &wayland_executor] { wayland_executor.spawn([this]
        { this->clipboard.end_of_dnd_gesture(); drag_surface.reset(); }); }}
{
    clipboard.register_interest(clipboard_observer, wayland_executor);
    // this will call focus_on() with the initial state
    seat.add_focus_listener(client.get(), this);
}

mf::WlDataDevice::~WlDataDevice()
{
    clipboard.unregister_interest(*clipboard_observer);
    seat.remove_focus_listener(client.get(), this);
}

auto mir::frontend::WlDataDevice::set_selection(
    mw::Weak<mw::WlDataSourceImpl> const& source,
    bool has_source,
    uint32_t serial) -> void
{
    // TODO: verify serial
    (void)serial;
    if (has_source)
    {
        auto const wl_source = WlDataSource::from(&source.value());
        wl_source->set_clipboard_paste_source();
    }
    else
    {
        clipboard.clear_paste_source();
    }
}

auto mir::frontend::WlDataDevice::start_drag(
    mw::Weak<mw::WlDataSourceImpl> const& source,
    bool has_source,
    mw::Weak<mw::WlSurfaceImpl> const& origin,
    mw::Weak<mw::WlSurfaceImpl> const& icon,
    bool has_icon,
    uint32_t serial) -> void
{
    // "The client must have an active implicit grab that matches the serial"
    if (!weak_surface || weak_surface.value().client.lock() != client || WlSurface::from(&origin.value())->client.lock() != client)
    {
        BOOST_THROW_EXCEPTION(
            mw::ProtocolError(object_id(), Error::role, "The client must have an active implicit grab"));
    }

    validate_pointer_event(client->event_for(serial));

    pointer_input_dispatcher->disable_dispatch_to_gesture_owner(end_of_gesture_callback);

    seat.for_each_listener(client, [this](PointerEventDispatcher* pointer)
        {
            pointer->start_dispatch_to_data_device(this);
        });

    if (has_icon)
    {
        auto const icon_surface = WlSurface::from(&icon.value());

        drag_surface.emplace(icon_surface->shared_from_this(), drag_icon_controller);
    }

    if (has_source)
    {
        if (auto const wl_source = WlDataSource::from(&source.value()))
        {
            wl_source->start_drag_n_drop_gesture();
        }
    }
}

void mf::WlDataDevice::validate_pointer_event(std::optional<std::shared_ptr<MirEvent const>> drag_event) const
{
    if (!drag_event || !drag_event.value() || mir_event_get_type(drag_event.value().get()) != mir_event_type_input)
    {
        BOOST_THROW_EXCEPTION(
            mw::ProtocolError(object_id(), Error::role, "Serial does not correspond to an input event"));
    }

    auto const input_ev = mir_event_get_input_event(drag_event.value().get());
    if (mir_input_event_get_type(input_ev) != mir_input_event_type_pointer)
    {
        BOOST_THROW_EXCEPTION(
            mw::ProtocolError(object_id(), Error::role, "Serial does not correspond to a pointer event"));
    }
}

void mf::WlDataDevice::focus_on(std::shared_ptr<WlSurface> const& surface)
{
    weak_surface = mw::Weak(surface);
    has_focus = static_cast<bool>(surface);
    paste_source_set(clipboard.paste_source());
}

void mf::WlDataDevice::paste_source_set(std::shared_ptr<ms::DataExchangeSource> const& source)
{
    if (source && has_focus)
    {
        if (!current_offer || current_offer.value().source != source)
        {
            auto const offer = std::make_shared<Offer>(shared_from_this(), source);
            current_offer = mw::Weak(offer);
            send_selection_event(offer, true);
        }
    }
    else
    {
        if (current_offer)
        {
            current_offer = {};
            send_selection_event(nullptr, false);
        }
    }
}

void mf::WlDataDevice::drag_n_drop_source_set(std::shared_ptr<scene::DataExchangeSource> const& source)
{
    weak_source = source;
    seat.for_each_listener(client, [this](PointerEventDispatcher* pointer)
        {
            pointer->start_dispatch_to_data_device(this);
        });

    if (has_focus)
    {
        make_new_dnd_offer_if_possible(source);
        sent_enter = false;
    }
}

void mf::WlDataDevice::event(std::shared_ptr<MirPointerEvent const> const& event, WlSurface& root_surface)
{
    switch(mir_pointer_event_action(event.get()))
    {
    case mir_pointer_action_button_up:
        send_drop_event();
        send_leave_event();
        if (current_offer)
        {
            if (!current_offer.value().accepted_mime_type)
            {
                current_offer.value().source->cancelled();
            }
            else
            {
                current_offer.value().source->dnd_drop_performed();
            }
        }
        break;
    case mir_pointer_action_leave:
        send_leave_event();
        break;
    case mir_pointer_action_enter:
        make_new_dnd_offer_if_possible(weak_source.lock());
        sent_enter = false;
        [[fallthrough]];
    case mir_pointer_action_motion:
    {
        if (!event->local_position())
        {
            log_error("pointer event cannot be sent to wl_surface as it lacks a local poisition");
            break;
        }

        auto const root_position = event->local_position().value();

        WlSurface* const target_surface = root_surface.subsurface_at(Point{root_position}).value_or(&root_surface);

        auto const position_on_target = root_position - DisplacementF{target_surface->total_offset()};
        auto const x = position_on_target.x.as_value();
        auto const y = position_on_target.y.as_value();

        if (sent_enter)
        {
            auto const timestamp = mir_input_event_get_wayland_timestamp(mir_pointer_event_input_event(event.get()));

            send_motion_event(timestamp, x, y);
        }
        else
        {
            auto const serial = client->next_serial(nullptr);
            if (current_offer)
            {
                send_enter_event(serial, target_surface->shared_from_this(), x, y, current_offer.value().shared_from_this(), true);
            }
            else
            {
                send_enter_event(serial, target_surface->shared_from_this(), x, y, nullptr, false);
            }

            sent_enter = true;
        }
        break;
    }
    case mir_pointer_action_button_down:
    case mir_pointer_actions:
        break;
    }
}

void mf::WlDataDevice::drag_n_drop_source_cleared()
{
    weak_source.reset();
}

void mf::WlDataDevice::end_of_dnd_gesture()
{
    seat.for_each_listener(client, [](PointerEventDispatcher* pointer)
    {
        pointer->stop_dispatch_to_data_device();
    });
}

void mf::WlDataDevice::make_new_dnd_offer_if_possible(std::shared_ptr<mir::scene::DataExchangeSource> const& source)
{
    if (source)
    {
        current_offer = mw::Weak(std::make_shared<Offer>(shared_from_this(), source));
        current_offer.value().send_action_event(mw::WlDataDeviceManagerImpl::DndAction::none);
        current_offer.value().send_source_actions_event(source->actions());
    }
}

mf::WlDataDevice::DragIconSurface::DragIconSurface(std::shared_ptr<WlSurface> const& icon, std::shared_ptr<DragIconController> drag_icon_controller)
    : NullWlSurfaceRole(icon.get()),
      surface{icon},
      drag_icon_controller{std::move(drag_icon_controller)}
{
    icon->set_role(this);

    auto spec = shell::SurfaceSpecification();
    if (auto const size = surface.value().buffer_size())
    {
        spec.width = size->width;
        spec.height = size->height;
    }
    spec.streams = std::vector<shell::StreamSpecification>{};
    spec.input_shape = std::vector<Rectangle>{};
    spec.depth_layer = mir_depth_layer_overlay;

    surface.value().populate_surface_data(spec.streams.value(), spec.input_shape.value(), {});

    auto const& session = surface.value().session;

    shared_scene_surface = session->create_surface(session, spec, nullptr, nullptr);

    DragIconSurface::drag_icon_controller->set_drag_icon(shared_scene_surface);
}

mf::WlDataDevice::DragIconSurface::~DragIconSurface()
{
    if (surface)
    {
        surface.value().clear_role();

        if (shared_scene_surface)
        {
            auto const& session = surface.value().session;
            session->destroy_surface(shared_scene_surface);
        }
    }
}

auto mf::WlDataDevice::DragIconSurface::scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>>
{
    return shared_scene_surface;
}
