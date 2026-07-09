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

#include "wayland_rs/src/ffi.rs.h"

#include <mir/executor.h>
#include <mir/scene/clipboard.h>

#include <unistd.h>

#include <stdexcept>
#include <utility>

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;
namespace ms = mir::scene;

namespace
{
auto as_primary_offer_ptr(mw::PrimarySelectionOfferV1* offer) -> std::shared_ptr<mw::PrimarySelectionOfferV1>
{
    return offer ? std::shared_ptr<mw::PrimarySelectionOfferV1>{std::shared_ptr<void>{}, offer} : nullptr;
}
}

class mf::PrimarySelectionSourceV1::Source : public ms::DataExchangeSource
{
public:
    Source(
        PrimarySelectionSourceV1* owner,
        std::shared_ptr<mir::Executor> wayland_executor,
        std::vector<std::string> types)
        : owner{owner},
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
            [owner = owner, mime_type, target_fd]
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
    auto offer_set_actions(uint32_t, uint32_t) -> uint32_t override
    {
        return 0;
    }
    void dnd_finished() override
    {
    }

    mw::Weak<PrimarySelectionSourceV1> const owner;
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::vector<std::string> const types;
};

mf::PrimarySelectionSourceV1::PrimarySelectionSourceV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::PrimarySelectionSourceV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<mir::Executor> wayland_executor)
    : mw::PrimarySelectionSourceV1{std::move(client), std::move(instance), object_id},
      wayland_executor{std::move(wayland_executor)}
{
}

auto mf::PrimarySelectionSourceV1::offer(rust::String mime_type) -> void
{
    mime_types.push_back(std::string{mime_type});
}

auto mf::PrimarySelectionSourceV1::make_source() -> std::shared_ptr<ms::DataExchangeSource>
{
    return std::make_shared<Source>(this, wayland_executor, mime_types);
}

namespace
{
class PrimarySelectionOfferV1 : public mw::PrimarySelectionOfferV1
{
public:
    PrimarySelectionOfferV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::PrimarySelectionOfferV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<ms::DataExchangeSource> source)
        : mw::PrimarySelectionOfferV1{std::move(client), std::move(instance), object_id},
          source{std::move(source)}
    {
    }

    auto receive(rust::String mime_type, int32_t fd) -> void override
    {
        source->initiate_send(std::string{mime_type}, mir::Fd{mir::IntOwnedFd{::dup(fd)}});
    }

    auto destroy() -> void override
    {
        destroy_and_delete();
    }

    std::shared_ptr<ms::DataExchangeSource> const source;
};

class PrimarySelectionDeviceV1 : public mw::PrimarySelectionDeviceV1, public mf::WlSeat::FocusListener
{
private:
    class ClipboardObserver : public ms::ClipboardObserver
    {
    public:
        explicit ClipboardObserver(PrimarySelectionDeviceV1* owner) : owner{owner}
        {
        }

    private:
        void drag_n_drop_source_set(std::shared_ptr<ms::DataExchangeSource> const&) override {}
        void drag_n_drop_source_cleared(std::shared_ptr<ms::DataExchangeSource> const&) override {}
        void end_of_dnd_gesture() override {}

        void paste_source_set(std::shared_ptr<ms::DataExchangeSource> const& source) override
        {
            if (owner)
            {
                owner.value().paste_source_set(source);
            }
        }

        mw::Weak<PrimarySelectionDeviceV1> const owner;
    };

public:
    PrimarySelectionDeviceV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::PrimarySelectionDeviceV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<ms::Clipboard> primary_selection_clipboard,
        mf::WlSeat& seat)
        : mw::PrimarySelectionDeviceV1{std::move(client), std::move(instance), object_id},
          clipboard_observer{std::make_shared<ClipboardObserver>(this)},
          primary_selection_clipboard{std::move(primary_selection_clipboard)},
          seat{seat}
    {
        this->primary_selection_clipboard->register_interest(clipboard_observer, mir::immediate_executor);
        seat.add_focus_listener(this->client.get(), this);
    }

    ~PrimarySelectionDeviceV1() override
    {
        primary_selection_clipboard->unregister_interest(*clipboard_observer);
        if (current.source)
        {
            primary_selection_clipboard->clear_paste_source(*current.source);
        }
        seat.remove_focus_listener(client.get(), this);
    }

