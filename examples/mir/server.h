/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SERVER_H_
#define MIR_SERVER_H_

#include <functional>
#include <memory>

namespace mir
{
namespace compositor{ class Compositor; }
namespace frontend { class SessionAuthorizer; }
namespace graphics { class Platform; class Display; class GLConfig; }
namespace input { class CompositeEventFilter; class InputDispatcher; class CursorListener; }
namespace options { class DefaultConfiguration; class Option; }
namespace shell { class FocusSetter; class DisplayLayout; }
namespace scene
{
class PlacementStrategy;
class SessionListener;
class PromptSessionListener;
class SurfaceConfigurator;
class SessionCoordinator;
class SurfaceCoordinator;
}

class MainLoop;
class ServerStatusListener;

// TODO A lot of functions return "null before initialization completes" - perhaps better to throw?

/// A declarative server implementation that doesn't tie client code to
/// volatile interfaces (like DefaultServerConfiguration)
class Server
{
public:
    /// set a callback to introduce additional configuration options.
    /// this will be invoked by run() before server initialisation starts
    void set_add_configuration_options(
        std::function<void(options::DefaultConfiguration& config)> const& add_configuration_options);

    /// set a handler for any command line options Mir does not recognise.
    /// This will be invoked if any unrecognised options are found during initialisation.
    /// Any unrecognised arguments are passed to this function. The pointers remain valid
    /// for the duration of the call only.
    /// If set_command_line_hander is not called the default action is to exit by
    /// throwing mir::AbnormalExit (which will be handled by the exception handler prior to
    /// exiting run().
    void set_command_line_hander(
        std::function<void(int argc, char const* const* argv)> const& command_line_hander);

    /// set the command line (this must remain valid while run() is called)
    void set_command_line(int argc, char const* argv[]);

    /// set a callback to be invoked when the server has been initialized,
    /// but before it starts. This allows client code to get access Mir objects
    void set_init_callback(std::function<void()> const& init_callback);

    /// Set a handler for exceptions. This is invoked in a catch (...) block and
    /// the exception can be re-thrown to retrieve type information.
    /// The default action is to call mir::report_exception(std::cerr)
    void set_exception_handler(std::function<void()> const& exception_handler);

    /// Run the Mir server until it exits
    void run();

    /// Tell the Mir server to exit
    void stop();

    /// returns true if and only if server exited normally. Otherwise false.
    bool exited_normally();

    /// Returns the configuration options.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto get_options() const -> std::shared_ptr<options::Option>;

    /// Returns the graphics platform options.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto the_graphics_platform() const -> std::shared_ptr<graphics::Platform>;

    /// Returns the graphics display options.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto the_display() const -> std::shared_ptr<graphics::Display>;

    /// Returns the main loop.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto the_main_loop() const -> std::shared_ptr<MainLoop>;

    /// Returns the composite event filter.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto the_composite_event_filter() const -> std::shared_ptr<input::CompositeEventFilter>;

// USC invokes the following
    auto the_cursor_listener() const -> std::shared_ptr<input::CursorListener>;

// USC overrides the following (TODO implement them)
    void override_the_cursor_listener(std::function<std::shared_ptr<input::CursorListener>()> const& cursor_listener_builder);
    void the_server_status_listener(std::function<std::shared_ptr<ServerStatusListener>()> const& server_status_listener_builder);

    void wrap_session_coordinator(std::function<std::shared_ptr<scene::SessionCoordinator>(std::shared_ptr<scene::SessionCoordinator>)>);
    void wrap_surface_coordinator(std::function<std::shared_ptr<scene::SurfaceCoordinator>(std::shared_ptr<scene::SurfaceCoordinator>)>);

// qtmir invokes the following

    /// Returns the display layout.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto the_shell_display_layout() const -> std::shared_ptr<shell::DisplayLayout>;

    /// Returns the session authorizer.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto the_session_authorizer() const -> std::shared_ptr<frontend::SessionAuthorizer>;

    /// Returns the session listener.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto the_session_listener() const -> std::shared_ptr<scene::SessionListener>;

    /// Returns the prompt session listener.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto the_prompt_session_listener() const -> std::shared_ptr<scene::PromptSessionListener>;

    /// Returns the surface configurator.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto the_surface_configurator() const -> std::shared_ptr<scene::SurfaceConfigurator>;

    // qtmir overrides the following
    // TODO I've only implemented the first of these as an example, the rest follow the same pattern
    void override_the_placement_strategy(std::function<std::shared_ptr<scene::PlacementStrategy>()> const& placement_strategy_builder);
    void override_the_session_listener(std::function<std::shared_ptr<scene::SessionListener>()> const& session_listener_builder);
    void override_the_prompt_session_listener(std::function<std::shared_ptr<scene::PromptSessionListener>()> const& prompt_session_listener_builder);
    void override_the_surface_configurator(std::function<std::shared_ptr<scene::SurfaceConfigurator>()> const& surface_configurator_builder);
    void override_the_session_authorizer(std::function<std::shared_ptr<frontend::SessionAuthorizer>()> const& session_authorizer_builder);
    void override_the_compositor(std::function<std::shared_ptr<compositor::Compositor>()> const& compositor_builder);
    void override_the_input_dispatcher(std::function<std::shared_ptr<input::InputDispatcher>()> const& input_dispatcher_builder);
    void override_the_gl_config(std::function<std::shared_ptr<graphics::GLConfig>()> const& gl_config_builder);
    void override_the_server_status_listener(std::function<std::shared_ptr<ServerStatusListener>()> const& server_status_listener_builder);
    void override_the_shell_focus_setter(std::function<std::shared_ptr<shell::FocusSetter>()> const& focus_setter_builder);

private:
    // TODO this should be hidden to avoid ABI changes if it changes.
    std::function<void(options::DefaultConfiguration& config)> add_configuration_options{
        [](options::DefaultConfiguration&){}};
    std::function<void(int argc, char const* const* argv)> command_line_hander{};
    std::function<void()> init_callback{[]{}};
    int argc{0};
    char const** argv{nullptr};
    std::function<void()> exception_handler{};
    bool exit_status{false};
    std::weak_ptr<options::Option> options;
    struct DefaultServerConfiguration;
    DefaultServerConfiguration* server_config{nullptr};

    std::function<std::shared_ptr<scene::PlacementStrategy>()> placement_strategy_builder;
};
}
#endif /* SERVER_H_ */
