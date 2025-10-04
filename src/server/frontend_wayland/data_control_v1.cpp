/*
 * Copyright © Canonical Ltd.
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
 */

#include "data_control_v1.h"
#include "wl_seat.h"

#include "mir/scene/clipboard.h"
#include "mir/scene/data_exchange.h"
#include "mir/wayland/protocol_error.h"

#include <cassert>

namespace mf = mir::frontend;
namespace ms = mir::scene;

namespace mir
{
namespace frontend
{

class DataControlDeviceV1;
class DataControlSourceV1 : public wayland::DataControlSourceV1
{
public:
    DataControlSourceV1(struct wl_resource* resource);

    static auto from(struct wl_resource* resource) -> mf::DataControlSourceV1*
    {
        return dynamic_cast<mf::DataControlSourceV1*>(wayland::DataControlSourceV1::from(resource));
    }

    auto mime_types() const -> std::vector<std::string> const&
    {
        return mime_types_;
    }

    void finalize()
    {
        if(finalized_)
            BOOST_THROW_EXCEPTION(
                wayland::ProtocolError(
                    resource, wayland::DataControlDeviceV1::Error::used_source, "Source already used"));

        finalized_ = true;
    }

private:
    void offer(std::string const& mime_type) override;

    std::vector<std::string> mime_types_;
    bool finalized_{false};
};

class DataExchangeSource: public ms::DataExchangeSource
{
public:
    DataExchangeSource(wayland::Weak<DataControlSourceV1> source, wayland::Weak<DataControlDeviceV1> device) :
        source{source},
        device{device}
    {
    }

    auto mime_types() const -> std::vector<std::string> const& override
    {
        return source.value().mime_types();
    }

    void initiate_send(std::string const& mime, Fd const& fd) override
    {
        source.value().send_send_event(mime, fd);
    }

    void cancelled() override
    {
        source.value().send_cancelled_event();
    }

    void dnd_drop_performed() override
    {
    }
    auto actions() -> uint32_t override
    {
        return 0;
    }
    void offer_accepted(std::optional<std::string> const&) override
    {
    }
    uint32_t offer_set_actions(uint32_t, uint32_t) override
    {
        return 0;
    }
    void dnd_finished() override
    {
    }

    wayland::Weak<DataControlSourceV1> const source;
    wayland::Weak<DataControlDeviceV1> const device;
};

class DataControlDeviceV1 : public wayland::DataControlDeviceV1
{
public:
    struct State
    {
        struct ClipboardObserver : public ms::ClipboardObserver
        {
            std::function<void(std::shared_ptr<ms::DataExchangeSource> const&)> on_source_set;

            ClipboardObserver(std::function<void(std::shared_ptr<ms::DataExchangeSource> const&)> on_source_set) :
                on_source_set{on_source_set}
            {
            }

            void paste_source_set(std::shared_ptr<ms::DataExchangeSource> const& source) override
            {
                on_source_set(source);
            }

            void drag_n_drop_source_set(std::shared_ptr<ms::DataExchangeSource> const&) override
            {
            }
            void drag_n_drop_source_cleared(std::shared_ptr<ms::DataExchangeSource> const&) override
            {
            }
            void end_of_dnd_gesture() override
            {
            }
        };

        State(std::shared_ptr<ms::Clipboard> const& clipboard, std::shared_ptr<ms::Clipboard> const& primary_clipboard);

        void on_clipboard_set(std::shared_ptr<ms::DataExchangeSource> const& source, bool is_primary);

        std::unordered_map<WlSeat const*, std::vector<wayland::Weak<DataControlDeviceV1>>> devices;

        std::shared_ptr<ms::Clipboard> const clipboard, primary_clipboard;
        std::shared_ptr<ms::ClipboardObserver> const clipboard_observer, primary_clipboard_observer;

        wayland::Weak<DataControlSourceV1> current_source, current_primary_source;
    };


    DataControlDeviceV1(struct wl_resource* resource, std::shared_ptr<State> const& state, WlSeat const* seat);

    ~DataControlDeviceV1();

