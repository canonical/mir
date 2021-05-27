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

#include "wl_data_source.h"
#include "mir/scene/clipboard.h"
#include "mir/executor.h"

#include <vector>
#include <algorithm>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland;

class mf::WlDataSource::ClipboardObserver : public ms::ClipboardObserver
{
public:
    ClipboardObserver(WlDataSource* owner) : owner{owner}
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

    wayland::Weak<WlDataSource> const owner;
};

class mf::WlDataSource::Source : public ms::ClipboardSource
{
public:
    Source(WlDataSource& wl_data_source)
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
        wayland_executor->spawn([wl_data_source=wl_data_source, mime_type, target_fd]()
            {
                if (wl_data_source)
                {
                    wl_data_source.value().send_send_event(mime_type, target_fd);
                }
            });
    }

private:
    std::shared_ptr<Executor> const wayland_executor;
    wayland::Weak<WlDataSource> const wl_data_source;
    std::vector<std::string> types;
};

mf::WlDataSource::WlDataSource(
    wl_resource* new_resource,
    std::shared_ptr<Executor> const& wayland_executor,
    scene::Clipboard& clipboard)
    : mw::DataSource{new_resource, Version<3>()},
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
}

auto mf::WlDataSource::from(struct wl_resource* resource) -> WlDataSource*
{
    return dynamic_cast<mf::WlDataSource*>(wayland::DataSource::from(resource));
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

void mf::WlDataSource::offer(std::string const& mime_type)
{
    mime_types.push_back(mime_type);
}

void mf::WlDataSource::paste_source_set(std::shared_ptr<scene::ClipboardSource> const& source)
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
