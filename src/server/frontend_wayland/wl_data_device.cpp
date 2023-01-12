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
#include "mir/geometry/forward.h"
#include "mir/input/composite_event_filter.h"
#include "mir/scene/clipboard.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/shell/surface_specification.h"
#include "mir/wayland/client.h"
#include "mir/wayland/protocol_error.h"

#include "mir_toolkit/events/enums.h"
#include "mir_toolkit/events/event.h"

namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace mw = mir::wayland;
namespace mev = mir::events;

class mf::WlDataDevice::ClipboardObserver : public ms::ClipboardObserver
{
public:
    ClipboardObserver(WlDataDevice* device) : device{device}
    {
    }

private:
    void paste_source_set(std::shared_ptr<ms::ClipboardSource> const& source) override
    {
        if (device)
        {
            device.value().paste_source_set(source);
        }
    }

    wayland::Weak<WlDataDevice> const device;
};

class mf::WlDataDevice::CursorEventFilter : public mi::EventFilter
{
public:
    CursorEventFilter(mf::DragWlSurface& surface, mf::WlDataDevice& data_device)
        : surface{surface},
          data_device{data_device}
    {}
          
    bool handle(MirEvent const& event) override
    {
        if (mir_event_get_type(&event) == mir_event_type_input)
        {
            auto const input_ev = mir_event_get_input_event(&event);
            auto const& ev_type = mir_input_event_get_type(input_ev);
            if (ev_type == mir_input_event_type_pointer)
            {
                auto const& pointer_event = mir_input_event_get_pointer_event(input_ev);

                if (mir_pointer_event_buttons(pointer_event) != mir_pointer_button_primary)
                {
                    data_device.end_drag();
                    return false;
                }

                auto const x = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x);
                auto const y = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y);

                auto const top_left = mir::geometry::Point{x, y};
                surface.move_scene_surface_to(top_left);

                // TODO - send_motion_event()

                return false;
            }
        }

        return false;
    }

private:
    mf::DragWlSurface& surface;
    mf::WlDataDevice& data_device;
};

class mf::WlDataDevice::Offer : public wayland::DataOffer
{
public:
    Offer(WlDataDevice* device, std::shared_ptr<scene::ClipboardSource> const& source);

    void accept(uint32_t serial, std::optional<std::string> const& mime_type) override
    {
        (void)serial, (void)mime_type;
    }

    void receive(std::string const& mime_type, mir::Fd fd) override;

    void finish() override
    {
    }

    void set_actions(uint32_t dnd_actions, uint32_t preferred_action) override
    {
        (void)dnd_actions, (void)preferred_action;
    }

private:
    friend mf::WlDataDevice;
    wayland::Weak<WlDataDevice> const device;
    std::shared_ptr<scene::ClipboardSource> const source;
};

mf::WlDataDevice::Offer::Offer(WlDataDevice* device, std::shared_ptr<scene::ClipboardSource> const& source) :
    mw::DataOffer(*device),
    device{device},
    source{source}
{
    device->send_data_offer_event(resource);
    for (auto const& type : source->mime_types())
    {
        send_offer_event(type);
    }
}

void mf::WlDataDevice::Offer::receive(std::string const& mime_type, mir::Fd fd)
{
    if (device && device.value().current_offer.is(*this))
    {
        source->initiate_send(mime_type, fd);
    }
}

mf::DragWlSurface::DragWlSurface(Executor& wayland_executor, WlSurface* icon)
    : NullWlSurfaceRole(icon),
      wayland_executor{wayland_executor},
      surface{icon}
{
    icon->set_role(this);
    create_scene_surface();
}

mf::DragWlSurface::~DragWlSurface()
{
    if (surface)
    {
        surface.value().clear_role();
    }

    surface_destroyed();
}

auto mf::DragWlSurface::scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>>
{
    return shared_scene_surface;
}

void mf::DragWlSurface::create_scene_surface()
{
    if (shared_scene_surface || !surface)
    {
        return;
    }

    // TODO - check surface has value

    auto spec = shell::SurfaceSpecification();
    spec.width = surface.value().buffer_size()->width;
    spec.height = surface.value().buffer_size()->height;
    spec.streams = std::vector<shell::StreamSpecification>{};
    spec.input_shape = std::vector<geom::Rectangle>{};

    // TODO - handle
    surface.value().populate_surface_data(spec.streams.value(), spec.input_shape.value(), {});

    auto const& session = surface.value().session;

    shared_scene_surface =
        session->create_surface(session, wayland::Weak<mf::WlSurface>(surface), spec, nullptr, nullptr);
}

void mf::DragWlSurface::surface_destroyed()
{
    auto const& session = surface.value().session;
    session->destroy_surface(shared_scene_surface);
}

mf::WlDataDevice::WlDataDevice(
    wl_resource* new_resource,
    Executor& wayland_executor,
    scene::Clipboard& clipboard,
    mf::WlSeat& seat,
    mi::CompositeEventFilter& composite_event_filter)
    : mw::DataDevice(new_resource, Version<3>()),
      wayland_executor{wayland_executor},
      clipboard{clipboard},
      seat{seat},
      composite_event_filter{composite_event_filter},
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
    // TODO: "The [origin surface] and client must have an active implicit grab that matches the serial"
    (void)source;
    if (!origin)
    {
        BOOST_THROW_EXCEPTION(
            mw::ProtocolError(resource, Error::role, "Origin surface does not exist."));
    }
 
    if (!icon)
    {
        // TODO - icon is allowed to be null according to the protocol, but this is an issue to be fixed in the PR
        // which implements the data transfer.
        return;
    }

    auto const icon_surface = WlSurface::from(icon.value());

    drag_surface.emplace(wayland_executor, icon_surface);

    auto const drag_event = client->event_for(serial);
    if (drag_event && drag_event.value() && mir_event_get_type(drag_event.value().get()) == mir_event_type_input)
    {
        auto const input_ev = mir_event_get_input_event(drag_event.value().get());
        auto const& ev_type = mir_input_event_get_type(input_ev);
        if (ev_type == mir_input_event_type_pointer)
        {
            auto const pointer_event = mir_input_event_get_pointer_event(input_ev);
            auto const x = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x);
            auto const y = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y);
            auto const top_left = mir::geometry::Point{x, y};

            // It is known that drag_surface will have a value from the emplace above and
            // that the scene_surface will have a value as it's set up in the DragWlSurface constructor.
            drag_surface.value().scene_surface().value()->move_to(top_left);
        }
    }
    else
    {
        return;
    }

    cursor_event_filter = std::make_shared<CursorEventFilter>(drag_surface.value(), *this);
    composite_event_filter.prepend(cursor_event_filter);
}

void mf::WlDataDevice::end_drag()
{
    // TODO - detect if on surface expecting data, then copy into it
    send_leave_event();
    cursor_event_filter.reset();
    drag_surface.reset();
    current_offer = {};
}

void mf::WlDataDevice::focus_on(WlSurface* surface)
{
    has_focus = static_cast<bool>(surface);
    paste_source_set(clipboard.paste_source());
}

void mf::WlDataDevice::paste_source_set(std::shared_ptr<scene::ClipboardSource> const& source)
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
