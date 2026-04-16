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

#include <mir/scene/clipboard.h>
#include <mir/scene/data_exchange.h>
#include <mir/synchronised.h>
#include <mir/wayland/protocol_error.h>

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
    DataControlSourceV1() = default;

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

    void offer(rust::String mime_type) override
    {
        if (finalized_)
            BOOST_THROW_EXCEPTION(
                wayland_rs::ProtocolError(
                    wayland_rs::ExtDataControlSourceV1Impl::Error::invalid_offer,
                    "Cannot add MIME types after source is set"));
        mime_types_.push_back(mime_type.c_str());
    }

    void destroy() override
    {
        for (auto const& clp : {state->clipboard, state->primary_clipboard})
        {
            if (auto const dxs = std::dynamic_pointer_cast<DataExchangeSource>(clp->paste_source());
                dxs && dxs->source.is(*source))
            {
                clp->clear_paste_source();
            }
        }
    }

private:
    std::vector<std::string> mime_types_;
    bool finalized_{false};
};

class DataExchangeSource : public ms::DataExchangeSource
{
public:
    DataExchangeSource(std::shared_ptr<DataControlSourceV1> const& source, std::shared_ptr<wayland_rs::WlSeatImpl> const& seat) :
        source{source},
        seat{seat}
    {
    }

    auto mime_types() const -> std::vector<std::string> const& override
    {
        if (auto const locked = source.lock())
            return locked->mime_types();

        return {};
    }

    void initiate_send(std::string const& mime, Fd const& fd) override
    {
        if (auto const locked = source.lock())
            locked->send_send_event(mime, fd);
    }

    void cancelled() override
    {
        if (auto const locked = source.lock())
            locked->send_cancelled_event();
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

    std::weak_ptr<DataControlSourceV1> const source;
    std::weak_ptr<wayland_rs::WlSeatImpl> const seat;
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

class DataControlDeviceV1 : public wayland_rs::ExtDataControlDeviceV1Impl
{
public:
    // Impl seperated out because it depends on `DataControlOfferV1`, which itself depends on this class
    DataControlDeviceV1(std::shared_ptr<DataControlStateV1> const& state, std::shared_ptr<wayland_rs::WlSeatImpl> const& seat);

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

    auto data_exchange_source_from_source(std::shared_ptr<wayland_rs::ExtDataControlSourceV1Impl> const& source)
        -> std::shared_ptr<ms::DataExchangeSource>
    {
        if (!source)
            return nullptr;

        auto const source_v1 = std::dynamic_pointer_cast<DataControlSourceV1>(source);
        if (!source_v1->try_finalize())
        {
            BOOST_THROW_EXCEPTION(
                wayland_rs::ProtocolError(
                    wayland_rs::ExtDataControlDeviceV1Impl::Error::used_source, "Source already used"));

            return std::make_shared<DataExchangeSource>(source_v1, seat.lock());
        }

        if (auto wl_data_source = mf::WlDataSource::from(data_control_source))
        {
            return wl_data_source->make_source();
        }

        if (auto primary_data_source = mf::PrimarySelectionSource::from(data_control_source))
        {
            return primary_data_source->make_source();
        }

        return nullptr;
    }

    void set_selection(std::shared_ptr<wayland_rs::ExtDataControlSourceV1Impl> const& source, bool _has_source) override
    {
        if (source)
        {
            auto data_exchange_source = data_exchange_source_from_source(*data_control_source);
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

    void set_primary_selection(std::shared_ptr<wayland_rs::ExtDataControlSourceV1Impl> const& source, bool _has_source) override
    {
        if (source)
        {
            auto data_exchange_source = data_exchange_source_from_source(*source);
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

    std::weak_ptr<wayland_rs::WlSeatImpl> const seat;
    std::shared_ptr<ms::ClipboardObserver> const clipboard_observer;
    std::shared_ptr<ms::ClipboardObserver> const primary_clipboard_observer;
    std::shared_ptr<ms::DataExchangeSource> our_source;
    std::shared_ptr<ms::DataExchangeSource> our_primary_source;
};

struct DataControlOfferV1 : public wayland_rs::ExtDataControlOfferV1Impl
{
    DataControlOfferV1(
        mf::DataControlDeviceV1* parent, std::vector<std::string> const& mime_types, bool is_primary) :
        parent{parent},
        mime_types_{mime_types},
        is_primary{is_primary}
    {
    }

    auto associate(rust::Box<wayland_rs::ExtDataControlOfferV1Ext> instance) -> void override
    {
        ExtDataControlOfferV1Impl::associate(std::move(instance));
        parent->send_data_offer_event(get_box());
        for (auto const& mime : mime_types_)
        {
            send_offer_event(mime);
        }
    }

    void receive(rust::String mime, int fd) override
    {
        parent->receive_from_current_source(mime.c_str(), mir::Fd{fd}, is_primary);
    }

    DataControlDeviceV1* parent;
    std::vector<std::string> const mime_types_;
    bool const is_primary;
};

class DataControlManagerV1 : public wayland_rs::ExtDataControlManagerV1Impl
{
public:
    DataControlManagerV1(
        std::shared_ptr<ms::Clipboard> const& clipboard,
        std::shared_ptr<ms::Clipboard> const& primary_clipboard) :
        state{std::make_shared<DataControlStateV1>(clipboard, primary_clipboard)}
    {
    }

    auto create_data_source() -> std::shared_ptr<wayland_rs::ExtDataControlSourceV1Impl> override
    {
        auto const source = std::make_shared<DataControlSourceV1>();
        return source;
    }

    auto get_data_device(std::shared_ptr<wayland_rs::WlSeatImpl> const& seat) -> std::shared_ptr<wayland_rs::ExtDataControlDeviceV1Impl> override
    {
        return std::make_shared<DataControlDeviceV1>(state, seat);
    }

    std::shared_ptr<DataControlStateV1> const state;
};
}
}

mf::DataControlDeviceV1::DataControlDeviceV1(
    std::shared_ptr<DataControlStateV1> const& state, std::shared_ptr<wayland_rs::WlSeatImpl> const& seat) :
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
        if (data_exchange_source->seat.lock() != this->seat.lock())
            return;
    }

    if ((is_primary && source == our_primary_source) || (!is_primary && source == our_source))
        return;

    if (source)
    {
        auto new_offer = std::make_shared<DataControlOfferV1>(this, source->mime_types(), is_primary);
        // No need to lock, we already lock at the start of `set_selection` and `set_primary_selection`
        if (is_primary)
            send_primary_selection_event(new_offer, true);
        else
            send_selection_event(new_offer, true);
    }
    else
    {
        if (is_primary)
            send_primary_selection_event(nullptr, false);
        else
            send_selection_event(nullptr, false);
    }
}

auto mf::create_data_control_manager_v1(
    std::shared_ptr<ms::Clipboard> const& clipboard,
    std::shared_ptr<ms::Clipboard> const& primary_clipboard) -> std::shared_ptr<wayland_rs::ExtDataControlManagerV1Impl>
{
    return std::make_shared<DataControlManagerV1>(clipboard, primary_clipboard);
}
