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
#include "mir/scene/clipboard.h"
#include "mir/wayland/weak.h"
#include "mir/executor.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;

namespace
{
class PrimarySelectionSource : public mw::PrimarySelectionSourceV1
{
private:
    class Source : public ms::ClipboardSource
    {
    public:
        Source(
            PrimarySelectionSource const* owner,
            std::shared_ptr<mir::Executor> wayland_executor,
            std::vector<std::string> types)
            : owner{std::move(owner)},
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
            wayland_executor->spawn([owner=owner, mime_type, target_fd]()
                {
                    if (owner)
                    {
                        owner.value().send_send_event(mime_type, target_fd);
                    }
                });
        }

    private:
        mw::Weak<PrimarySelectionSource const> const owner;
        std::shared_ptr<mir::Executor> const wayland_executor;
        std::vector<std::string> const types;
    };

    std::vector<std::string> mime_types;
    std::shared_ptr<mir::Executor> const wayland_executor;

public:
    PrimarySelectionSource(
        wl_resource* resource,
        std::shared_ptr<mir::Executor> wayland_executor)
        : PrimarySelectionSourceV1{resource, Version<1>()},
          wayland_executor{std::move(wayland_executor)}
    {
    }

    void offer(std::string const& mime_type) override
    {
        mime_types.push_back(mime_type);
    }

    auto make_source() const -> std::shared_ptr<ms::ClipboardSource>
    {
        return std::make_shared<Source>(this, wayland_executor, mime_types);
    }
};

class PrimarySelectionOffer : public mw::PrimarySelectionOfferV1
{
public:
    PrimarySelectionOffer(mw::PrimarySelectionDeviceV1& parent, std::shared_ptr<ms::ClipboardSource> source)
        : PrimarySelectionOfferV1{parent},
          source{std::move(source)}
    {
    }

    void receive(std::string const& mime_type, mir::Fd fd) override
    {
        source->initiate_send(mime_type, fd);
    }

    std::shared_ptr<ms::ClipboardSource> const source;
};

class PrimarySelectionDevice : public mw::PrimarySelectionDeviceV1, public mf::WlSeat::FocusListener
{
private:
    class ClipboardObserver : public ms::ClipboardObserver
    {
    public:
        ClipboardObserver(PrimarySelectionDevice* owner) : owner{owner}
        {
        }

    private:
        void paste_source_set(std::shared_ptr<ms::ClipboardSource> const& source) override
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
        wl_resource* resource,
        std::shared_ptr<ms::Clipboard> primary_selection_clipboard,
        mf::WlSeat& seat)
        : PrimarySelectionDeviceV1{resource, Version<1>()},
          clipboard_observer{std::make_shared<ClipboardObserver>(this)},
          primary_selection_clipboard{std::move(primary_selection_clipboard)},
          seat{seat}
    {
        this->primary_selection_clipboard->register_interest(clipboard_observer, mir::immediate_executor);
        seat.add_focus_listener(client, this);
    }

    ~PrimarySelectionDevice()
    {
        primary_selection_clipboard->unregister_interest(*clipboard_observer);
        if (current.source)
        {
            primary_selection_clipboard->clear_paste_source(*current.source);
        }
        seat.remove_focus_listener(client, this);
    }

private:
    void set_selection(std::optional<wl_resource*> const& source, uint32_t serial) override
    {
        (void)serial;
        auto const source_wrapper = mw::make_weak(source ?
            dynamic_cast<PrimarySelectionSource*>(mw::PrimarySelectionSourceV1::from(source.value())) :
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

    void focus_on(mf::WlSurface* surface) override
    {
        has_focus = static_cast<bool>(surface);
        paste_source_set(primary_selection_clipboard->paste_source());
    }

    void paste_source_set(std::shared_ptr<ms::ClipboardSource> const& source)
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
                auto const offer = new PrimarySelectionOffer{*this, source};
                current_offer = mw::make_weak(offer);
                send_data_offer_event(offer->resource);
                for (auto const& type : source->mime_types())
                {
                    offer->send_offer_event(type);
                }
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

    struct Selection {
        std::shared_ptr<ms::ClipboardSource> source;
        mw::Weak<PrimarySelectionSource> wrapper;
    };

    bool has_focus{false};
    Selection pending, current;
    mw::Weak<PrimarySelectionOffer> current_offer;
    std::shared_ptr<ClipboardObserver> clipboard_observer;
    std::shared_ptr<ms::Clipboard> const primary_selection_clipboard;
    mf::WlSeat& seat;
};

class PrimarySelectionManager : public mw::PrimarySelectionDeviceManagerV1
{
public:
    PrimarySelectionManager(
        wl_resource* manager,
        std::shared_ptr<mir::Executor> wayland_executor,
        std::shared_ptr<ms::Clipboard> primary_selection_clipboard)
        : PrimarySelectionDeviceManagerV1{manager, Version<1>()},
          wayland_executor{std::move(wayland_executor)},
          primary_selection_clipboard{std::move(primary_selection_clipboard)}
    {
    }

    void create_source(struct wl_resource* id) override
    {
        new PrimarySelectionSource{id, wayland_executor};
    }

    void get_device(struct wl_resource* id, struct wl_resource* seat) override
    {
        auto const wl_seat = mf::WlSeat::from(seat);
        if (!wl_seat)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error(
                "client provided incorrect seat to zwp_primary_selection_device_manager_v1.get_device"));
        }
        new PrimarySelectionDevice{id, primary_selection_clipboard, *wl_seat};
    }

    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<ms::Clipboard> const primary_selection_clipboard;
};

class PrimarySelectionGlobal : public mw::PrimarySelectionDeviceManagerV1::Global
{
public:
    PrimarySelectionGlobal(
        wl_display* display,
        std::shared_ptr<mir::Executor> wayland_executor,
        std::shared_ptr<ms::Clipboard> primary_selection_clipboard)
        : Global{display, Version<1>()},
          wayland_executor{std::move(wayland_executor)},
          primary_selection_clipboard{std::move(primary_selection_clipboard)}
    {
    }

    void bind(wl_resource* manager) override
    {
        new PrimarySelectionManager{manager, wayland_executor, primary_selection_clipboard};
    }

    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<ms::Clipboard> const primary_selection_clipboard;
};
}

auto mf::create_primary_selection_device_manager_v1(
    wl_display* display,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<ms::Clipboard> primary_selection_clipboard)
-> std::shared_ptr<mw::PrimarySelectionDeviceManagerV1::Global>
{
    return std::make_shared<PrimarySelectionGlobal>(
        display,
        std::move(wayland_executor),
        std::move(primary_selection_clipboard));
}
