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
#include <miral/output.h>

#include <mir/server.h>

#include <mutex>

class miral::WaylandTools::Self
{
public:
    void set_server(mir::Server* server)
    {
        std::lock_guard lock(mutex);
        this->server = server;
    }

    void for_each_output_binding(
        mir::wayland::Client* client,
        mir::graphics::DisplayConfigurationOutputId output,
        std::function<void(struct wl_resource* output)> const& callback)
    {
        std::lock_guard lock(mutex);
        if (server)
        {
            server->for_each_output_binding(client, output, callback);
        }
    }

private:
    std::mutex mutex;
    mir::Server* server = nullptr;
};

miral::WaylandTools::WaylandTools() :
    self{std::make_shared<Self>()}
{
}

miral::WaylandTools::~WaylandTools() = default;
miral::WaylandTools::WaylandTools(WaylandTools const&) = default;
auto miral::WaylandTools::operator=(WaylandTools const&)-> WaylandTools& = default;

void miral::WaylandTools::operator()(mir::Server& server) const
{
    self->set_server(&server);
    server.add_stop_callback([self=this->self] { self->set_server(nullptr); });
}

void miral::WaylandTools::for_each_binding(
    mir::wayland::Client* client,
    Output const& output,
    std::function<void(wl_resource* output)> const& callback) const
{
    self->for_each_output_binding(client, mir::graphics::DisplayConfigurationOutputId{output.id()}, callback);
}

