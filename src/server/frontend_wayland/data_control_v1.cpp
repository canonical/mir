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
#include "primary_selection_v1.h"
#include "wl_data_source.h"
#include "wl_seat.h"

#include "mir/scene/clipboard.h"
#include "mir/scene/data_exchange.h"
#include "mir/synchronised.h"
#include "mir/wayland/protocol_error.h"

#include "mir/log.h"

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
    DataControlSourceV1(struct wl_resource* id) :
        wayland::DataControlSourceV1{id, Version<1>{}}
    {
    }

    static auto from(struct wl_resource* id) -> mf::DataControlSourceV1*
    {
        return dynamic_cast<mf::DataControlSourceV1*>(wayland::DataControlSourceV1::from(id));
    }

    auto mime_types() const -> std::vector<std::string> const&
    {
        return mime_types_;
    }

    auto try_finalize() -> bool
    {
        if (!finalized_)
        {
            finalized_ = true;
            return true;
        }

        return false;
    }

private:
    void offer(std::string const& mime_type) override
    {
        if (finalized_)
            BOOST_THROW_EXCEPTION(
                wayland::ProtocolError(
                    resource,
                    wayland::DataControlSourceV1::Error::invalid_offer,
                    "Cannot add MIME types after source is set"));
        mime_types_.push_back(mime_type);
    }

    std::vector<std::string> mime_types_;
    bool finalized_{false};
};

class DataExchangeSource : public ms::DataExchangeSource
{
public:
    DataExchangeSource(wayland::Weak<DataControlSourceV1> source, WlSeat const* const seat) :
        source{source},
        seat{seat}
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
    WlSeat const* const seat;
};

struct DataControlStateV1
{
    DataControlStateV1(
        std::shared_ptr<ms::Clipboard> const& clipboard, std::shared_ptr<ms::Clipboard> const& primary_clipboard) :
        clipboard{clipboard},
        primary_clipboard{primary_clipboard}
    {
    }

    std::shared_ptr<ms::Clipboard> const clipboard;
    std::shared_ptr<ms::Clipboard> const primary_clipboard;
};

class DataControlDeviceV1 : public wayland::DataControlDeviceV1
{
public:
    // Impl seperated out because it depends on `DataControlOfferV1`, which itself depends on this class
    DataControlDeviceV1(struct wl_resource* id, std::shared_ptr<DataControlStateV1> const& state, WlSeat const* seat);

    ~DataControlDeviceV1() override
    {
        shared_state->clipboard->unregister_interest(*clipboard_observer);
        shared_state->primary_clipboard->unregister_interest(*primary_clipboard_observer);
        send_finished_event();
    }

    void receive_from_current_source(std::string const& mime, mir::Fd fd, bool is_primary)
    {
        auto send_initiated = false;
        auto const target_clipboard = is_primary ? shared_state->primary_clipboard : shared_state->clipboard;

        target_clipboard->do_with_paste_source(
            [mime, fd, &send_initiated](auto const& paste_source)
            {
                if (!paste_source)
                    return;
                paste_source->initiate_send(mime, fd);
                send_initiated = true;
            });

        if(!send_initiated)
            mir::log_warning("Attempt to receive from invalid source");
    }

private:
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

    auto data_exchange_source_from_source(std::optional<struct wl_resource*> const& data_control_source)
        -> std::shared_ptr<ms::DataExchangeSource>
    {
        if (auto source = mf::DataControlSourceV1::from(*data_control_source))
        {
            if (!source)
                return nullptr;

            if (!source->try_finalize())
                BOOST_THROW_EXCEPTION(
                    wayland::ProtocolError(
                        resource, wayland::DataControlDeviceV1::Error::used_source, "Source already used"));

            return std::make_shared<DataExchangeSource>(wayland::Weak{source}, seat);
        }

        if (auto wl_data_source = mf::WlDataSource::from(*data_control_source))
        {
            return wl_data_source->make_source();
        }

        if (auto primary_data_source = mf::PrimarySelectionSource::from(*data_control_source))
        {
            return primary_data_source->make_source();
        }

        return nullptr;
    }

    void set_selection(std::optional<struct wl_resource*> const& data_control_source) override
    {
        if (data_control_source)
        {
            auto data_exchange_source = data_exchange_source_from_source(data_control_source);
            if(!data_control_source)
            {
                mir::log_warning("Attempt to call `set_selection` with a null source or an unhandled source type");
                return;
            }

            our_source = data_exchange_source;
            shared_state->clipboard->set_paste_source(data_exchange_source);
        }
        else
        {
            shared_state->clipboard->clear_paste_source();
        }
    }

