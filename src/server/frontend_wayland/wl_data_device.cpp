/*
 * Copyright © 2018 Canonical Ltd.
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

#include "wl_data_device.h"
#include "wl_data_source.h"
#include "mir/scene/clipboard.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland;

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

class mf::WlDataDevice::Offer : public wayland::DataOffer
{
public:
    Offer(WlDataDevice* device, std::shared_ptr<scene::ClipboardSource> const& source);

    void accept(uint32_t serial, std::experimental::optional<std::string> const& mime_type) override
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
    device->send_selection_event(resource);
}

void mf::WlDataDevice::Offer::receive(std::string const& mime_type, mir::Fd fd)
{
    source->initiate_send(mime_type, fd);
}

mf::WlDataDevice::WlDataDevice(
    wl_resource* new_resource,
    Executor& wayland_executor,
    scene::Clipboard& clipboard,
    mf::WlSeat& seat)
    : mw::DataDevice(new_resource, Version<3>()),
      clipboard{clipboard},
      seat{seat},
      clipboard_observer{std::make_shared<ClipboardObserver>(this)}
{
    clipboard.register_interest(clipboard_observer, wayland_executor);
    seat.add_focus_listener(this);
    focus_on(seat.current_focused_client());
}

mf::WlDataDevice::~WlDataDevice()
{
    clipboard.unregister_interest(*clipboard_observer);
    seat.remove_focus_listener(this);
}

void mf::WlDataDevice::set_selection(std::experimental::optional<struct wl_resource*> const& source, uint32_t serial)
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

void mf::WlDataDevice::focus_on(wl_client* focus)
{
    has_focus = client == focus;
    auto const paste_source = clipboard.paste_source();
    if (has_focus && paste_source && !current_offer)
    {
        current_offer = wayland::make_weak(new Offer{this, paste_source});
    }
}

void mf::WlDataDevice::paste_source_set(std::shared_ptr<scene::ClipboardSource> const& source)
{
    if (source && has_focus)
    {
        current_offer = wayland::make_weak(new Offer{this, source});
    }
    else
    {
        current_offer = {};
        send_selection_event(std::experimental::nullopt);
    }
}
