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
#include "protocol_error.h"
#include "wl_seat.h"

#include <mir/scene/clipboard.h>
#include <mir/scene/data_exchange.h>
#include <mir/synchronised.h>

#include <mir/log.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;

namespace mir
{
namespace frontend
{

class DataControlDeviceV1;
class DataControlSourceV1 : public wayland_rs::ExtDataControlSourceV1Impl
{
public:
    DataControlSourceV1()
    {
    }

    static auto from(wayland_rs::ExtDataControlSourceV1Impl* id) -> mf::DataControlSourceV1*
    {
        return dynamic_cast<mf::DataControlSourceV1*>(id);
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
    void offer(rust::String mime_type) override
    {
        if (finalized_)
            BOOST_THROW_EXCEPTION(
                wayland_rs::ProtocolError(
                    object_id(),
                    wayland_rs::ExtDataControlSourceV1Impl::Error::invalid_offer,
                    "Cannot add MIME types after source is set"));
        mime_types_.push_back(mime_type.c_str());
    }

    std::vector<std::string> mime_types_;
    bool finalized_{false};
};

class DataExchangeSource : public ms::DataExchangeSource
{
public:
    DataExchangeSource(wayland_rs::Weak<DataControlSourceV1> source, wayland_rs::WlSeatImpl const* seat) :
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

    wayland_rs::Weak<DataControlSourceV1> const source;
    wayland_rs::WlSeatImpl const* seat;
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

class DataControlDeviceV1 : public wayland_rs::ExtDataControlDeviceV1Impl, public std::enable_shared_from_this<DataControlDeviceV1>
{
public:
    // Impl seperated out because it depends on `DataControlOfferV1`, which itself depends on this class
    DataControlDeviceV1(std::shared_ptr<DataControlStateV1> const& state, wayland_rs::WlSeatImpl const* seat);

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

        if (!send_initiated)
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

    auto data_exchange_source_from_source(wayland_rs::Weak<wayland_rs::ExtDataControlSourceV1Impl> const&)
        -> std::shared_ptr<ms::DataExchangeSource>
    {
        // TODO:
        // if (auto source = mf::DataControlSourceV1::from(&data_control_source.value()))
        // {
        //     if (!source)
        //         return nullptr;
        //
        //     if (!source->try_finalize())
        //         BOOST_THROW_EXCEPTION(
        //             wayland_rs::ProtocolError(
        //                 object_id(), wayland_rs::ExtDataControlDeviceV1Impl::Error::used_source, "Source already used"));
        //
        //     return std::make_shared<DataExchangeSource>(data_control_source, seat);
        // }
        //
        // if (auto wl_data_source = mf::WlDataSource::from(data_control_source))
        // {
        //     return wl_data_source->make_source();
        // }
        //
        // if (auto primary_data_source = mf::PrimarySelectionSource::from(data_control_source))
        // {
        //     return primary_data_source->make_source();
        // }

        return nullptr;
    }

    auto set_selection(wayland_rs::Weak<wayland_rs::ExtDataControlSourceV1Impl> const& data_control_source, bool has_source) -> void override
    {
        if (has_source)
        {
            auto data_exchange_source = data_exchange_source_from_source(data_control_source);
            if (!data_control_source)
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

    auto set_primary_selection(wayland_rs::Weak<wayland_rs::ExtDataControlSourceV1Impl> const& data_control_source, bool) -> void override
    {
        if (data_control_source)
        {
            auto data_exchange_source = data_exchange_source_from_source(data_control_source);
            if (!data_control_source)
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

    wayland_rs::WlSeatImpl const* const seat;
    std::shared_ptr<ms::ClipboardObserver> const clipboard_observer;
    std::shared_ptr<ms::ClipboardObserver> const primary_clipboard_observer;
    std::shared_ptr<ms::DataExchangeSource> our_source;
    std::shared_ptr<ms::DataExchangeSource> our_primary_source;
};

struct DataControlOfferV1 : public wayland_rs::ExtDataControlOfferV1Impl, public std::enable_shared_from_this<DataControlDeviceV1>
{
    DataControlOfferV1(
        wayland_rs::Weak<mf::DataControlDeviceV1> parent, std::vector<std::string> const& mime_types, bool is_primary) :
        parent{parent},
        mime_types_{mime_types},
        is_primary{is_primary}
    {
    }

    auto associate(rust::Box<wayland_rs::ExtDataControlOfferV1Ext> instance, uint32_t object_id) -> void override
    {
        parent.value().send_data_offer_event(instance);
        for (auto const& mime : mime_types_)
        {
            send_offer_event(mime);
        }
        ExtDataControlOfferV1Impl::associate(std::move(instance), object_id);
    }

    auto receive(rust::String mime, int32_t fd) -> void override
    {
        parent.value().receive_from_current_source(mime.c_str(), mir::Fd(fd), is_primary);
    }

    wayland_rs::Weak<DataControlDeviceV1> const parent;
    std::vector<std::string> const mime_types_;
    bool const is_primary;
};

class Instance : public wayland_rs::ExtDataControlManagerV1Impl
{
public:
    Instance(std::shared_ptr<DataControlStateV1> const& state) :
        state{state}
    {
    }

    auto create_data_source() -> std::shared_ptr<wayland_rs::ExtDataControlSourceV1Impl> override
    {
        auto const source = std::make_shared<DataControlSourceV1>();

        source->add_destroy_listener(
            [state=state, source]
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
        return source;
    }

    auto get_data_device(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat) -> std::shared_ptr<wayland_rs::ExtDataControlDeviceV1Impl> override
    {
        return std::make_shared<DataControlDeviceV1>(state, &seat.value());
    }

private:
    std::shared_ptr<DataControlStateV1> const state;
};
}
}

mf::DataControlDeviceV1::DataControlDeviceV1(
    std::shared_ptr<DataControlStateV1> const& state, wayland_rs::WlSeatImpl const* seat) :
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

    // TODO:
    // if (source)
    // {
    //     auto new_offer = std::make_shared<DataControlOfferV1>(wayland_rs::Weak{shared_from_this()}, source->mime_types(), is_primary);
    //     // No need to lock, we already lock at the start of `set_selection` and `set_primary_selection`
    //     if (is_primary)
    //         send_primary_selection_event(new_offer, true);
    //     else
    //         send_selection_event(new_offer, true);
    // }
    // else
    // {
    //     if (is_primary)
    //         send_primary_selection_event({}, false);
    //     else
    //         send_selection_event({}, false);
    // }
}

mf::DataControlManagerV1::DataControlManagerV1(
    std::shared_ptr<ms::Clipboard> const& clipboard,
    std::shared_ptr<ms::Clipboard> const& primary_clipboard)
    : state{std::make_shared<DataControlStateV1>(clipboard, primary_clipboard)}
{
}

auto mf::DataControlManagerV1::create() -> std::shared_ptr<wayland_rs::ExtDataControlManagerV1Impl>
{
    return std::make_shared<Instance>(state);
}