    void set_primary_selection(std::optional<struct wl_resource*> const& data_control_source) override
    {
        if (data_control_source)
        {
            auto data_exchange_source = data_exchange_source_from_source(data_control_source);
            if(!data_control_source)
            {
                mir::log_warning(
                    "Attempt to call `set_primary_selection` with a null source or an unhandled source type");
                return;
            }
            our_primary_source = data_exchange_source;
            shared_state->primary_clipboard->set_paste_source(data_exchange_source);
        }
        else
        {
            shared_state->primary_clipboard->clear_paste_source();
        }
    }

    // Depends on `DataControlOffer`
    void on_clipboard_set(std::shared_ptr<ms::DataExchangeSource> const& source, bool is_primary);

    std::shared_ptr<DataControlStateV1> const shared_state;

    WlSeat const* const seat;
    std::shared_ptr<ms::ClipboardObserver> const clipboard_observer;
    std::shared_ptr<ms::ClipboardObserver> const primary_clipboard_observer;
    std::shared_ptr<ms::DataExchangeSource> our_source;
    std::shared_ptr<ms::DataExchangeSource> our_primary_source;
};

struct DataControlOfferV1 : public wayland::DataControlOfferV1
{
    DataControlOfferV1(
        wayland::Weak<mf::DataControlDeviceV1> parent, std::vector<std::string> const& mime_types, bool is_primary) :
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

    wayland::Weak<DataControlDeviceV1> const parent;
    std::vector<std::string> const mime_types_;
    bool const is_primary;
};

class DataControlManagerV1 : public wayland::DataControlManagerV1::Global
{
public:
    DataControlManagerV1(
        struct wl_display* display,
        std::shared_ptr<ms::Clipboard> const& clipboard,
        std::shared_ptr<ms::Clipboard> const& primary_clipboard) :
        wayland::DataControlManagerV1::Global(display, Version<1>{}),
        state{std::make_shared<DataControlStateV1>(clipboard, primary_clipboard)}
    {
    }

private:
    class Instance : public wayland::DataControlManagerV1
    {
    public:
        Instance(struct wl_resource* id, std::shared_ptr<DataControlStateV1> const& state) :
            wayland::DataControlManagerV1(id, Version<1>{}),
            state{state}
        {
        }

    private:
        void create_data_source(struct wl_resource* id) override
        {
            auto const source = new DataControlSourceV1{id};

            source->add_destroy_listener(
                [this, source]
                {
                    for (auto const& clp : {state->clipboard, state->primary_clipboard})
                    {
                        if (auto const dxs = std::dynamic_pointer_cast<DataExchangeSource>(clp->paste_source());
                            dxs && dxs->source.is(*source))
                        {
                            clp->clear_paste_source();
                        }
                    }
                });
        }

        void get_data_device(struct wl_resource* id, struct wl_resource* seat) override
        {
            new DataControlDeviceV1(id, state, WlSeat::from(seat));
        }

        std::shared_ptr<DataControlStateV1> const state;
    };

    void bind(wl_resource* id) override
    {
        new Instance{id, state};
    }

    std::shared_ptr<DataControlStateV1> const state;
};
}
}

mf::DataControlDeviceV1::DataControlDeviceV1(
    struct wl_resource* id, std::shared_ptr<DataControlStateV1> const& state, WlSeat const* seat) :
    wayland::DataControlDeviceV1(id, Version<1>{}),
    shared_state{state},
    seat{seat},
    clipboard_observer{std::make_shared<ClipboardObserver>([this](auto source) { on_clipboard_set(source, false); })},
    primary_clipboard_observer{
        std::make_shared<ClipboardObserver>([this](auto source) { on_clipboard_set(source, true); })}
{
    state->clipboard->register_interest(clipboard_observer);
    state->primary_clipboard->register_interest(primary_clipboard_observer);
}

void mf::DataControlDeviceV1::on_clipboard_set(std::shared_ptr<ms::DataExchangeSource> const& source, bool is_primary)
{
    // If the source comes from ext-data-control, need to do a bunch of checks first
    if (auto data_exchange_source = std::dynamic_pointer_cast<mf::DataExchangeSource>(source))
    {
        // If the notification comes from a different seat, ignore it
        if (data_exchange_source->seat != this->seat)
            return;
    }

    if ((is_primary && source == our_primary_source) || (!is_primary && source == our_source))
        return;

    auto new_offer = new DataControlOfferV1(wayland::Weak{this}, source->mime_types(), is_primary);

    // No need to lock, we already lock at the start of `set_selection` and `set_primary_selection`
    if (is_primary)
        send_primary_selection_event({new_offer->resource});
    else
        send_selection_event({new_offer->resource});
}

auto mf::create_data_control_manager_v1(
    struct wl_display* display,
    std::shared_ptr<ms::Clipboard> const& clipboard,
    std::shared_ptr<ms::Clipboard> const& primary_clipboard) -> std::shared_ptr<wayland::DataControlManagerV1::Global>
{
    return std::make_shared<DataControlManagerV1>(display, clipboard, primary_clipboard);
}