    void receive_from_current_source(std::string const& mime, mir::Fd fd, bool is_primary)
    {
        if (is_primary && state->current_primary_source)
        {
            state->current_primary_source.value().send_send_event(mime, fd);
            return;
        }

        if (!is_primary && state->current_source)
        {
            state->current_source.value().send_send_event(mime, fd);
            return;
        }

        std::unreachable();
    }

private:
    void set_selection_on_clipboard(
        std::optional<struct wl_resource*> const& source, std::shared_ptr<ms::Clipboard> const& clipboard);
    void set_selection(std::optional<struct wl_resource*> const& source) override;
    void set_primary_selection(std::optional<struct wl_resource*> const& source) override;

    std::shared_ptr<State> const state;
    WlSeat const* const seat;
};

struct DataControlOfferV1 : public wayland::DataControlOfferV1
{
    DataControlOfferV1(wayland::Weak<mf::DataControlDeviceV1> parent, std::vector<std::string> const& mime_types, bool is_primary) :
        wayland::DataControlOfferV1{parent.value()},
        parent{parent},
        mime_types_{mime_types},
        is_primary{is_primary}
    {
        parent.value().send_data_offer_event(resource);
        for (auto const& mime : mime_types)
        {
            send_offer_event(mime);
        }
    }

    void receive(std::string const& mime, mir::Fd fd) override
    {
        parent.value().receive_from_current_source(mime, fd, is_primary);
    }

    wayland::Weak<DataControlDeviceV1> parent;
    std::vector<std::string> const mime_types_;
    bool const is_primary;
};

class DataControlManagerV1 : public wayland::DataControlManagerV1::Global
{
public:
    DataControlManagerV1(
        struct wl_display*,
        std::shared_ptr<ms::Clipboard> const& clipboard,
        std::shared_ptr<ms::Clipboard> const& primary_clipboard);
    ~DataControlManagerV1() = default;

private:
    class Instance : public wayland::DataControlManagerV1
    {
    public:
        Instance(struct wl_resource*, std::shared_ptr<mf::DataControlDeviceV1::State> const& state);

    private:
        void create_data_source(struct wl_resource* id) override;
        void get_data_device(struct wl_resource* id, struct wl_resource* seat) override;

        std::shared_ptr<mf::DataControlDeviceV1::State> const state;
    };

    void bind(wl_resource*) override;

    std::shared_ptr<mf::DataControlDeviceV1::State> const state;
};
}
}

mf::DataControlManagerV1::DataControlManagerV1(
    struct wl_display* display,
    std::shared_ptr<ms::Clipboard> const& clipboard,
    std::shared_ptr<ms::Clipboard> const& primary_clipboard) :
    wayland::DataControlManagerV1::Global(display, Version<1>{}),
    state{std::make_shared<DataControlDeviceV1::State>(clipboard, primary_clipboard)}
{
}

void mf::DataControlManagerV1::bind(wl_resource* resource)
{
    new Instance{resource, state};
}

mf::DataControlManagerV1::Instance::Instance(
    struct wl_resource* resource,
    std::shared_ptr<DataControlDeviceV1::State> const& state) :
    wayland::DataControlManagerV1(resource, Version<1>{}),
    state{state}

{
}

void mf::DataControlManagerV1::Instance::create_data_source(struct wl_resource* resource)
{
    // TODO free this memory? I don't think libwayland cleans this up
    new DataControlSourceV1{resource};
}

void mf::DataControlManagerV1::Instance::get_data_device(struct wl_resource* id, struct wl_resource* seat)
{
    auto const* wl_seat = WlSeat::from(seat);
    auto device = wayland::Weak<DataControlDeviceV1>(new DataControlDeviceV1(id, state, wl_seat));

    // TODO: Once a seat is destroyed, destroy all of the corresponding devices
    // (by removing the corresponding table entry)
    state->devices[wl_seat].push_back(device);

    if(state->current_source)
    {
        auto offer = new mf::DataControlOfferV1(device, state->current_source.value().mime_types(), false);
        device.value().send_selection_event({offer->resource});
    }

    if(state->current_primary_source)
    {
        auto offer = new mf::DataControlOfferV1(device, state->current_primary_source.value().mime_types(), true);
        device.value().send_primary_selection_event({offer->resource});
    }
}

