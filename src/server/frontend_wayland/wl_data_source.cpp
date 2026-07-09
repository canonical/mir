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

#include "wl_data_source.h"

#include "protocol_error.h"
#include "wl_data_device_manager.h"

#include <mir/executor.h>
#include <mir/scene/clipboard.h>

#include <utility>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland_rs;

class mf::WlDataSource::ClipboardObserver : public ms::ClipboardObserver
{
public:
    explicit ClipboardObserver(WlDataSource* owner) : owner{owner}
    {
    }

private:
    void paste_source_set(std::shared_ptr<ms::DataExchangeSource> const& source) override
    {
        if (owner)
        {
            owner.value().paste_source_set(source);
        }
    }

    void drag_n_drop_source_set(std::shared_ptr<ms::DataExchangeSource> const& source) override
    {
        if (owner)
        {
            owner.value().drag_n_drop_source_set(source);
        }
    }

    void drag_n_drop_source_cleared(std::shared_ptr<ms::DataExchangeSource> const&) override
    {
    }

    void end_of_dnd_gesture() override
    {
        if (owner)
        {
            if (auto const s = owner.value().dnd_source.lock())
            {
                // Relies on WlDataSource::Source::cancelled() doing nothing if performed or cancelled already happened
                s->cancelled();
            }
        }
    }

    mw::Weak<WlDataSource> const owner;
};

class mf::WlDataSource::Source : public ms::DataExchangeSource
{
public:
    explicit Source(WlDataSource& wl_data_source)
        : wayland_executor{wl_data_source.wayland_executor},
          wl_data_source{&wl_data_source},
          types{wl_data_source.mime_types}
    {
    }

    auto mime_types() const -> std::vector<std::string> const& override
    {
        return types;
    }

    void initiate_send(std::string const& mime_type, Fd const& target_fd) override
    {
        wayland_executor->spawn([wl_data_source=wl_data_source, mime_type, target_fd]
            {
                if (wl_data_source)
                {
                    wl_data_source.value().send_send_event(mime_type, target_fd);
                }
            });
    }

    void cancelled() override
    {
        if (wl_data_source && !performed_or_cancelled)
        {
            wl_data_source.value().send_cancelled_event();
            performed_or_cancelled = true;
        }
    }

    void dnd_drop_performed() override
    {
        if (wl_data_source && !performed_or_cancelled)
        {
            wl_data_source.value().send_dnd_drop_performed_event_if_supported();
            performed_or_cancelled = true;
        }
    }

    auto actions() -> uint32_t override
    {
        if (wl_data_source)
        {
            return wl_data_source.value().dnd_actions;
        }
        else
        {
            return 0;
        }
    }

    void offer_accepted(std::optional<std::string> const& mime_type) override
    {
        if (wl_data_source)
        {
            wl_data_source.value().send_target_event(mime_type);
        }
    }

    auto offer_set_actions(uint32_t dnd_actions, uint32_t preferred_action) -> uint32_t override
    {
        if (wl_data_source)
        {
            return wl_data_source.value().drag_n_drop_set_actions(dnd_actions, preferred_action);
        }
        else
        {
            return mw::DataDeviceManager::DndAction::none;
        }
    }

    void dnd_finished() override
    {
        if (wl_data_source)
        {
            wl_data_source.value().send_dnd_finished_event_if_supported();
        }
    }

private:
    std::shared_ptr<Executor> const wayland_executor;
    mw::Weak<WlDataSource> const wl_data_source;
    std::vector<std::string> const types;
    bool performed_or_cancelled{false};
};

mf::WlDataSource::WlDataSource(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::DataSourceMiddleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> const& wayland_executor,
    scene::Clipboard& clipboard)
    : mw::DataSource{std::move(client), std::move(instance), object_id},
      wayland_executor{wayland_executor},
      clipboard{clipboard},
      clipboard_observer{std::make_shared<ClipboardObserver>(this)}
{
    clipboard.register_interest(clipboard_observer, *wayland_executor);
}

mf::WlDataSource::~WlDataSource()
{
    clipboard.unregister_interest(*clipboard_observer);
    if (auto const source = paste_source.lock())
    {
        clipboard.clear_paste_source(*source);
    }
    if (auto const source = dnd_source.lock())
    {
        clipboard.clear_drag_n_drop_source(source);
    }
}

void mf::WlDataSource::set_clipboard_paste_source()
{
    if (clipboards_paste_source_is_ours)
    {
        return;
    }
    auto const source = std::make_shared<Source>(*this);
    paste_source = source;
    clipboards_paste_source_is_ours = false; // set to true once our observer gets notified of the change
    clipboard.set_paste_source(source);
}

void mf::WlDataSource::start_drag_n_drop_gesture()
{
    send_target_event(std::nullopt);
    auto const source = std::make_shared<Source>(*this);
    dnd_source = source;
    clipboard.set_drag_n_drop_source(source);
}

auto mf::WlDataSource::offer(rust::String mime_type) -> void
{
    mime_types.push_back(std::string{mime_type});
}

void mf::WlDataSource::paste_source_set(std::shared_ptr<ms::DataExchangeSource> const& source)
{
    if (source && source == paste_source.lock())
    {
        // We're being notified that the paste source we set is now active
        clipboards_paste_source_is_ours = true;
    }
    else if (clipboards_paste_source_is_ours)
    {
        // Our paste source was active (so it's not still pending), but now a different one has been set
        paste_source.reset();
        clipboards_paste_source_is_ours = false;
        send_cancelled_event();
    }
}

void mf::WlDataSource::drag_n_drop_source_set(std::shared_ptr<scene::DataExchangeSource> const& source)
{
    if (source && dnd_source.lock() == source)
    {
        dnd_source_source_is_ours = true;
    }
    else if (dnd_source_source_is_ours)
    {
        dnd_source.reset();
        dnd_source_source_is_ours = false;
        send_cancelled_event();
    }
}

auto mf::WlDataSource::set_actions(uint32_t dnd_actions) -> void
{
    if (!mf::validate_dnd_actions(dnd_actions))
    {
        throw mw::ProtocolError{
            object_id(),
            Error::invalid_action_mask,
            "Invalid DnD actions 0x%x", dnd_actions};
    }
    this->dnd_actions = dnd_actions;
}

auto mf::WlDataSource::drag_n_drop_set_actions(uint32_t dnd_actions, uint32_t preferred_action) -> uint32_t
{
    using DndAction = mw::DataDeviceManager::DndAction;

    if (preferred_action == DndAction::ask)
    {
        preferred_action = DndAction::r_move;
    }

    if (dnd_actions & DndAction::ask)
    {
        preferred_action = DndAction::r_move;
    }

    auto const acceptable_options = this->dnd_actions & dnd_actions;

    for (auto action : {preferred_action, DndAction::copy, DndAction::r_move, DndAction::none})
    {
        if (action & acceptable_options)
        {
            if (!dnd_action || dnd_action.value() != action)
            {
                dnd_action = action;
                send_action_event_if_supported(action);
            }
            return action;
        }
    }

    return DndAction::none;
}

auto mf::WlDataSource::make_source() -> std::shared_ptr<mir::scene::DataExchangeSource>
{
    return std::make_shared<Source>(*this);
}
