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

#include <miral/wayland_tools.h>

class miral::WaylandTools::Self {};

miral::WaylandTools::WaylandTools() :
    self{std::make_shared<Self>()}
{
}

miral::WaylandTools::~WaylandTools() = default;
miral::WaylandTools::WaylandTools(WaylandTools const&) = default;
auto miral::WaylandTools::operator=(WaylandTools const&)-> WaylandTools& = default;

void miral::WaylandTools::operator()(mir::Server& server) const
{
    (void) server;
}

void miral::WaylandTools::for_each_binding(mir::wayland::Client* client, Output const& output,
    std::function<void(wl_resource* output)> const& callback) const
{
    (void) client;
    (void) output;
    (void) callback;
}