void mf::DataControlDeviceV1::State::on_clipboard_set(std::shared_ptr<ms::DataExchangeSource> const& source, bool is_primary)
{
    if (current_source)
    {
        current_source.value().send_cancelled_event();
        current_source = {};
    }

    if (auto data_exchange_source = std::dynamic_pointer_cast<mf::DataExchangeSource>(source))
    {
        if (!data_exchange_source->source)
            return;
        current_source = data_exchange_source->source;

        if (!data_exchange_source->device)
            return;
        auto const* originating_client = data_exchange_source->device.value().client;

        if (auto seat_devices_iter = this->devices.find(data_exchange_source->device.value().seat);
            seat_devices_iter != this->devices.end())
        {
            auto const& seat_devices = seat_devices_iter->second;
            for (auto const& device_weak : seat_devices)
            {
                // TODO: We want to remove destroyed devices at some point
                // _after_ iterating over them. One solution could be to erase
                // all expired devices _before_ iterating
                if (!device_weak)
                    continue;

                // skip the originating client
                if (device_weak.value().client == originating_client)
                    continue;

                auto new_offer = new DataControlOfferV1(device_weak, data_exchange_source->mime_types(), is_primary);

                if(is_primary)
                    device_weak.value().send_primary_selection_event({new_offer->resource});
                else
                    device_weak.value().send_selection_event({new_offer->resource});
            }
        }
    }
}

mf::DataControlDeviceV1::State::State(
    std::shared_ptr<ms::Clipboard> const& clipboard, std::shared_ptr<ms::Clipboard> const& primary_clipboard) :
    clipboard{clipboard},
    primary_clipboard{primary_clipboard},
    clipboard_observer{std::make_shared<ClipboardObserver>([this](auto source) { on_clipboard_set(source, false); })},
    primary_clipboard_observer{
        std::make_shared<ClipboardObserver>([this](auto source) { on_clipboard_set(source, true); })}
{
    clipboard->register_interest(clipboard_observer);
    primary_clipboard->register_interest(primary_clipboard_observer);
}

mf::DataControlDeviceV1::DataControlDeviceV1(
    struct wl_resource* resource, std::shared_ptr<State> const& state, WlSeat const* seat) :
    wayland::DataControlDeviceV1(resource, Version<1>{}),
    state{state},
    seat{seat}
{
    // Tell the client about the current source, if any.
    if (state->current_source)
    {
        auto offer = new DataControlOfferV1(wayland::Weak{this}, state->current_source.value().mime_types(), false);
        send_selection_event({offer->resource});
    }
    else
    {
        send_selection_event({});
    }
}

void mf::DataControlDeviceV1::set_selection_on_clipboard(
    std::optional<struct wl_resource*> const& data_control_source, std::shared_ptr<ms::Clipboard> const& clipboard)
{
    if (data_control_source)
    {
        auto source = mf::DataControlSourceV1::from(*data_control_source);
        if (!source)
            return;

        source->finalize();

        auto const data_exchange_source =
            std::make_shared<DataExchangeSource>(wayland::Weak{source}, wayland::Weak{this});
        clipboard->set_paste_source(data_exchange_source);
    }
    else
    {
        clipboard->clear_paste_source();
    }
}

void mf::DataControlDeviceV1::set_selection(std::optional<struct wl_resource*> const& data_control_source)
{
    set_selection_on_clipboard(data_control_source, state->clipboard);
}

void mf::DataControlDeviceV1::set_primary_selection(std::optional<struct wl_resource*> const& data_control_source)
{
    set_selection_on_clipboard(data_control_source, state->primary_clipboard);
}

mf::DataControlDeviceV1::~DataControlDeviceV1()
{
    send_finished_event();
}

mf::DataControlSourceV1::DataControlSourceV1(struct wl_resource* id) :
    wayland::DataControlSourceV1{id, Version<1>{}}
{
}

void mf::DataControlSourceV1::offer(std::string const& mime_type)
{
    if (finalized_)
        BOOST_THROW_EXCEPTION(
            wayland::ProtocolError(
                resource,
                wayland::DataControlSourceV1::Error::invalid_offer,
                "Cannot add MIME types after source is set"));
    mime_types_.push_back(mime_type);
}

auto mf::create_data_control_manager_v1(
    struct wl_display* display,
    std::shared_ptr<ms::Clipboard> const& clipboard,
    std::shared_ptr<ms::Clipboard> const& primary_clipboard) -> std::shared_ptr<wayland::DataControlManagerV1::Global>
{
    return std::make_shared<DataControlManagerV1>(display, clipboard, primary_clipboard);
}
