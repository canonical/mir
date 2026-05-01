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

#include "primary_selection_v1.h"
#include "wl_seat.h"
#include "weak.h"
#include <mir/scene/clipboard.h>
#include <mir/executor.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;
namespace ms = mir::scene;

class mf::PrimarySelectionSource::Source : public ms::DataExchangeSource
{
public:
    Source(
        PrimarySelectionSource* owner,
        std::shared_ptr<mir::Executor> wayland_executor,
        std::vector<std::string> types) :
        owner{owner->shared_from_this()},
        wayland_executor{std::move(wayland_executor)},
        types{std::move(types)}
    {
    }

    auto mime_types() const -> std::vector<std::string> const& override
    {
        return types;
    }

    void initiate_send(std::string const& mime_type, mir::Fd const& target_fd) override
    {
        wayland_executor->spawn(
            [owner = owner, mime_type, target_fd]()
            {
                if (owner)
                {
                    owner.value().send_send_event(mime_type, target_fd);
                }
            });
    }

private:
    void cancelled() override
    {
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

    mw::Weak<PrimarySelectionSource> const owner;
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::vector<std::string> const types;
};

mf::PrimarySelectionSource::PrimarySelectionSource(
    std::shared_ptr<mir::Executor> wayland_executor) :
    wayland_executor{std::move(wayland_executor)}
{
}

auto mir::frontend::PrimarySelectionSource::from(wayland_rs::ZwpPrimarySelectionSourceV1Impl* resource) -> PrimarySelectionSource*
{
    return dynamic_cast<mf::PrimarySelectionSource*>(resource);
}

void mf::PrimarySelectionSource::offer(rust::String mime_type)
{
    mime_types.push_back(mime_type.c_str());
}

auto mf::PrimarySelectionSource::make_source() -> std::shared_ptr<ms::DataExchangeSource>
{
    return std::make_shared<Source>(this, wayland_executor, mime_types);
}

namespace
{
class PrimarySelectionOffer : public mw::ZwpPrimarySelectionOfferV1Impl
{
public:
    PrimarySelectionOffer(mw::ZwpPrimarySelectionDeviceV1Impl&, std::shared_ptr<ms::DataExchangeSource> source)
        : source{std::move(source)}
    {
    }

    auto receive(rust::String mime_type, int32_t fd) -> void override
    {
        source->initiate_send(mime_type.c_str(), mir::Fd{fd});
    }

    std::shared_ptr<ms::DataExchangeSource> const source;
};

class PrimarySelectionDevice : public mw::ZwpPrimarySelectionDeviceV1Impl, public mf::WlSeatGlobal::FocusListener, std::enable_shared_from_this<PrimarySelectionDevice>
{
private:
    class ClipboardObserver : public ms::ClipboardObserver
    {
    public:
        ClipboardObserver(PrimarySelectionDevice* owner) : owner{owner->shared_from_this()}
        {
        }

    private:
        void drag_n_drop_source_set(const std::shared_ptr<ms::DataExchangeSource>&) override {}
        void drag_n_drop_source_cleared(const std::shared_ptr<ms::DataExchangeSource>&) override {}
        void end_of_dnd_gesture() override {}

        void paste_source_set(std::shared_ptr<ms::DataExchangeSource> const& source) override
        {
            if (owner)
            {
                owner.value().paste_source_set(source);
            }
        }

        mw::Weak<PrimarySelectionDevice> const owner;
    };

public:
    PrimarySelectionDevice(
        std::shared_ptr<mw::Client> const& client,
        std::shared_ptr<ms::Clipboard> primary_selection_clipboard,
        mf::WlSeatGlobal& seat)
        : client{client},
          clipboard_observer{std::make_shared<ClipboardObserver>(this)},
          primary_selection_clipboard{std::move(primary_selection_clipboard)},
          seat{seat}
    {
        this->primary_selection_clipboard->register_interest(clipboard_observer, mir::immediate_executor);
        seat.add_focus_listener(client.get(), this);
    }

    ~PrimarySelectionDevice()
    {
        primary_selection_clipboard->unregister_interest(*clipboard_observer);
        if (current.source)
        {
            primary_selection_clipboard->clear_paste_source(*current.source);
        }
        seat.remove_focus_listener(client.get(), this);
    }

    auto set_selection(mir::wayland_rs::Weak<mir::wayland_rs::ZwpPrimarySelectionSourceV1Impl> const& source, bool has_source, uint32_t serial) -> void override
    {
        (void)serial;
        auto const source_wrapper = mw::Weak(has_source ?
            dynamic_cast<mf::PrimarySelectionSource*>(&source.value())->shared_from_this() :
            nullptr);
        if (source_wrapper)
        {
            pending = {source_wrapper.value().make_source(), source_wrapper};
            primary_selection_clipboard->set_paste_source(pending.source);
        }
        else
        {
            pending = {};
            primary_selection_clipboard->clear_paste_source();
        }
    }

    void focus_on(std::shared_ptr<mir::frontend::WlSurface> const& surface) override
    {
        has_focus = static_cast<bool>(surface);
        paste_source_set(primary_selection_clipboard->paste_source());
    }

private:
    void paste_source_set(std::shared_ptr<ms::DataExchangeSource> const& source)
    {
        if (pending.source == source)
        {
            if (current.wrapper && pending.wrapper != current.wrapper)
            {
                current.wrapper.value().send_cancelled_event();
            }
            current = std::move(pending);
            pending = {};
        }
        if (source && has_focus)
        {
            if (!current_offer || current_offer.value().source != source)
            {
                // TODO: This won't wor
                auto const offer = std::make_shared<PrimarySelectionOffer>(*this, source);
                current_offer = mw::Weak(offer);
                // send_data_offer_event(offer->resource);
                for (auto const& type : source->mime_types())
                {
                    offer->send_offer_event(type);
                }
                // send_selection_event(current_offer.value().resource);
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

    struct Selection {
        std::shared_ptr<ms::DataExchangeSource> source;
        mw::Weak<mf::PrimarySelectionSource> wrapper;
    };

    bool has_focus{false};
    Selection pending, current;
    mw::Weak<PrimarySelectionOffer> current_offer;
    std::shared_ptr<mw::Client> client;
    std::shared_ptr<ClipboardObserver> clipboard_observer;
    std::shared_ptr<ms::Clipboard> const primary_selection_clipboard;
    mf::WlSeatGlobal& seat;
};
}

mf::PrimarySelectionManager::PrimarySelectionManager(
    std::shared_ptr<wayland_rs::Client> const& client,
    std::shared_ptr<mir::Executor> wayland_executor,
    std::shared_ptr<ms::Clipboard> primary_selection_clipboard)
    : client{client},
      wayland_executor{std::move(wayland_executor)},
      primary_selection_clipboard{std::move(primary_selection_clipboard)}
{
}

auto mf::PrimarySelectionManager::create_source() -> std::shared_ptr<wayland_rs::ZwpPrimarySelectionSourceV1Impl>
{
    return std::make_shared<mf::PrimarySelectionSource>(wayland_executor);
}

auto mf::PrimarySelectionManager::get_device(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat)
    -> std::shared_ptr<wayland_rs::ZwpPrimarySelectionDeviceV1Impl>
{
    auto const wl_seat = mf::WlSeatGlobal::from(&seat.value());
    if (!wl_seat)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "client provided incorrect seat to zwp_primary_selection_device_manager_v1.get_device"));
    }
    return std::make_shared<PrimarySelectionDevice>(client, primary_selection_clipboard, *wl_seat);
}
