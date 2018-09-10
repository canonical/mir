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
#include <unordered_map>
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
class WlSubcompositor;
class WlApplication;
class WlSeat;
class OutputManager;

class Shell;
class MirDisplay;
class SessionAuthorizer;
class DataDeviceManager;

class WaylandExtensions
{
public:
    WaylandExtensions() = default;
    virtual ~WaylandExtensions() = default;
    WaylandExtensions(WaylandExtensions const&) = delete;
    WaylandExtensions& operator=(WaylandExtensions const&) = delete;

    void init(wl_display* display, std::shared_ptr<Shell> const& shell, WlSeat* seat, OutputManager* const output_manager);

    auto get_extension(std::string const& name) const -> std::shared_ptr<void>;

protected:

    virtual void add_extension(std::string const name, std::shared_ptr<void> implementation);
    virtual void custom_extensions(wl_display* display, std::shared_ptr<Shell> const& shell, WlSeat* seat, OutputManager* const output_manager);

private:
    std::unordered_map<std::string, std::shared_ptr<void>> extension_protocols;
};

class WaylandConnector : public Connector
{
public:
    WaylandConnector(
        optional_value<std::string> const& display_name,
        std::shared_ptr<Shell> const& shell,
        std::shared_ptr<MirDisplay> const& display_config,
        std::shared_ptr<input::InputDeviceHub> const& input_hub,
        std::shared_ptr<input::Seat> const& seat,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<SessionAuthorizer> const& session_authorizer,
        bool arw_socket,
        std::unique_ptr<WaylandExtensions> extensions);

    ~WaylandConnector() override;

    void start() override;
    void stop() override;

    int client_socket_fd() const override;

    int client_socket_fd(
        std::function<void(std::shared_ptr<Session> const& session)> const& connect_handler) const override;

    void run_on_wayland_display(std::function<void(wl_display*)> const& functor);

    auto socket_name() const -> optional_value<std::string> override;

    auto get_extension(std::string const& name) const -> std::shared_ptr<void>;
    auto get_wl_display() const -> wl_display*;

private:
    std::unique_ptr<wl_display, void(*)(wl_display*)> const display;
    mir::Fd const pause_signal;
    std::unique_ptr<WlCompositor> compositor_global;
    std::unique_ptr<WlSubcompositor> subcompositor_global;
    std::unique_ptr<WlSeat> seat_global;
    std::unique_ptr<OutputManager> output_manager;
    std::shared_ptr<graphics::WaylandAllocator> const allocator;
    std::unique_ptr<DataDeviceManager> data_device_manager_global;
    std::unique_ptr<WaylandExtensions> const extensions;
    std::thread dispatch_thread;
    wl_event_source* pause_source;
    std::string wayland_display;

    // Only accessed on event loop
    std::unordered_map<int, std::function<void(std::shared_ptr<Session> const& session)>> mutable connect_handlers;
};

auto create_wl_shell(wl_display* display, std::shared_ptr<Shell> const& shell, WlSeat* seat, OutputManager* const output_manager)
    -> std::shared_ptr<void>;
}
}

#endif // MIR_FRONTEND_WAYLAND_CONNECTOR_H_
