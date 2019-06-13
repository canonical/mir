/*
 * Copyright © 2014-2019 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SERVER_H_
#define MIR_SERVER_H_

#include "mir/shell/window_manager_builder.h"
#include "mir/optional_value.h"
#include "mir_toolkit/common.h"

#include <functional>
#include <memory>
#include <vector>

struct wl_display;

namespace mir
{
template<class Observer>
class ObserverRegistrar;

namespace compositor { class Compositor; class DisplayBufferCompositorFactory; class CompositorReport; }
namespace frontend { class SessionAuthorizer; class Session; class SessionMediatorObserver; }
namespace graphics { class Cursor; class Platform; class Display; class GLConfig; class DisplayConfigurationPolicy; class DisplayConfigurationObserver; }
namespace input { class CompositeEventFilter; class InputDispatcher; class CursorListener; class CursorImages; class TouchVisualizer; class InputDeviceHub;}
namespace logging { class Logger; }
namespace options { class Option; }
namespace scene { class Session; }
namespace cookie
{
using Secret = std::vector<uint8_t>;
class Authority;
}
namespace shell
{
class DisplayLayout;
class DisplayConfigurationController;
class FocusController;
class HostLifecycleEventListener;
class InputTargeter;
class PersistentSurfaceStore;
class Shell;
class SurfaceStack;
}
namespace scene
{
class ApplicationNotRespondingDetector;
class BufferStreamFactory;
class PromptSessionListener;
class PromptSessionManager;
class SessionListener;
class SessionCoordinator;
class SurfaceFactory;
class CoordinateTranslator;
}
namespace input
{
class SeatObserver;
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
        double default_value);

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
    /// Add a callback to be invoked when the settings have been applied, but before
    /// the server has been initialized. This allows client code to get access Mir objects.
    /// If multiple callbacks are added they will be invoked in the sequence added.
    void add_pre_init_callback(std::function<void()> const& pre_init_callback);

    /// Add a callback to be invoked when the server has been initialized,
    /// but before it starts. This allows client code to get access Mir objects.
    /// If multiple callbacks are added they will be invoked in the sequence added.
    void add_init_callback(std::function<void()> const& init_callback);

    /// Add a callback to be invoked when the server is about to stop,
    /// If multiple callbacks are added they will be invoked in the reverse sequence added.
    void add_stop_callback(std::function<void()> const& stop_callback);

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

    /// Sets an override functor for creating the cursor images.
    void override_the_cursor_images(Builder<input::CursorImages> const& cursor_images_builder);

    /// Sets an override functor for creating the per-display rendering code.
    void override_the_display_buffer_compositor_factory(
        Builder<compositor::DisplayBufferCompositorFactory> const& compositor_builder);

    /// Sets an override functor for creating the gl config.
    void override_the_gl_config(Builder<graphics::GLConfig> const& gl_config_builder);

    /// Sets an override functor for creating the cookie authority.
    /// A secret can be saved and any process this secret is shared
    /// with can verify Mir-generated cookies, or produce their own.
    void override_the_cookie_authority(Builder<cookie::Authority> const& cookie_authority_builder);

    /// Sets an override functor for creating the coordinate translator.
    void override_the_coordinate_translator(
        Builder<scene::CoordinateTranslator> const& coordinate_translator_builder);

    /// Sets an override functor for creating the host lifecycle event listener.
    void override_the_host_lifecycle_event_listener(
        Builder<shell::HostLifecycleEventListener> const& host_lifecycle_event_listener_builder);

    /// Sets an override functor for creating the input dispatcher.
    void override_the_input_dispatcher(Builder<input::InputDispatcher> const& input_dispatcher_builder);

    /// Sets an override functor for creating the input targeter.
    void override_the_input_targeter(Builder<shell::InputTargeter> const& input_targeter_builder);

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

    /// Sets an override functor for creating the shell.
    void override_the_shell(Builder<shell::Shell> const& wrapper);

    /// Sets an override functor for creating the window manager.
    void override_the_window_manager_builder(shell::WindowManagerBuilder const wmb);

    /// Sets an override functor for creating the application not responding detector.
    void override_the_application_not_responding_detector(
        Builder<scene::ApplicationNotRespondingDetector> const& anr_detector_builder);

    /// Sets an override functor for creating the persistent_surface_store
    void override_the_persistent_surface_store(Builder<shell::PersistentSurfaceStore> const& persistent_surface_store);

    /// Each of the wrap functions takes a wrapper functor of the same form
    template<typename T> using Wrapper = std::function<std::shared_ptr<T>(std::shared_ptr<T> const&)>;

    /// Sets a wrapper functor for creating the cursor.
    void wrap_cursor(Wrapper<graphics::Cursor> const& cursor_builder);

    /// Sets a wrapper functor for creating the cursor listener.
    void wrap_cursor_listener(Wrapper<input::CursorListener> const& wrapper);

    /// Sets a wrapper functor for creating the per-display rendering code.
    void wrap_display_buffer_compositor_factory(
        Wrapper<compositor::DisplayBufferCompositorFactory> const& wrapper);

    /// Sets a wrapper functor for creating the display configuration policy.
    void wrap_display_configuration_policy(Wrapper<graphics::DisplayConfigurationPolicy> const& wrapper);

    /// Sets a wrapper functor for creating the shell.
    void wrap_shell(Wrapper<shell::Shell> const& wrapper);

    /// Sets a wrapper functor for creating the surface stack.
    void wrap_surface_stack(Wrapper<shell::SurfaceStack> const& surface_stack);

    /// Sets a wrapper functor for creating the application not responding detector.
    void wrap_application_not_responding_detector(Wrapper<scene::ApplicationNotRespondingDetector> const & anr_detector);
/** @} */

