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

#include "mir/events/pointer_event.h"
#include "mir/log.h"
#include "mir/scene/clipboard.h"
#include "mir/wayland/client.h"
#include "mir/wayland/protocol_error.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland;
using namespace mir::geometry;

class mf::WlDataDevice::ClipboardObserver : public ms::ClipboardObserver
{
public:
    ClipboardObserver(WlDataDevice* device) : device{device}
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

    void drag_n_drop_source_cleared(std::shared_ptr<ms::DataExchangeSource> const& source) override
    {
        if (device)
        {
            device.value().drag_n_drop_source_cleared(source);
        }
    }

private:

    wayland::Weak<WlDataDevice> const device;
};

class mf::WlDataDevice::Offer : public wayland::DataOffer
{
public:
    Offer(WlDataDevice* device, std::shared_ptr<ms::DataExchangeSource> const& source);

    void accept(uint32_t serial, std::optional<std::string> const& mime_type) override
    {
        (void)serial;
        accepted_mime_type = mime_type;
        source->offer_accepted(mime_type);
    }

    void receive(std::string const& mime_type, mir::Fd fd) override;

    void finish() override
    {
        if (device && device.value().current_offer.is(*this))
        {
            device.value().clipboard.clear_drag_n_drop_source(source);
        }
    }

    void set_actions(uint32_t dnd_actions, uint32_t preferred_action) override
    {
        send_action_event_if_supported(source->offer_set_actions(dnd_actions, preferred_action));
    }

private:
    friend mf::WlDataDevice;
    wayland::Weak<WlDataDevice> const device;
    std::shared_ptr<ms::DataExchangeSource> const source;
    std::optional<std::string> accepted_mime_type;
};

mf::WlDataDevice::Offer::Offer(WlDataDevice* device, std::shared_ptr<ms::DataExchangeSource> const& source) :
    mw::DataOffer(*device),
    device{device},
    source{source}
{
    device->send_data_offer_event(resource);
    for (auto const& type : source->mime_types())
    {
        send_offer_event(type);
    }

    send_action_event_if_supported(mw::DataDeviceManager::DndAction::none);
    send_source_actions_event_if_supported(source->actions());
}

void mf::WlDataDevice::Offer::receive(std::string const& mime_type, mir::Fd fd)
{
    if (device && device.value().current_offer.is(*this))
    {
        source->initiate_send(mime_type, fd);
    }
}

mf::WlDataDevice::WlDataDevice(
    wl_resource* new_resource,
    Executor& wayland_executor,
    scene::Clipboard& clipboard,
    WlSeat& seat)
    : mw::DataDevice(new_resource, Version<3>()),
      clipboard{clipboard},
      seat{seat},
      clipboard_observer{std::make_shared<ClipboardObserver>(this)}
{
    clipboard.register_interest(clipboard_observer, wayland_executor);
    // this will call focus_on() with the initial state
    seat.add_focus_listener(client, this);
}

mf::WlDataDevice::~WlDataDevice()
{
    clipboard.unregister_interest(*clipboard_observer);
    seat.remove_focus_listener(client, this);
}

void mf::WlDataDevice::set_selection(std::optional<wl_resource*> const& source, uint32_t serial)
{
    // TODO: verify serial
    (void)serial;
    if (source)
    {
        auto const wl_source = WlDataSource::from(source.value());
        wl_source->set_clipboard_paste_source();
    }
    else
    {
        clipboard.clear_paste_source();
    }
}

void mf::WlDataDevice::start_drag(
    std::optional<wl_resource*> const& source,
    wl_resource* origin,
    std::optional<wl_resource*> const& icon,
    uint32_t serial)
{
    // "The [origin surface] and client must have an active implicit grab that matches the serial"
    if (!origin)
    {
        BOOST_THROW_EXCEPTION(
            mw::ProtocolError(resource, Error::role, "Origin surface does not exist."));
    }

    validate_pointer_event(client->event_for(serial));

    if (auto const wl_source = WlDataSource::from(source.value()))
    {
        wl_source->start_drag_n_drop_gesture(icon);
    }
}

void mf::WlDataDevice::validate_pointer_event(std::optional<std::shared_ptr<MirEvent const>> drag_event) const
{
    if (!drag_event || !drag_event.value() || mir_event_get_type(drag_event.value().get()) != mir_event_type_input)
    {
        BOOST_THROW_EXCEPTION(
            mw::ProtocolError(this->resource, Error::role, "Serial does not correspond to an input event"));
    }

    auto const input_ev = mir_event_get_input_event(drag_event.value().get());
    if (mir_input_event_get_type(input_ev) != mir_input_event_type_pointer)
    {
        BOOST_THROW_EXCEPTION(
            mw::ProtocolError(this->resource, Error::role, "Serial does not correspond to a pointer event"));
    }
}

void mf::WlDataDevice::focus_on(WlSurface* surface)
{
    weak_surface = make_weak(surface);
    has_focus = static_cast<bool>(surface);
    paste_source_set(clipboard.paste_source());
}

void mf::WlDataDevice::paste_source_set(std::shared_ptr<ms::DataExchangeSource> const& source)
{
    if (source && has_focus)
    {
        if (!current_offer || current_offer.value().source != source)
        {
            current_offer = wayland::make_weak(new Offer{this, source});
            send_selection_event(current_offer.value().resource);
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
    seat.for_each_listener(client, [this](PointerEventDispatcher* pointer)
        {
            pointer->start_dispatch_to_data_device(this);
        });

    if (source && has_focus)
    {
        if (!current_offer || current_offer.value().source != source)
        {
            if (weak_surface)
            {
                current_offer = wayland::make_weak(new Offer{this, source});
                sent_enter = false;
            }
        }
    }
}

void mf::WlDataDevice::event(std::shared_ptr<MirPointerEvent const> const& event, WlSurface& root_surface)
{
    switch(mir_pointer_event_action(event.get()))
    {
    case mir_pointer_action_button_down:
    case mir_pointer_action_button_up:
        if (current_offer)
        {
            send_drop_event();
            if (!current_offer.value().accepted_mime_type && wl_resource_get_version(resource) >= 3)
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
        if (auto const source = weak_source.lock())
        {
            current_offer = wayland::make_weak(new Offer{this, source});
            sent_enter = false;
        }
        [[fallthrough]];
    case mir_pointer_action_motion:
    {
        if (!event->local_position())
        {
            log_error("pointer event cannot be sent to wl_surface as it lacks a local poisition");
            break;
        }

        if (!current_offer) break;

        auto const root_position = event->local_position().value();
        WlSurface* target_surface;

        Point root_point{root_position};
        target_surface = root_surface.subsurface_at(root_point).value_or(&root_surface);

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
            send_enter_event(serial, target_surface->resource, x, y, current_offer.value().resource);
            sent_enter = true;
        }
        break;
    }
    case mir_pointer_actions:
        break;
    }
}

void mf::WlDataDevice::drag_n_drop_source_cleared(std::shared_ptr<scene::DataExchangeSource> const& /*source*/)
{
    weak_source.reset();

    seat.for_each_listener(client, [](PointerEventDispatcher* pointer)
    {
        pointer->stop_dispatch_to_data_device();
    });
}
