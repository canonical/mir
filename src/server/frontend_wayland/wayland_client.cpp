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

#include "wayland_client.h"

#include <mir/scene/session.h>
#include <mir/shell/shell.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

namespace
{
int const max_serial_event_pairs = 100;
}

mf::WaylandClient::WaylandClient(
    wayland_rs::RawWlClient raw_client,
    std::shared_ptr<ms::Session> session,
    std::shared_ptr<msh::Shell> shell,
    WaylandSerialSource serial_source)
    : raw{std::move(raw_client)},
      session{std::move(session)},
      shell{std::move(shell)},
      serial_source{std::move(serial_source)}
{
}

mf::WaylandClient::~WaylandClient()
{
    mark_being_destroyed();
}

auto mf::WaylandClient::raw_client() const -> wayland_rs::RawWlClient const&
{
    return raw;
}

auto mf::WaylandClient::is_being_destroyed() const -> bool
{
    return being_destroyed;
}

auto mf::WaylandClient::client_session() const -> std::shared_ptr<ms::Session>
{
    return session;
}

auto mf::WaylandClient::next_serial(std::shared_ptr<MirEvent const> event) -> uint32_t
{
    auto const serial = ++(*serial_source);
    serial_event_pairs.push_front({serial, std::move(event)});
    while (serial_event_pairs.size() > max_serial_event_pairs)
    {
        serial_event_pairs.pop_back();
    }
    return serial;
}

auto mf::WaylandClient::event_for(uint32_t serial) -> std::optional<std::shared_ptr<MirEvent const>>
{
    for (auto const& pair : serial_event_pairs)
    {
        if (pair.first == serial)
        {
            return pair.second;
        }
    }
    return std::nullopt;
}

void mf::WaylandClient::set_output_geometry_scale(float scale)
{
    output_geometry_scale_ = scale;
}

auto mf::WaylandClient::output_geometry_scale() -> float
{
    return output_geometry_scale_;
}

void mf::WaylandClient::mark_being_destroyed()
{
    if (being_destroyed)
    {
        return;
    }

    being_destroyed = true;
    if (session)
    {
        shell->close_session(session);
    }
}
