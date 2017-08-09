/*
 * Copyright © 2012, 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
#ifndef MIR_SERVER_CONFIGURATION_H_
#define MIR_SERVER_CONFIGURATION_H_

#include <functional>
#include <memory>

namespace mir
{
namespace cookie
{
class Authority;
}
namespace compositor
{
class Compositor;
}
namespace frontend
{
class Connector;
class Shell;
}
namespace shell
{
class SessionContainer;
}
namespace graphics
{
class Display;
class DisplayConfigurationPolicy;
class Platform;
}
namespace input
{
class InputManager;
class InputDispatcher;
class EventFilter;
}

namespace scene
{
class ApplicationNotRespondingDetector;
}

class MainLoop;
class ServerStatusListener;
class DisplayChanger;
class EmergencyCleanup;

class ServerConfiguration
{
public:
    // TODO most of these interfaces are wider DisplayServer needs...
    // TODO ...some or all of them need narrowing
    virtual std::shared_ptr<frontend::Connector> the_connector() = 0;
    virtual std::shared_ptr<frontend::Connector> the_wayland_connector() = 0;
    virtual std::shared_ptr<frontend::Connector> the_prompt_connector() = 0;
    virtual std::shared_ptr<graphics::Display> the_display() = 0;
    virtual std::shared_ptr<compositor::Compositor> the_compositor() = 0;
    virtual std::shared_ptr<input::InputManager> the_input_manager() = 0;
    virtual std::shared_ptr<input::InputDispatcher> the_input_dispatcher() = 0;
    virtual std::shared_ptr<MainLoop> the_main_loop() = 0;
    virtual std::shared_ptr<ServerStatusListener> the_server_status_listener() = 0;
    virtual std::shared_ptr<DisplayChanger> the_display_changer() = 0;
    virtual std::shared_ptr<graphics::Platform>  the_graphics_platform() = 0;
    virtual std::shared_ptr<EmergencyCleanup> the_emergency_cleanup() = 0;
    virtual std::shared_ptr<cookie::Authority> the_cookie_authority() = 0;
    virtual auto the_fatal_error_strategy() -> void (*)(char const* reason, ...) = 0;
    virtual std::shared_ptr<scene::ApplicationNotRespondingDetector> the_application_not_responding_detector() = 0;
    virtual std::function<void()> the_stop_callback() = 0;

protected:
    ServerConfiguration() = default;
    virtual ~ServerConfiguration() = default;

    ServerConfiguration(ServerConfiguration const&) = delete;
    ServerConfiguration& operator=(ServerConfiguration const&) = delete;
};
}


#endif /* MIR_SERVER_CONFIGURATION_H_ */
