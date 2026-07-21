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

#include "protocol_error.h"
#include "wl_data_device_manager.h"
#include "wl_data_source.h"
#include "wl_surface.h"

#include "wayland_rs/src/ffi.rs.h"

#include <mir/events/pointer_event.h>
#include <mir/executor.h>
#include <mir/frontend/drag_icon_controller.h>
#include <mir/frontend/pointer_input_dispatcher.h>
#include <mir/log.h>
#include <mir/scene/clipboard.h>
#include <mir/scene/data_exchange.h>
#include <mir/scene/session.h>
#include <mir/scene/surface.h>
#include <mir/shell/surface_specification.h>

#include <unistd.h>

#include <stdexcept>
#include <utility>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland_rs;
using namespace mir::geometry;

namespace
{
auto as_data_offer_ptr(mw::DataOffer* offer) -> std::shared_ptr<mw::DataOffer>
{
    return offer ? std::shared_ptr<mw::DataOffer>{std::shared_ptr<void>{}, offer} : nullptr;
}
}

class mf::WlDataDevice::ClipboardObserver : public ms::ClipboardObserver
{
public:
    explicit ClipboardObserver(WlDataDevice* device) : device{device}
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

    void drag_n_drop_source_cleared(std::shared_ptr<ms::DataExchangeSource> const&) override
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

    mw::Weak<WlDataDevice> const device;
};

class mf::WlDataDevice::Offer : public mw::DataOffer
{
public:
    Offer(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::DataOfferMiddleware> instance,
        uint32_t object_id,
        OfferType type,
        WlDataDevice* device,
        std::shared_ptr<ms::DataExchangeSource> const& source)
        : mw::DataOffer{std::move(client), std::move(instance), object_id},
          type{type},
          device{device},
          source{source}
    {
    }

    auto accept(uint32_t, std::optional<rust::String> const& mime_type) -> void override
    {
        accepted_mime_type = mime_type.transform([](rust::String const& mime)
            {
                return std::string{mime};
            });
        source->offer_accepted(accepted_mime_type);
    }

    auto receive(rust::String mime_type, int32_t fd) -> void override
    {
        if (device && device.value().current_offer.is(*this))
        {
            source->initiate_send(std::string{mime_type}, mir::Fd{mir::IntOwnedFd{::dup(fd)}});
        }
    }

    auto destroy() -> void override
    {
        destroy_and_delete();
    }

    auto finish() -> void override
    {
        if (type != OfferType::dnd)
        {
            throw mw::ProtocolError{
                object_id(),
                Error::invalid_offer,
                "finish is only valid for drag and drop offers"};
        }

        source->dnd_finished();
    }

    auto set_actions(uint32_t dnd_actions, uint32_t preferred_action) -> void override
    {
        if (!mf::validate_dnd_actions(dnd_actions))
        {
            throw mw::ProtocolError{
                object_id(),
                Error::invalid_action_mask,
                "Invalid DnD actions 0x%x", dnd_actions};
        }
        if (!mf::validate_dnd_action(preferred_action))
        {
            throw mw::ProtocolError{
                object_id(),
                Error::invalid_action,
                "Invalid DnD action 0x%x", preferred_action};
        }
        if (preferred_action != mw::DataDeviceManager::DndAction::none && (dnd_actions & preferred_action) == 0)
        {
            throw mw::ProtocolError{
                object_id(),
                Error::invalid_action,
                "Preferred action 0x%x not in DnD actions 0x%x", preferred_action, dnd_actions};
        }
        if (type != OfferType::dnd)
        {
            throw mw::ProtocolError{
                object_id(),
                Error::invalid_offer,
                "set_actions is only valid for drag and drop offers"};
        }

        auto const action = source->offer_set_actions(dnd_actions, preferred_action);

        if (!dnd_action || dnd_action.value() != action)
        {
            dnd_action = action;
            send_action_event_if_supported(action);
        }
    }

private:
    friend WlDataDevice;
    OfferType const type;
    mw::Weak<WlDataDevice> const device;
    std::shared_ptr<ms::DataExchangeSource> const source;
    std::optional<std::string> accepted_mime_type;
    std::optional<uint32_t> dnd_action;
};

mf::WlDataDevice::WlDataDevice(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::DataDeviceMiddleware> instance,
    uint32_t object_id,
    Executor& wayland_executor,
    scene::Clipboard& clipboard,
    WlSeat& seat,
    std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher,
    std::shared_ptr<DragIconController> drag_icon_controller)
    : mw::DataDevice{std::move(client), std::move(instance), object_id},
      clipboard{clipboard},
      seat{seat},
      clipboard_observer{std::make_shared<ClipboardObserver>(this)},
      pointer_input_dispatcher{std::move(pointer_input_dispatcher)},
      drag_icon_controller{std::move(drag_icon_controller)},
      end_of_gesture_callback{[this, &wayland_executor]
        {
            wayland_executor.spawn([this]
                {
                    this->clipboard.end_of_dnd_gesture();
                    drag_surface.reset();
                });
        }}
{
    clipboard.register_interest(clipboard_observer, wayland_executor);
    // this will call focus_on() with the initial state
    // (this->client: the constructor parameter of the same name is moved-from)
    seat.add_focus_listener(this->client.get(), this);
}

mf::WlDataDevice::~WlDataDevice()
{
    clipboard.unregister_interest(*clipboard_observer);
    seat.remove_focus_listener(client.get(), this);
}

