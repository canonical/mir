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

#include "mir/shell/window_manager_builder.h"
#include "mir_toolkit/common.h"

#include <functional>
#include <memory>
#include <vector>

namespace mir
{
namespace compositor { class Compositor; class DisplayBufferCompositorFactory; }
namespace frontend { class SessionAuthorizer; class Session; class SessionMediatorReport; }
namespace graphics { class Cursor; class Platform; class Display; class GLConfig; class DisplayConfigurationPolicy; class DisplayConfigurationReport; }
namespace input { class CompositeEventFilter; class InputDispatcher; class CursorListener; class TouchVisualizer; class InputDeviceHub;}
namespace logging { class Logger; }
namespace options { class Option; }
namespace shell
{
class DisplayLayout;
class FocusController;
class HostLifecycleEventListener;
class InputTargeter;
class Shell;
}
namespace scene
{
class ApplicationNotRespondingDetector;
class BufferStreamFactory; 
class PromptSessionListener;
class PromptSessionManager;
class SessionListener;
class SessionCoordinator;
class SurfaceCoordinator;
class SurfaceFactory;
}

class Fd;
class MainLoop;
class ServerStatusListener;

enum class OptionType
{
    null,
    integer,
    string,
    boolean
};

/// Customise and run a Mir server.
class Server
{
public:
    Server();

/** @name Essential operations
 * These are the commands used to run and stop.
 *  @{ */
    /// set the command line.
    /// This must remain valid while apply_settings() and run() are called.
    void set_command_line(int argc, char const* argv[]);

    /// set the cookie-verification secret.
    /// This secret is used to generate timestamps that can be attested to by
    /// libmircookie. Any process this secret is shared with can verify Mir-generated
    /// cookies, or produce their own.
    /// \note If not explicitly set, a random secret will be chosen.
    void set_cookie_secret(std::vector<uint8_t> const& secret);

    /// Applies any configuration options, hooks, or custom implementations.
    /// Must be called before calling run() or accessing any mir subsystems.
    void apply_settings();

    /// The pixel formats that may be used when creating surfaces
    auto supported_pixel_formats() const -> std::vector<MirPixelFormat>;

    /// Run the Mir server until it exits
    void run();

    /// Tell the Mir server to exit
    void stop();

    /// returns true if and only if server exited normally. Otherwise false.
    bool exited_normally();
/** @} */

/** @name Configuration options
 *  These functions allow customization of the handling of configuration
 * options. The add and set functions should be called before apply_settings()
 * otherwise they throw a std::logic_error.
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
        char const* default_value);

    /// Add user configuration option(s) to Mir's option handling.
    /// These will be resolved during initialisation from the command line,
    /// environment variables, a config file or the supplied default.
    void add_configuration_option(
        std::string const& option,
        std::string const& description,
        bool default_value);

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

    /// Set the configuration filename.
    /// This will be searched for and parsed in the standard locations. Vis:
    /// 1. $XDG_CONFIG_HOME (if set, otherwise $HOME/.config (if set))
    /// 2. $XDG_CONFIG_DIRS (if set, otherwise /etc/xdg)
    void set_config_filename(std::string const& config_file);

    /// Returns the configuration options.
    /// This will be null before initialization starts. It will be available
    /// when the init_callback has been invoked (and thereafter until the server exits).
    auto get_options() const -> std::shared_ptr<options::Option>;
/** @} */

/** @name Using hooks into the run() logic
 *  These allow the user to insert logic into startup or error handling.
 *  For obvious reasons they should be called before run().
 *  @{ */
    /// Add a callback to be invoked when the server has been initialized,
    /// but before it starts. This allows client code to get access Mir objects.
    /// If multiple callbacks are added they will be invoked in the sequence added.
    void add_init_callback(std::function<void()> const& init_callback);

    /// Set a handler for exceptions. This is invoked in a catch (...) block and
    /// the exception can be re-thrown to retrieve type information.
    /// The default action is to call mir::report_exception(std::cerr)
    void set_exception_handler(std::function<void()> const& exception_handler);

    /// Functor for processing SIGTERM or SIGINT
    /// This will not be called directly by a signal handler: arbitrary functions may be invoked.
    using Terminator = std::function<void(int signal)>;

    /// Set handler for termination requests.
    /// terminator will be called following receipt of SIGTERM or SIGINT.
    /// The default terminator stop()s the server, replacements should probably
    /// do the same in addition to any additional shutdown logic.
    void set_terminator(Terminator const& terminator);

