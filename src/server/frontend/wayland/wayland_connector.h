/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_FRONTEND_WAYLAND_CONNECTOR_H_
#define MIR_FRONTEND_WAYLAND_CONNECTOR_H_

#include "mir/frontend/connector.h"
#include "mir/fd.h"
#include "mir/optional_value.h"

#include <wayland-server-core.h>
#include <thread>

namespace mir
{
namespace input
{
class InputDeviceHub;
class Seat;
}
namespace graphics
{
class GraphicBufferAllocator;
class WaylandAllocator;
}
namespace geometry
{
struct Size;
}
namespace frontend
{
class WlCompositor;
class WlApplication;
class WlShell;
class XdgShellV6;
class WlSeat;
class OutputManager;

class Shell;
class DisplayChanger;
class SessionAuthorizer;
class DataDeviceManager;

template<typename Callable>
inline auto run_unless(std::shared_ptr<bool> const& condition, Callable&& callable)
{
    return
        [callable = std::move(callable), condition]()
        {
            if (*condition)
                return;

            callable();
        };
}

std::shared_ptr<mir::frontend::Session> session_for_client(wl_client* client);

struct WlMirWindow
{
    virtual void new_buffer_size(geometry::Size const& buffer_size) = 0;
    virtual void commit() = 0;
    virtual void visiblity(bool visible) = 0;
    virtual void destroy() = 0;
    virtual ~WlMirWindow() = default;
} extern * const null_wl_mir_window_ptr;

class WaylandConnector : public Connector
{
public:
    WaylandConnector(
        optional_value<std::string> const& display_name,
        std::shared_ptr<Shell> const& shell,
        DisplayChanger& display_config,
        std::shared_ptr<input::InputDeviceHub> const& input_hub,
        std::shared_ptr<input::Seat> const& seat,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<SessionAuthorizer> const& session_authorizer,
        bool arw_socket);

    ~WaylandConnector() override;

    void start() override;
    void stop() override;

    int client_socket_fd() const override;

    int client_socket_fd(
        std::function<void(std::shared_ptr<Session> const& session)> const& connect_handler) const override;

    void run_on_wayland_display(std::function<void(wl_display*)> const& functor);

private:
    std::unique_ptr<wl_display, void(*)(wl_display*)> const display;
    mir::Fd const pause_signal;
    std::unique_ptr<WlCompositor> compositor_global;
    std::unique_ptr<WlSeat> seat_global;
    std::unique_ptr<OutputManager> output_manager;
    std::shared_ptr<graphics::WaylandAllocator> const allocator;
    std::unique_ptr<WlShell> shell_global;
    std::unique_ptr<DataDeviceManager> data_device_manager_global;
    std::unique_ptr<XdgShellV6> xdg_shell_global;
    std::thread dispatch_thread;
    wl_event_source* pause_source;
};
}
}

#endif // MIR_FRONTEND_WAYLAND_CONNECTOR_H_
