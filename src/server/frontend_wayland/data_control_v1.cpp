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

#include "protocol_error.h"
#include "wl_seat.h"

#include "wayland_rs/src/ffi.rs.h"

#include <mir/log.h>
#include <mir/scene/clipboard.h>
#include <mir/scene/data_exchange.h>
#include <mir/synchronised.h>

#include <unistd.h>

#include <stdexcept>
#include <utility>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland;

namespace mir
{
namespace frontend
{

class ExtDataControlDeviceV1;
class ExtDataControlSourceV1 : public mw::ExtDataControlSourceV1
{
public:
    ExtDataControlSourceV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::ExtDataControlSourceV1Middleware> instance,
        uint32_t object_id)
        : mw::ExtDataControlSourceV1{std::move(client), std::move(instance), object_id}
    {
    }

    auto mime_types() const -> std::vector<std::string> const&
    {
        return mime_types_;
    }

    auto destroy() -> void override
    {
        destroy_and_delete();
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
    auto offer(rust::String mime_type) -> void override
    {
        if (finalized_)
        {
            throw mw::ProtocolError{
                object_id(),
                mw::ExtDataControlSourceV1::Error::invalid_offer,
                "Cannot add MIME types after source is set"};
        }
        mime_types_.push_back(std::string{mime_type});
    }

    std::vector<std::string> mime_types_;
    bool finalized_{false};
};

class DataExchangeSource : public ms::DataExchangeSource
{
public:
    DataExchangeSource(mw::Weak<ExtDataControlSourceV1> source, WlSeat const* const seat)
        : source{source},
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
    auto offer_set_actions(uint32_t, uint32_t) -> uint32_t override
    {
        return 0;
    }
    void dnd_finished() override
    {
    }

    mw::Weak<ExtDataControlSourceV1> const source;
    WlSeat const* const seat;
};

struct DataControlStateV1
{
    DataControlStateV1(
        std::shared_ptr<ms::Clipboard> const& clipboard,
        std::shared_ptr<ms::Clipboard> const& primary_clipboard)
        : clipboard{clipboard},
          primary_clipboard{primary_clipboard}
    {
    }

    std::shared_ptr<ms::Clipboard> const clipboard;
    std::shared_ptr<ms::Clipboard> const primary_clipboard;
};

class ExtDataControlOfferV1 : public mw::ExtDataControlOfferV1
{
public:
    ExtDataControlOfferV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::ExtDataControlOfferV1Middleware> instance,
        uint32_t object_id,
        mw::Weak<ExtDataControlDeviceV1> parent,
        std::vector<std::string> mime_types,
        bool is_primary);

    auto receive(rust::String mime, int32_t fd) -> void override;
    auto destroy() -> void override
    {
        destroy_and_delete();
    }

private:
    mw::Weak<ExtDataControlDeviceV1> const parent;
    std::vector<std::string> const mime_types_;
    bool const is_primary;
};

class ExtDataControlDeviceV1 : public mw::ExtDataControlDeviceV1
{
public:
    ExtDataControlDeviceV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::ExtDataControlDeviceV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<DataControlStateV1> const& state,
        WlSeat const* seat);

    ~ExtDataControlDeviceV1() override
    {
        shared_state->clipboard->unregister_interest(*clipboard_observer);
        shared_state->primary_clipboard->unregister_interest(*primary_clipboard_observer);
        send_finished_event();
    }

    void receive_from_current_source(std::string const& mime, mir::Fd const& fd, bool is_primary)
    {
        auto send_initiated = false;
        auto const target_clipboard = is_primary ? shared_state->primary_clipboard : shared_state->clipboard;

        target_clipboard->do_with_paste_source(
            [mime, fd, &send_initiated](auto const& paste_source)
            {
                if (!paste_source)
                {
                    return;
                }
                paste_source->initiate_send(mime, fd);
                send_initiated = true;
            });

        if (!send_initiated)
        {
            mir::log_warning("Attempt to receive from invalid source");
        }
    }

private:
    auto destroy() -> void override
    {
        destroy_and_delete();
    }

