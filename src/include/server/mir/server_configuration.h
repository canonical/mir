/*
 * Copyright © 2012-2019 Canonical Ltd.
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
 */
#ifndef MIR_SERVER_CONFIGURATION_H_
#define MIR_SERVER_CONFIGURATION_H_

#include <functional>
#include <memory>
#include <vector>

struct wl_display;

namespace mir
{
namespace cookie
{
class Authority;
}
namespace compositor
{
class Compositor;
class ScreenShooter;
}
namespace frontend
{
class Connector;
}
namespace shell
{
class SessionContainer;
class Shell;
class IdleHandler;
}
namespace graphics
{
class Display;
class DisplayConfigurationPolicy;
class DisplayPlatform;
class RenderingPlatform;
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
class Session;
class Clipboard;
class TextInputHub;
class IdleHub;
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
    virtual std::shared_ptr<frontend::Connector> the_wayland_connector() = 0;
    virtual std::shared_ptr<frontend::Connector> the_xwayland_connector() = 0;
    virtual std::shared_ptr<graphics::Display> the_display() = 0;
    virtual std::shared_ptr<compositor::Compositor> the_compositor() = 0;
    virtual std::shared_ptr<compositor::ScreenShooter> the_screen_shooter() = 0;
    virtual std::shared_ptr<input::InputManager> the_input_manager() = 0;
    virtual std::shared_ptr<input::InputDispatcher> the_input_dispatcher() = 0;
    virtual std::shared_ptr<MainLoop> the_main_loop() = 0;
    virtual std::shared_ptr<ServerStatusListener> the_server_status_listener() = 0;
    virtual std::shared_ptr<DisplayChanger> the_display_changer() = 0;
    virtual auto the_display_platforms() -> std::vector<std::shared_ptr<graphics::DisplayPlatform>> const& = 0;
    virtual auto the_rendering_platforms() -> std::vector<std::shared_ptr<graphics::RenderingPlatform>> const& = 0;
    virtual std::shared_ptr<EmergencyCleanup> the_emergency_cleanup() = 0;
    virtual std::shared_ptr<cookie::Authority> the_cookie_authority() = 0;
    virtual auto the_fatal_error_strategy() -> void (*)(char const* reason, ...) = 0;
    virtual std::shared_ptr<scene::ApplicationNotRespondingDetector> the_application_not_responding_detector() = 0;
    virtual std::shared_ptr<scene::Clipboard> the_main_clipboard() = 0;
    virtual std::shared_ptr<scene::Clipboard> the_primary_selection_clipboard() = 0;
    virtual std::shared_ptr<scene::TextInputHub> the_text_input_hub() = 0;
    virtual std::shared_ptr<scene::IdleHub> the_idle_hub() = 0;
    virtual std::shared_ptr<shell::IdleHandler> the_idle_handler() = 0;
    virtual std::function<void()> the_stop_callback() = 0;
    virtual void add_wayland_extension(
        std::string const& name,
        std::function<std::shared_ptr<void>(
            wl_display*,
            std::function<void(std::function<void()>&& work)> const&)> builder) = 0;

    using WaylandProtocolExtensionFilter = std::function<bool(std::shared_ptr<scene::Session> const&, char const*)>;
    virtual void set_wayland_extension_filter(WaylandProtocolExtensionFilter const& extension_filter) = 0;
    virtual void set_enabled_wayland_extensions(std::vector<std::string> const& extensions) = 0;

protected:
    ServerConfiguration() = default;
    virtual ~ServerConfiguration() = default;

    ServerConfiguration(ServerConfiguration const&) = delete;
    ServerConfiguration& operator=(ServerConfiguration const&) = delete;
};
}


#endif /* MIR_SERVER_CONFIGURATION_H_ */
