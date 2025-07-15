/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_WAYLAND_TOOLS_H
#define MIRAL_WAYLAND_TOOLS_H

#include <miral/application.h>

#include <functional>
#include <memory>

struct wl_display;
struct wl_client;
struct wl_resource;

namespace mir { class Server; }

namespace mir::wayland { class Client; }

namespace miral
{
class Output;

/// Tools for implementors of Wayland extension protocols
///
/// \remark Since MirAL 5.5
class WaylandTools
{
public:
    WaylandTools();
    ~WaylandTools();
    WaylandTools(WaylandTools const&);
    auto operator=(WaylandTools const&) -> WaylandTools&;

    void operator()(mir::Server& server) const;

    void for_each_binding(mir::wayland::Client* client, Output const& output, std::function<void(wl_resource* output)> const& callback) const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_WAYLAND_TOOLS_H