    struct ClipboardObserver : public ms::ClipboardObserver
    {
        explicit ClipboardObserver(std::function<void(std::shared_ptr<ms::DataExchangeSource> const&)> on_source_set)
            : on_source_set{std::move(on_source_set)}
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

        std::function<void(std::shared_ptr<ms::DataExchangeSource> const&)> on_source_set;
    };

    auto data_exchange_source_from_source(std::optional<mw::Weak<mw::ExtDataControlSourceV1>> const& data_control_source)
        -> std::shared_ptr<ms::DataExchangeSource>
    {
        if (!data_control_source)
        {
            return nullptr;
        }

        if (auto source = mw::ExtDataControlSourceV1::from<ExtDataControlSourceV1>(*data_control_source))
        {
            if (!source->try_finalize())
            {
                throw mw::ProtocolError{
                    object_id(), mw::ExtDataControlDeviceV1::Error::used_source, "Source already used"};
            }

            return std::make_shared<DataExchangeSource>(mw::make_weak(source), seat);
        }

        return nullptr;
    }

    auto set_selection(std::optional<mw::Weak<mw::ExtDataControlSourceV1>> const& data_control_source) -> void override
    {
        if (data_control_source)
        {
            auto data_exchange_source = data_exchange_source_from_source(data_control_source);
            if (!data_exchange_source)
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

    auto set_primary_selection(
        std::optional<mw::Weak<mw::ExtDataControlSourceV1>> const& data_control_source) -> void override
    {
        if (data_control_source)
        {
            auto data_exchange_source = data_exchange_source_from_source(data_control_source);
            if (!data_exchange_source)
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

    auto create_offer(std::shared_ptr<ms::DataExchangeSource> const& source, bool is_primary)
        -> std::shared_ptr<ExtDataControlOfferV1>
    {
        ExtDataControlOfferV1* offer_ptr = nullptr;
        auto offer = mw::create_ext_data_control_offer_v1(
            *client->raw_client(),
            get_box()->version(),
            [&](rust::Box<mw::ExtDataControlOfferV1Middleware> box, uint32_t id)
                -> std::shared_ptr<mw::ExtDataControlOfferV1>
            {
                auto created = std::make_shared<ExtDataControlOfferV1>(
                    client,
                    std::move(box),
                    id,
                    mw::make_weak(this),
                    source->mime_types(),
                    is_primary);
                offer_ptr = created.get();
                return created;
            });

        if (!offer_ptr)
        {
            throw std::logic_error{"failed to create ext_data_control_offer_v1"};
        }

        send_data_offer_event(offer->get_box());
        for (auto const& mime : source->mime_types())
        {
            offer_ptr->send_offer_event(mime);
        }

        return std::static_pointer_cast<ExtDataControlOfferV1>(offer);
    }

    void on_clipboard_set(std::shared_ptr<ms::DataExchangeSource> const& source, bool is_primary);

    std::shared_ptr<DataControlStateV1> const shared_state;

    WlSeat const* const seat;
    std::shared_ptr<ms::ClipboardObserver> const clipboard_observer;
    std::shared_ptr<ms::ClipboardObserver> const primary_clipboard_observer;
    std::shared_ptr<ms::DataExchangeSource> our_source;
    std::shared_ptr<ms::DataExchangeSource> our_primary_source;
};

class ExtDataControlManagerV1 : public mw::ExtDataControlManagerV1
{
public:
    ExtDataControlManagerV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::ExtDataControlManagerV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<ms::Clipboard> const& clipboard,
        std::shared_ptr<ms::Clipboard> const& primary_clipboard)
        : mw::ExtDataControlManagerV1{std::move(client), std::move(instance), object_id},
          state{std::make_shared<DataControlStateV1>(clipboard, primary_clipboard)}
    {
    }

    using mw::ExtDataControlManagerV1::get_data_device;

private:
    auto destroy() -> void override
    {
        destroy_and_delete();
    }

    auto create_data_source(
        rust::Box<mw::ExtDataControlSourceV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::ExtDataControlSourceV1> override
    {
        auto source = std::make_shared<ExtDataControlSourceV1>(client, std::move(child_instance), child_object_id);
        auto source_ptr = source.get();

        source->add_destroy_listener(
            [state=state, source_ptr]
            {
                for (auto const& clp : {state->clipboard, state->primary_clipboard})
                {
                    if (auto const dxs = std::dynamic_pointer_cast<DataExchangeSource>(clp->paste_source());
                        dxs && dxs->source.is(*source_ptr))
                    {
                        clp->clear_paste_source();
                    }
                }
            });

        return source;
    }

    auto get_data_device(
        mw::Weak<mw::Seat> const& seat,
        rust::Box<mw::ExtDataControlDeviceV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::ExtDataControlDeviceV1> override
    {
        auto const wl_seat = WlSeat::from(seat);
        if (!wl_seat)
        {
            throw std::logic_error{"client provided incorrect seat to ext_data_control_manager_v1.get_data_device"};
        }

        return std::make_shared<ExtDataControlDeviceV1>(
            client,
            std::move(child_instance),
            child_object_id,
            state,
            wl_seat);
    }

    std::shared_ptr<DataControlStateV1> const state;
};

}
}

mf::ExtDataControlOfferV1::ExtDataControlOfferV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::ExtDataControlOfferV1Middleware> instance,
    uint32_t object_id,
    mw::Weak<ExtDataControlDeviceV1> parent,
    std::vector<std::string> mime_types,
    bool is_primary)
    : mw::ExtDataControlOfferV1{std::move(client), std::move(instance), object_id},
      parent{parent},
      mime_types_{std::move(mime_types)},
      is_primary{is_primary}
{
}

auto mf::ExtDataControlOfferV1::receive(rust::String mime, int32_t fd) -> void
{
    parent.value().receive_from_current_source(std::string{mime}, mir::Fd{::dup(fd)}, is_primary);
}

mf::ExtDataControlDeviceV1::ExtDataControlDeviceV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::ExtDataControlDeviceV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<DataControlStateV1> const& state,
    WlSeat const* seat)
    : mw::ExtDataControlDeviceV1{std::move(client), std::move(instance), object_id},
      shared_state{state},
      seat{seat},
      clipboard_observer{std::make_shared<ClipboardObserver>([this](auto source) { on_clipboard_set(source, false); })},
      primary_clipboard_observer{
          std::make_shared<ClipboardObserver>([this](auto source) { on_clipboard_set(source, true); })}
{
    state->clipboard->register_interest(clipboard_observer);
    state->primary_clipboard->register_interest(primary_clipboard_observer);
}

void mf::ExtDataControlDeviceV1::on_clipboard_set(std::shared_ptr<ms::DataExchangeSource> const& source, bool is_primary)
{
    // If the source comes from ext-data-control, need to do a bunch of checks first
    if (auto data_exchange_source = std::dynamic_pointer_cast<mf::DataExchangeSource>(source))
    {
        // If the notification comes from a different seat, ignore it
        if (data_exchange_source->seat != this->seat)
        {
            return;
        }
    }

    if ((is_primary && source == our_primary_source) || (!is_primary && source == our_source))
    {
        return;
    }

    if (source)
    {
        auto const new_offer = create_offer(source, is_primary);
        // No need to lock, we already lock at the start of `set_selection` and `set_primary_selection`
        if (is_primary)
        {
            send_primary_selection_event(new_offer);
        }
        else
        {
            send_selection_event(new_offer);
        }
    }
    else
    {
        if (is_primary)
        {
            send_primary_selection_event({});
        }
        else
        {
            send_selection_event({});
        }
    }
}

auto mf::create_ext_data_control_manager_v1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::ExtDataControlManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<ms::Clipboard> const& clipboard,
    std::shared_ptr<ms::Clipboard> const& primary_clipboard) -> std::shared_ptr<mw::ExtDataControlManagerV1>
{
    return std::make_shared<ExtDataControlManagerV1>(
        std::move(client),
        std::move(instance),
        object_id,
        clipboard,
        primary_clipboard);
}