    /// Functor for processing fatal signals for any "emergency cleanup".
    /// That is: SIGQUIT, SIGABRT, SIGFPE, SIGSEGV & SIGBUS
    ///
    /// \warning This will be called directly by a signal handler:
    /// Only async-signal-safe functions may be called
    using EmergencyCleanupHandler = std::function<void()>;

    /// Add cleanup for abnormal terminations.
    /// handler will be called on receipt of a fatal signal after which the
    /// default signal-handler will terminate the process.
    void add_emergency_cleanup(EmergencyCleanupHandler const& handler);
/** @} */

/** @name Providing custom implementation
 * Provide alternative implementations of Mir subsystems: the functors will be invoked
 * during initialization of the Mir server (or when accessor methods are called).
 * They should be called before apply_settings() otherwise they throw a std::logic_error.
 *  @{ */
    /// Each of the override functions takes a builder functor of the same form
    /// \note If a null pointer is returned by the builder the default is used instead.
    template<typename T> using Builder = std::function<std::shared_ptr<T>()>;

    /// Sets an override functor for creating the compositor.
    void override_the_compositor(Builder<compositor::Compositor> const& compositor_builder);

    /// Sets an override functor for creating the per-display rendering code.
    void override_the_display_buffer_compositor_factory(
        Builder<compositor::DisplayBufferCompositorFactory> const& compositor_builder);

    /// Sets an override functor for creating the display configuration report.
    void override_the_display_configuration_report(
        Builder<graphics::DisplayConfigurationReport> const& report_builder);

    /// Sets an override functor for creating the gl config.
    void override_the_gl_config(Builder<graphics::GLConfig> const& gl_config_builder);

    /// Sets an override functor for creating the host lifecycle event listener.
    void override_the_host_lifecycle_event_listener(
        Builder<shell::HostLifecycleEventListener> const& host_lifecycle_event_listener_builder);

    /// Sets an override functor for creating the input dispatcher.
    void override_the_input_dispatcher(Builder<input::InputDispatcher> const& input_dispatcher_builder);

    /// Sets an override functor for creating the logger.
    void override_the_logger(Builder<logging::Logger> const& logger_builder);

    /// Sets an override functor for creating the prompt session listener.
    void override_the_prompt_session_listener(Builder<scene::PromptSessionListener> const& prompt_session_listener_builder);

    /// Sets an override functor for creating the prompt session manager.
    void override_the_prompt_session_manager(Builder<scene::PromptSessionManager> const& prompt_session_manager_builder);

    /// Sets an override functor for creating the status listener.
    void override_the_server_status_listener(Builder<ServerStatusListener> const& server_status_listener_builder);

    /// Sets an override functor for creating the session authorizer.
    void override_the_session_authorizer(Builder<frontend::SessionAuthorizer> const& session_authorizer_builder);

    /// Sets an override functor for creating the session listener.
    void override_the_session_listener(Builder<scene::SessionListener> const& session_listener_builder);

    /// Sets an override functor for creating the session mediator report.
    void override_the_session_mediator_report(Builder<frontend::SessionMediatorReport> const& session_mediator_builder);

    /// Sets an override functor for creating the shell.
    void override_the_shell(Builder<shell::Shell> const& wrapper);

    /// Sets an override functor for creating the window manager.
    void override_the_window_manager_builder(shell::WindowManagerBuilder const wmb);

    /// Sets an override functor for creating the application not responding detector.
    void override_the_application_not_responding_detector(
        Builder<scene::ApplicationNotRespondingDetector> const& anr_detector_builder);

    /// Each of the wrap functions takes a wrapper functor of the same form
    template<typename T> using Wrapper = std::function<std::shared_ptr<T>(std::shared_ptr<T> const&)>;

    /// Sets a wrapper functor for creating the cursor listener.
    void wrap_cursor_listener(Wrapper<input::CursorListener> const& wrapper);

    /// Sets a wrapper functor for creating the per-display rendering code.
    void wrap_display_buffer_compositor_factory(
        Wrapper<compositor::DisplayBufferCompositorFactory> const& wrapper);

    /// Sets a wrapper functor for creating the display configuration policy.
    void wrap_display_configuration_policy(Wrapper<graphics::DisplayConfigurationPolicy> const& wrapper);