auto mf::WlDataDevice::set_selection(
    std::optional<mw::Weak<mw::DataSource>> const& source,
    uint32_t serial) -> void
{
    // TODO: verify serial
    (void)serial;
    if (source)
    {
        if (auto const wl_source = mw::DataSource::from<WlDataSource>(*source))
        {
            wl_source->set_clipboard_paste_source();
        }
    }
    else
    {
        clipboard.clear_paste_source();
    }
}

auto mf::WlDataDevice::start_drag(
    std::optional<mw::Weak<mw::DataSource>> const& source,
    mw::Weak<mw::Surface> const& origin,
    std::optional<mw::Weak<mw::Surface>> const& icon,
    uint32_t serial) -> void
{
    auto const origin_surface = mw::Surface::from<WlSurface>(origin);
    if (!weak_surface || weak_surface.value().client != client || !origin_surface || origin_surface->client != client)
    {
        throw mw::ProtocolError{object_id(), Error::role, "The client must have an active implicit grab"};
    }

    validate_pointer_event(client->event_for(serial));

    pointer_input_dispatcher->disable_dispatch_to_gesture_owner(end_of_gesture_callback);

    seat.for_each_listener(client.get(), [this](PointerEventDispatcher* pointer)
        {
            pointer->start_dispatch_to_data_device(this);
        });

    if (icon)
    {
        if (auto const icon_surface = mw::Surface::from<WlSurface>(*icon))
        {
            drag_surface.emplace(icon_surface, drag_icon_controller);
        }
    }

    if (source)
    {
        if (auto const wl_source = mw::DataSource::from<WlDataSource>(*source))
        {
            wl_source->start_drag_n_drop_gesture();
        }
    }
}

void mf::WlDataDevice::validate_pointer_event(std::optional<std::shared_ptr<MirEvent const>> drag_event)
{
    if (!drag_event || !drag_event.value() || mir_event_get_type(drag_event.value().get()) != mir_event_type_input)
    {
        throw mw::ProtocolError{object_id(), Error::role, "Serial does not correspond to an input event"};
    }

    auto const input_ev = mir_event_get_input_event(drag_event.value().get());
    if (mir_input_event_get_type(input_ev) != mir_input_event_type_pointer)
    {
        throw mw::ProtocolError{object_id(), Error::role, "Serial does not correspond to a pointer event"};
    }
}

void mf::WlDataDevice::focus_on(WlSurface* surface)
{
    weak_surface = surface ? mw::make_weak(surface) : mw::Weak<WlSurface>{};
    has_focus = static_cast<bool>(surface);
    paste_source_set(clipboard.paste_source());
}

auto mf::WlDataDevice::create_offer(
    OfferType type,
    std::shared_ptr<ms::DataExchangeSource> const& source) -> std::shared_ptr<Offer>
{
    Offer* offer_ptr = nullptr;
    auto offer = mw::create_wl_data_offer(
        *client->raw_client(),
        get_box()->version(),
        [&](rust::Box<mw::DataOfferMiddleware> box, uint32_t id) -> std::shared_ptr<mw::DataOffer>
        {
            auto created = std::make_shared<Offer>(client, std::move(box), id, type, this, source);
            offer_ptr = created.get();
            return created;
        });

    if (!offer_ptr)
    {
        throw std::logic_error{"failed to create wl_data_offer"};
    }

    send_data_offer_event(offer->get_box());
    for (auto const& mime_type : source->mime_types())
    {
        offer_ptr->send_offer_event(mime_type);
    }

    return std::static_pointer_cast<Offer>(offer);
}

void mf::WlDataDevice::paste_source_set(std::shared_ptr<ms::DataExchangeSource> const& source)
{
    if (source && has_focus)
    {
        if (!current_offer || current_offer.value().source != source)
        {
            auto const offer = create_offer(OfferType::selection, source);
            current_offer = mw::make_weak(offer.get());
            send_selection_event(as_data_offer_ptr(offer.get()));
        }
    }
    else
    {
        if (current_offer)
        {
            current_offer = {};
            send_selection_event(std::nullopt);
        }
    }
}

void mf::WlDataDevice::drag_n_drop_source_set(std::shared_ptr<scene::DataExchangeSource> const& source)
{
    weak_source = source;
    seat.for_each_listener(client.get(), [this](PointerEventDispatcher* pointer)
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
            if (!current_offer.value().accepted_mime_type && get_box()->version() >= 3)
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
                send_enter_event(serial, as_surface_ptr(target_surface), x, y, as_data_offer_ptr(&current_offer.value()));
            }
            else
            {
                send_enter_event(serial, as_surface_ptr(target_surface), x, y, std::nullopt);
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
    seat.for_each_listener(client.get(), [](PointerEventDispatcher* pointer)
    {
        pointer->stop_dispatch_to_data_device();
    });
}

void mf::WlDataDevice::make_new_dnd_offer_if_possible(std::shared_ptr<mir::scene::DataExchangeSource> const& source)
{
    if (source)
    {
        auto const offer = create_offer(OfferType::dnd, source);
        current_offer = mw::make_weak(offer.get());
        current_offer.value().send_action_event_if_supported(mw::DataDeviceManager::DndAction::none);
        current_offer.value().send_source_actions_event_if_supported(source->actions());
    }
}

mf::WlDataDevice::DragIconSurface::DragIconSurface(WlSurface* icon, std::shared_ptr<DragIconController> drag_icon_controller)
    : NullWlSurfaceRole(icon),
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
