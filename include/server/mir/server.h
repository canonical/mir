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
namespace compositor { class Compositor; }
namespace frontend { class SessionAuthorizer; }
namespace graphics { class Platform; class Display; class GLConfig; }
namespace input { class CompositeEventFilter; class InputDispatcher; class CursorListener; }
namespace options { class Option; }
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

enum class OptionType
{
    null,
    integer,
    string
};

/// Customise and run a Mir server.
class Server
{
public:
    Server();

/** @name Essential operations
 * These are the commands used to run and stop.
 *  @{ */
    /// set the command line (this must remain valid while run() is called)
    void set_command_line(int argc, char const* argv[]);

    /// Run the Mir server until it exits
    void run();

    /// Tell the Mir server to exit
    void stop();

    /// returns true if and only if server exited normally. Otherwise false.
    bool exited_normally();
/** @} */

/** @name Configuration options
 *  @{ */
    /// Add user configuration option(s) to Mir's option handling.
    /// These will be resolved during initialisation from the command line,
    /// environment variables, a config file or the supplied default.
    void add_configuration_option(
        std::string const& option,
        std::string const& description,
        int default_value);

    /// Add user configuration option(s) to Mir's option handling.
    /// These will be resolved during initialisation from the command line,
    /// environment variables, a config file or the supplied default.
    void add_configuration_option(
        std::string const& option,
        std::string const& description,
        std::string const& default_value);

    /// Add user configuration option(s) to Mir's option handling.
    /// These will be resolved during initialisation from the command line,
    /// environment variables, a config file or the supplied default.
    void add_configuration_option(
        std::string const& option,
        std::string const& description,
        OptionType type);

    /// Set a handler for any command line options Mir does not recognise.
    /// This will be invoked if any unrecognised options are found during initialisation.
    /// Any unrecognised arguments are passed to this function. The pointers remain valid
    /// for the duration of the call only.
    /// If set_command_line_handler is not called the default action is to exit by
    /// throwing mir::AbnormalExit (which will be handled by the exception handler prior to
    /// exiting run().
    void set_command_line_handler(
        std::function<void(int argc, char const* const* argv)> const& command_line_hander);

    /// Returns the configuration options.
    /// This will be null before initialization completes. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto get_options() const -> std::shared_ptr<options::Option>;
/** @} */

/** @name Using hooks into the run() logic
 *  @{ */
    /// Add a callback to be invoked when the server has been initialized,
    /// but before it starts. This allows client code to get access Mir objects.
    /// If multiple callbacks are added they will be invoked in the sequence added.
    void add_init_callback(std::function<void()> const& init_callback);

    /// Set a handler for exceptions. This is invoked in a catch (...) block and
    /// the exception can be re-thrown to retrieve type information.
    /// The default action is to call mir::report_exception(std::cerr)
    void set_exception_handler(std::function<void()> const& exception_handler);
/** @} */

/** @name Getting access to Mir subsystems
 * These will throw before initialization starts or after the server exits.
 * They may be invoked by the functors that provide alternative implementations of
 * Mir subsystems and when the init_callback is invoked (and thereafter
 * until the server exits).
 *  @{ */
    /// \return the composite event filter.
    auto the_composite_event_filter() const -> std::shared_ptr<input::CompositeEventFilter>;

    /// \return the cursor listener.
    auto the_cursor_listener() const -> std::shared_ptr<input::CursorListener>;

    /// \return the graphics display.
    auto the_display() const -> std::shared_ptr<graphics::Display>;

    /// \return the graphics platform.
    auto the_graphics_platform() const -> std::shared_ptr<graphics::Platform>;

    /// \return the main loop.
    auto the_main_loop() const -> std::shared_ptr<MainLoop>;

    /// \return the prompt session listener.
    auto the_prompt_session_listener() const -> std::shared_ptr<scene::PromptSessionListener>;

    /// \return the session authorizer.
    auto the_session_authorizer() const -> std::shared_ptr<frontend::SessionAuthorizer>;

    /// \return the session listener.
    auto the_session_listener() const -> std::shared_ptr<scene::SessionListener>;

    /// \return the display layout.
    auto the_shell_display_layout() const -> std::shared_ptr<shell::DisplayLayout>;

    /// \return the surface configurator.
    auto the_surface_configurator() const -> std::shared_ptr<scene::SurfaceConfigurator>;
/** @} */

/** @name Providing custom implementation
 * Provide alternative implementations of Mir subsystems: the functors will be invoked during initialization
 * (this is only useful before initialization starts).
 *  @{ */
    /// Sets an override functor for creating the compositor.
    void override_the_compositor(std::function<std::shared_ptr<compositor::Compositor>()> const& compositor_builder);

    /// Sets an override functor for creating the cursor listener.
    void override_the_cursor_listener(std::function<std::shared_ptr<input::CursorListener>()> const& cursor_listener_builder);

    /// Sets an override functor for creating the gl config.
    void override_the_gl_config(std::function<std::shared_ptr<graphics::GLConfig>()> const& gl_config_builder);

    /// Sets an override functor for creating the input dispatcher.
    void override_the_input_dispatcher(std::function<std::shared_ptr<input::InputDispatcher>()> const& input_dispatcher_builder);

    /// Sets an override functor for creating the placement strategy.
    void override_the_placement_strategy(std::function<std::shared_ptr<scene::PlacementStrategy>()> const& placement_strategy_builder);

    /// Sets an override functor for creating the prompt session listener.
    void override_the_prompt_session_listener(std::function<std::shared_ptr<scene::PromptSessionListener>()> const& prompt_session_listener_builder);

    /// Sets an override functor for creating the status listener.
    void override_the_server_status_listener(std::function<std::shared_ptr<ServerStatusListener>()> const& server_status_listener_builder);

    /// Sets an override functor for creating the session authorizer.
    void override_the_session_authorizer(std::function<std::shared_ptr<frontend::SessionAuthorizer>()> const& session_authorizer_builder);

    /// Sets an override functor for creating the session listener.
    void override_the_session_listener(std::function<std::shared_ptr<scene::SessionListener>()> const& session_listener_builder);

    /// Sets an override functor for creating the shell focus setter.
    void override_the_shell_focus_setter(std::function<std::shared_ptr<shell::FocusSetter>()> const& focus_setter_builder);

    /// Sets an override functor for creating the surface configurator.
    void override_the_surface_configurator(std::function<std::shared_ptr<scene::SurfaceConfigurator>()> const& surface_configurator_builder);

    /// Sets a wrapper functor for creating the session coordinator.
    void wrap_session_coordinator(std::function<std::shared_ptr<scene::SessionCoordinator>(std::shared_ptr<scene::SessionCoordinator> const& wrapped)> const& wrapper);

    /// Sets a wrapper functor for creating the surface coordinator.
    void wrap_surface_coordinator(std::function<std::shared_ptr<scene::SurfaceCoordinator>(std::shared_ptr<scene::SurfaceCoordinator> const& wrapped)> const& wrapper);
/** @} */

private:
    struct ServerConfiguration;
    struct Self;
    std::shared_ptr<Self> const self;
};
}
#endif /* SERVER_H_ */