    /// Sets a wrapper functor for creating the shell.
    void wrap_shell(Wrapper<shell::Shell> const& wrapper);
/** @} */

/** @name Getting access to Mir subsystems
 * These may be invoked by the functors that provide alternative implementations of
 * Mir subsystems.
 * They should only be used after apply_settings() is called - otherwise they throw
 *  a std::logic_error.
 *  @{ */
    /// \return the compositor.
    auto the_compositor() const -> std::shared_ptr<compositor::Compositor>;

    /// \return the composite event filter.
    auto the_composite_event_filter() const -> std::shared_ptr<input::CompositeEventFilter>;

    /// \return the cursor listener.
    auto the_cursor_listener() const -> std::shared_ptr<input::CursorListener>;

    /// \return the cursor
    auto the_cursor() const -> std::shared_ptr<graphics::Cursor>;

    /// \return the focus controller.
    auto the_focus_controller() const -> std::shared_ptr<shell::FocusController>;

    /// \return the graphics display.
    auto the_display() const -> std::shared_ptr<graphics::Display>;

    /// \return the GL config.
    auto the_gl_config() const -> std::shared_ptr<graphics::GLConfig>;

    /// \return the graphics platform.
    auto the_graphics_platform() const -> std::shared_ptr<graphics::Platform>;

    /// \return the input targeter.
    auto the_input_targeter() const -> std::shared_ptr<shell::InputTargeter>;

    /// \return the logger.
    auto the_logger() const -> std::shared_ptr<logging::Logger>;

    /// \return the main loop.
    auto the_main_loop() const -> std::shared_ptr<MainLoop>;

    /// \return the prompt session listener.
    auto the_prompt_session_listener() const -> std::shared_ptr<scene::PromptSessionListener>;

    /// \return the prompt session manager.
    auto the_prompt_session_manager() const ->std::shared_ptr<scene::PromptSessionManager>;

    /// \return the session authorizer.
    auto the_session_authorizer() const -> std::shared_ptr<frontend::SessionAuthorizer>;

    /// \return the session coordinator.
    auto the_session_coordinator() const -> std::shared_ptr<scene::SessionCoordinator>;

    /// \return the session listener.
    auto the_session_listener() const -> std::shared_ptr<scene::SessionListener>;

    /// \return the shell.
    auto the_shell() const -> std::shared_ptr<shell::Shell>;

    /// \return the display layout.
    auto the_shell_display_layout() const -> std::shared_ptr<shell::DisplayLayout>;

    /// \return the buffer stream factory
    auto the_buffer_stream_factory() const -> std::shared_ptr<scene::BufferStreamFactory>;

    /// \return the surface factory
    auto the_surface_factory() const -> std::shared_ptr<scene::SurfaceFactory>;

    /// \return the surface coordinator.
    auto the_surface_coordinator() const -> std::shared_ptr<scene::SurfaceCoordinator>;

    /// \return the touch visualizer.
    auto the_touch_visualizer() const -> std::shared_ptr<input::TouchVisualizer>;

    /// \return the input device hub
    auto the_input_device_hub() const -> std::shared_ptr<input::InputDeviceHub>;

    /// \return the application not responding detector
    auto the_application_not_responding_detector() const ->
        std::shared_ptr<scene::ApplicationNotRespondingDetector>;
/** @} */

/** @name Client side support
 * These facilitate use of the server through the client API.
 * They should be called while the server is running (i.e. run() has been called and
 * not exited) otherwise they throw a std::logic_error.
 * @{ */
    using ConnectHandler = std::function<void(std::shared_ptr<frontend::Session> const& session)>;

    /// Get a file descriptor that can be used to connect a client
    /// It can be passed to another process, or used directly with mir_connect()
    /// using the format "fd://%d".
    auto open_client_socket() -> Fd;

    /// Get a file descriptor that can be used to connect a client
    /// It can be passed to another process, or used directly with mir_connect()
    /// using the format "fd://%d".
    /// \param connect_handler callback to be invoked when the client connects
    auto open_client_socket(ConnectHandler const& connect_handler) -> Fd;

    /// Get a file descriptor that can be used to connect a prompt provider
    /// It can be passed to another process, or used directly with mir_connect()
    /// using the format "fd://%d".
    auto open_prompt_socket() -> Fd;
/** @} */

private:
    struct ServerConfiguration;
    struct Self;
    std::shared_ptr<Self> const self;
};
}
#endif /* SERVER_H_ */