private:
    auto destroy() -> void override
    {
        destroy_and_delete();
    }

    auto set_selection(std::optional<mw::Weak<mw::PrimarySelectionSourceV1>> const& source, uint32_t serial) -> void override
    {
        (void)serial;
        auto const source_wrapper = source ?
            mw::make_weak(mw::PrimarySelectionSourceV1::from<mf::PrimarySelectionSourceV1>(*source)) :
            mw::Weak<mf::PrimarySelectionSourceV1>{};
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

    void focus_on(mf::WlSurface* surface) override
    {
        has_focus = static_cast<bool>(surface);
        paste_source_set(primary_selection_clipboard->paste_source());
    }

    auto create_offer(std::shared_ptr<ms::DataExchangeSource> const& source) -> std::shared_ptr<PrimarySelectionOfferV1>
    {
        PrimarySelectionOfferV1* offer_ptr = nullptr;
        auto offer = mw::create_zwp_primary_selection_offer_v1(
            *client->raw_client(),
            get_box()->version(),
            [&](rust::Box<mw::PrimarySelectionOfferV1Middleware> box, uint32_t id)
                -> std::shared_ptr<mw::PrimarySelectionOfferV1>
            {
                auto created = std::make_shared<PrimarySelectionOfferV1>(client, std::move(box), id, source);
                offer_ptr = created.get();
                return created;
            });

        if (!offer_ptr)
        {
            throw std::logic_error{"failed to create zwp_primary_selection_offer_v1"};
        }

        send_data_offer_event(offer->get_box());
        for (auto const& type : source->mime_types())
        {
            offer_ptr->send_offer_event(type);
        }

        return std::static_pointer_cast<PrimarySelectionOfferV1>(offer);
    }

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
                auto const offer = create_offer(source);
                current_offer = mw::make_weak(offer.get());
                send_selection_event(as_primary_offer_ptr(offer.get()));
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

    struct Selection
    {
        std::shared_ptr<ms::DataExchangeSource> source;
        mw::Weak<mf::PrimarySelectionSourceV1> wrapper;
    };

    bool has_focus{false};
    Selection pending, current;
    mw::Weak<PrimarySelectionOfferV1> current_offer;
    std::shared_ptr<ClipboardObserver> clipboard_observer;
    std::shared_ptr<ms::Clipboard> const primary_selection_clipboard;
    mf::WlSeat& seat;
};

class PrimarySelectionDeviceManagerV1 : public mw::PrimarySelectionDeviceManagerV1
{
public:
    PrimarySelectionDeviceManagerV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::PrimarySelectionDeviceManagerV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<mir::Executor> wayland_executor,
        std::shared_ptr<ms::Clipboard> primary_selection_clipboard)
        : mw::PrimarySelectionDeviceManagerV1{std::move(client), std::move(instance), object_id},
          wayland_executor{std::move(wayland_executor)},
          primary_selection_clipboard{std::move(primary_selection_clipboard)}
    {
    }

    using mw::PrimarySelectionDeviceManagerV1::get_device;

private:
    auto destroy() -> void override
    {
        destroy_and_delete();
    }

    auto create_source(
        rust::Box<mw::PrimarySelectionSourceV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::PrimarySelectionSourceV1> override
    {
        return std::make_shared<mf::PrimarySelectionSourceV1>(
            client,
            std::move(child_instance),
            child_object_id,
            wayland_executor);
    }

    auto get_device(
        mw::Weak<mw::Seat> const& seat,
        rust::Box<mw::PrimarySelectionDeviceV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::PrimarySelectionDeviceV1> override
    {
        auto const wl_seat = mf::WlSeat::from(seat);
        if (!wl_seat)
        {
            throw std::logic_error{
                "client provided incorrect seat to zwp_primary_selection_device_manager_v1.get_device"};
        }

        return std::make_shared<PrimarySelectionDeviceV1>(
            client,
            std::move(child_instance),
            child_object_id,
            primary_selection_clipboard,
            *wl_seat);
    }

    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<ms::Clipboard> const primary_selection_clipboard;
};
}

auto mf::create_zwp_primary_selection_device_manager_v1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::PrimarySelectionDeviceManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<ms::Clipboard> primary_selection_clipboard)
-> std::shared_ptr<mw::PrimarySelectionDeviceManagerV1>
{
    return std::make_shared<PrimarySelectionDeviceManagerV1>(
        std::move(client),
        std::move(instance),
        object_id,
        std::move(wayland_executor),
        std::move(primary_selection_clipboard));
}