/** @name Getting access to Mir subsystems
 * These may be invoked by the functors that provide alternative implementations of
 * Mir subsystems.
 * They should only be used after apply_settings() is called - otherwise they throw
 *  a std::logic_error.
 *  @{ */
    /// \return the compositor.
    auto the_compositor() const -> std::shared_ptr<compositor::Compositor>;

    /// \return the compositor report.
    auto the_compositor_report() const -> std::shared_ptr<compositor::CompositorReport>;

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

    auto the_display_configuration_controller() const -> std::shared_ptr<shell::DisplayConfigurationController>;

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

    /// \return the surface stack.
    auto the_surface_stack() const -> std::shared_ptr<shell::SurfaceStack>;

    /// \return the touch visualizer.
    auto the_touch_visualizer() const -> std::shared_ptr<input::TouchVisualizer>;

    /// \return the input device hub
    auto the_input_device_hub() const -> std::shared_ptr<input::InputDeviceHub>;

    /// \return the application not responding detector
    auto the_application_not_responding_detector() const ->
        std::shared_ptr<scene::ApplicationNotRespondingDetector>;

    /// \return the persistent surface store
    auto the_persistent_surface_store() const -> std::shared_ptr<shell::PersistentSurfaceStore>;

    /// \return a registrar to add and remove DisplayConfigurationChangeObservers
    auto the_display_configuration_observer_registrar() const ->
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>>;

    /// \return a registrar to add and remove SeatObservers
    auto the_seat_observer_registrar() const ->
        std::shared_ptr<ObserverRegistrar<input::SeatObserver>>;

    /// \return a registrar to add and remove SessionMediatorObservers
    auto the_session_mediator_observer_registrar() const ->
        std::shared_ptr<ObserverRegistrar<frontend::SessionMediatorObserver>>;


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

    /// Get a file descriptor that can be used to connect a Wayland client
    /// \param connect_handler callback to be invoked when the client connects
    auto open_client_wayland(ConnectHandler const& connect_handler) -> int;

    /// Get a file descriptor that can be used to connect a prompt provider
    /// It can be passed to another process, or used directly with mir_connect()
    /// using the format "fd://%d".
    auto open_prompt_socket() -> Fd;

    /// Get a file descriptor that can be used to connect a Wayland client
    /// It can be passed to another process, or used with wl_display_connect_to_fd()
    auto open_wayland_client_socket() -> Fd;

    void run_on_wayland_display(std::function<void(wl_display*)> const& functor);
    void add_wayland_extension(
        std::string const& name,
        std::function<std::shared_ptr<void>(
            wl_display*,
            std::function<void(std::function<void()>&& work)> const&)> builder);

    void set_wayland_extension_filter(
        std::function<bool(std::shared_ptr<scene::Session> const&, char const*)> const& extension_filter);

    /// Get the name of the Mir endpoint (if any) usable as a $MIR_SERVER value
    auto mir_socket_name() const -> optional_value<std::string>;

    /// Get the name of the Wayland endpoint (if any) usable as a $WAYLAND_DISPLAY value
    auto wayland_display() const -> optional_value<std::string>;

    /// Overrides the standard set of Wayland extensions (mir::frontend::get_standard_extensions()) with a new list
    void set_enabled_wayland_extensions(std::vector<std::string> const& extensions);
/** @} */

private:
    struct ServerConfiguration;
    struct Self;
    std::shared_ptr<Self> const self;
};
}
#endif /* SERVER_H_ */
