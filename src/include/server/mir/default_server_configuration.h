/*
 * Copyright © 2012-2020 Canonical Ltd.
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
#ifndef MIR_DEFAULT_SERVER_CONFIGURATION_H_
#define MIR_DEFAULT_SERVER_CONFIGURATION_H_

#include "mir/cached_ptr.h"
#include "mir/server_configuration.h"
#include "mir/shell/window_manager_builder.h"

#include <memory>
#include <string>
#include <mutex>

namespace mir
{
class ServerActionQueue;
class SharedLibrary;
class SharedLibraryProberReport;

template<class Observer>
class ObserverRegistrar;

template<class Observer>
class ObserverMultiplexer;

class ConsoleServices;

namespace cookie
{
class Authority;
}
namespace dispatch
{
class MultiplexingDispatchable;
}
namespace compositor
{
class BufferStreamFactory;
class Scene;
class DisplayBufferCompositorFactory;
class Compositor;
class CompositorReport;
}
namespace frontend
{
class Connector;
class SessionAuthorizer;
class EventSink;
class DisplayChanger;
class InputConfigurationChanger;
class SurfaceStack;
}

namespace shell
{
class DisplayConfigurationController;
class InputTargeter;
class FocusController;
class DisplayLayout;
class Shell;
class ShellReport;
class SurfaceStack;
class PersistentSurfaceStore;
namespace decoration { class Manager; }
}
namespace time
{
class Clock;
}
namespace scene
{
class SurfaceFactory;
class BroadcastingSessionEventSink;
class BufferStreamFactory;
class MediatingDisplayChanger;
class SessionContainer;
class SessionEventSink;
class SessionEventHandlerRegister;
class SessionListener;
class SessionCoordinator;
class SurfaceStack;
class SceneReport;
class PromptSessionListener;
class PromptSessionManager;
}
namespace graphics
{
class Platform;
class Display;
class DisplayReport;
class DisplayConfigurationObserver;
class GraphicBufferAllocator;
class Cursor;
class CursorImage;
class GLConfig;
}
namespace input
{
class InputReport;
class SeatObserver;
class Scene;
class InputManager;
class KeyboardObserver;
class SurfaceInputDispatcher;
class InputDeviceRegistry;
class InputDeviceHub;
class DefaultInputDeviceHub;
class CompositeEventFilter;
class EventFilterChainDispatcher;
class CursorListener;
class TouchVisualizer;
class CursorImages;
class Seat;
class KeyMapper;
}

namespace logging
{
class Logger;
}

namespace options
{
class Option;
class Configuration;
}

namespace report
{
class ReportFactory;
}

namespace renderer
{
class RendererFactory;
}

struct WaylandExtensionHook
{
    std::string name;
    std::function<std::shared_ptr<void>(wl_display*, std::function<void(std::function<void()>&& work)> const& run_on_wayland_mainloop)> builder;
};

class DefaultServerConfiguration : public virtual ServerConfiguration
{
public:
    DefaultServerConfiguration(int argc, char const* argv[]);
    explicit DefaultServerConfiguration(std::shared_ptr<options::Configuration> const& configuration_options);

    /** @name DisplayServer dependencies
     * dependencies of DisplayServer on the rest of the Mir
     *  @{ */
    std::shared_ptr<frontend::Connector>    the_wayland_connector() override;
    std::shared_ptr<frontend::Connector>    the_xwayland_connector() override;
    std::shared_ptr<graphics::Display>      the_display() override;
    std::shared_ptr<compositor::Compositor> the_compositor() override;
    std::shared_ptr<compositor::ScreenShooter> the_screen_shooter() override;
    std::shared_ptr<input::InputManager>    the_input_manager() override;
    std::shared_ptr<MainLoop>               the_main_loop() override;
    std::shared_ptr<ServerStatusListener>   the_server_status_listener() override;
    std::shared_ptr<DisplayChanger>         the_display_changer() override;
    std::vector<std::shared_ptr<graphics::DisplayPlatform>> const& the_display_platforms() override;
    auto the_rendering_platforms() -> std::vector<std::shared_ptr<graphics::RenderingPlatform>> const& override;
    std::shared_ptr<input::InputDispatcher> the_input_dispatcher() override;
    std::shared_ptr<EmergencyCleanup>       the_emergency_cleanup() override;
    std::shared_ptr<cookie::Authority>      the_cookie_authority() override;
    std::shared_ptr<scene::Clipboard>       the_main_clipboard() override;
    std::shared_ptr<scene::Clipboard>       the_primary_selection_clipboard() override;
    std::shared_ptr<scene::TextInputHub>    the_text_input_hub() override;
    std::shared_ptr<scene::IdleHub>         the_idle_hub() override;
    std::shared_ptr<shell::IdleHandler>     the_idle_handler() override;
    std::function<void()>                   the_stop_callback() override;
    void add_wayland_extension(
        std::string const& name,
        std::function<std::shared_ptr<void>(
            wl_display*,
            std::function<void(std::function<void()>&& work)> const&)> builder) override;

    void set_wayland_extension_filter(WaylandProtocolExtensionFilter const& extension_filter) override;
    void set_enabled_wayland_extensions(std::vector<std::string> const& extensions) override;

    /**
     * Function to call when a "fatal" error occurs. This implementation allows
     * the default strategy to be overridden by --on-fatal-error-except to avoid a
     * core.
     * To change the default strategy used FatalErrorStrategy. See acceptance test
     * ServerShutdown.fatal_error_default_can_be_changed_to_abort
     * for an example.
     */
    auto the_fatal_error_strategy() -> void (*)(char const* reason, ...) override final;
    std::shared_ptr<scene::ApplicationNotRespondingDetector> the_application_not_responding_detector() override;
    virtual std::shared_ptr<scene::ApplicationNotRespondingDetector>
        wrap_application_not_responding_detector(
            std::shared_ptr<scene::ApplicationNotRespondingDetector> const& wrapped);
    /** @} */

    /** @name graphics configuration - customization
     * configurable interfaces for modifying graphics
     *  @{ */
    virtual std::shared_ptr<renderer::RendererFactory>   the_renderer_factory();
    virtual std::shared_ptr<shell::DisplayConfigurationController> the_display_configuration_controller();
    virtual std::shared_ptr<graphics::DisplayConfigurationPolicy> the_display_configuration_policy();

    virtual std::shared_ptr<graphics::GLConfig> the_gl_config();
    /** @} */

    /** @name graphics configuration - dependencies
     * dependencies of graphics on the rest of the Mir
     *  @{ */
    virtual std::shared_ptr<graphics::DisplayReport> the_display_report();
    virtual std::shared_ptr<graphics::Cursor> the_cursor();
    virtual std::shared_ptr<graphics::Cursor> wrap_cursor(std::shared_ptr<graphics::Cursor> const& wrapped);
    virtual std::shared_ptr<graphics::CursorImage> the_default_cursor_image();
    virtual std::shared_ptr<input::CursorImages> the_cursor_images();
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>>
        the_display_configuration_observer_registrar();

    /** @} */

    /** @name compositor configuration - customization
     * configurable interfaces for modifying compositor
     *  @{ */
    virtual std::shared_ptr<compositor::CompositorReport> the_compositor_report();
    virtual std::shared_ptr<compositor::DisplayBufferCompositorFactory> the_display_buffer_compositor_factory();
    virtual std::shared_ptr<compositor::DisplayBufferCompositorFactory> wrap_display_buffer_compositor_factory(
        std::shared_ptr<compositor::DisplayBufferCompositorFactory> const& wrapped);
    /** @} */

    /** @name compositor configuration - dependencies
     * dependencies of compositor on the rest of the Mir
     *  @{ */
    virtual std::shared_ptr<graphics::GraphicBufferAllocator> the_buffer_allocator();
    virtual std::shared_ptr<compositor::Scene>                  the_scene();
    /** @} */

    /** @name frontend configuration - dependencies
     * dependencies of frontend on the rest of the Mir
     *  @{ */
    virtual std::shared_ptr<frontend::SessionAuthorizer>      the_session_authorizer();
    virtual std::shared_ptr<frontend::DisplayChanger>         the_frontend_display_changer();
    /** @name frontend configuration - internal dependencies
     * internal dependencies of frontend
     *  @{ */
    virtual std::shared_ptr<frontend::SurfaceStack>           the_frontend_surface_stack();
    /** @} */
    /** @} */

    // the_focus_controller() is an interface for the_shell().
    std::shared_ptr<shell::FocusController> the_focus_controller();

    /** @name shell configuration - customization
     * configurable interfaces for modifying shell
     *  @{ */
    virtual auto the_shell() -> std::shared_ptr<shell::Shell>;
    virtual auto the_window_manager_builder() -> shell::WindowManagerBuilder;
    virtual auto the_decoration_manager() -> std::shared_ptr<shell::decoration::Manager>;
    virtual std::shared_ptr<scene::SessionListener>     the_session_listener();
    virtual std::shared_ptr<shell::DisplayLayout>       the_shell_display_layout();
    virtual std::shared_ptr<scene::PromptSessionListener> the_prompt_session_listener();
    virtual std::shared_ptr<scene::PromptSessionManager>  the_prompt_session_manager();

    virtual std::shared_ptr<shell::PersistentSurfaceStore> the_persistent_surface_store();
    virtual std::shared_ptr<shell::ShellReport>         the_shell_report();
    /** @} */

    /** @name internal scene configuration
     * builder functions used in the default implementation.
     * The interfaces returned are not published, so the functions are only useful in tests
     *  @{ */
    virtual std::shared_ptr<scene::SessionContainer>  the_session_container();
    virtual std::shared_ptr<scene::SessionEventSink>  the_session_event_sink();
    virtual std::shared_ptr<scene::SessionEventHandlerRegister> the_session_event_handler_register();
    virtual std::shared_ptr<scene::SurfaceFactory>    the_surface_factory();
    virtual std::shared_ptr<shell::SurfaceStack>      the_surface_stack();
    virtual std::shared_ptr<shell::SurfaceStack>      wrap_surface_stack(std::shared_ptr<shell::SurfaceStack> const& wrapped);
    /** @} */

    /** @name scene configuration - dependencies
     * dependencies of scene on the rest of the Mir
     *  @{ */
    virtual std::shared_ptr<scene::BufferStreamFactory> the_buffer_stream_factory();
    virtual std::shared_ptr<scene::SceneReport>      the_scene_report();
    /** @} */

    /** @name scene configuration - services
     * services provided by scene for the rest of Mir
     *  @{ */
    virtual std::shared_ptr<scene::SessionCoordinator>  the_session_coordinator();
    /** @} */


    /** @name input configuration
     *  @{ */
    virtual std::shared_ptr<input::InputReport> the_input_report();
    virtual std::shared_ptr<ObserverRegistrar<input::SeatObserver>> the_seat_observer_registrar();
    virtual std::shared_ptr<input::CompositeEventFilter> the_composite_event_filter();

    virtual std::shared_ptr<input::EventFilterChainDispatcher> the_event_filter_chain_dispatcher();

    virtual std::shared_ptr<shell::InputTargeter> the_input_targeter();
    virtual std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> the_keyboard_observer_registrar();
    virtual std::shared_ptr<input::Scene>  the_input_scene();
    virtual std::shared_ptr<input::CursorListener> the_cursor_listener();
    virtual std::shared_ptr<input::TouchVisualizer> the_touch_visualizer();
    virtual std::shared_ptr<input::Seat> the_seat();
    virtual std::shared_ptr<input::KeyMapper> the_key_mapper();

    // new input reading related parts:
    virtual std::shared_ptr<dispatch::MultiplexingDispatchable> the_input_reading_multiplexer();
    virtual std::shared_ptr<input::InputDeviceRegistry> the_input_device_registry();
    virtual std::shared_ptr<input::InputDeviceHub> the_input_device_hub();
    virtual std::shared_ptr<input::SurfaceInputDispatcher> the_surface_input_dispatcher();
    /** @} */

    /** @name logging configuration - customization
     * configurable interfaces for modifying logging
     *  @{ */
    virtual std::shared_ptr<logging::Logger> the_logger();
    /** @} */

    virtual std::shared_ptr<time::Clock> the_clock();
    virtual std::shared_ptr<ServerActionQueue> the_server_action_queue();
    virtual std::shared_ptr<SharedLibraryProberReport>  the_shared_library_prober_report();

    virtual std::shared_ptr<ConsoleServices> the_console_services();
    auto default_reports() -> std::shared_ptr<void>;

protected:
    std::shared_ptr<options::Option> the_options() const;
    std::shared_ptr<input::DefaultInputDeviceHub>  the_default_input_device_hub();
    std::shared_ptr<graphics::DisplayConfigurationObserver> the_display_configuration_observer();
    std::shared_ptr<input::SeatObserver> the_seat_observer();

    virtual std::shared_ptr<scene::MediatingDisplayChanger> the_mediating_display_changer();

    /** @} */

    /** @Convenience wrapper functions
     *  @{ */
    virtual std::shared_ptr<graphics::DisplayConfigurationPolicy> wrap_display_configuration_policy(
        std::shared_ptr<graphics::DisplayConfigurationPolicy> const& wrapped);

    virtual std::shared_ptr<shell::Shell>  wrap_shell(
        std::shared_ptr<shell::Shell> const& wrapped);

    virtual std::shared_ptr<input::CursorListener>  wrap_cursor_listener(
        std::shared_ptr<input::CursorListener> const& wrapped);
/** @} */

    CachedPtr<frontend::Connector>   connector;
    CachedPtr<frontend::Connector>   wayland_connector;
    CachedPtr<frontend::Connector>   xwayland_connector;

    CachedPtr<input::InputReport> input_report;
    CachedPtr<input::EventFilterChainDispatcher> event_filter_chain_dispatcher;
    CachedPtr<input::CompositeEventFilter> composite_event_filter;
    CachedPtr<input::InputManager>    input_manager;
    CachedPtr<input::SurfaceInputDispatcher>    surface_input_dispatcher;
    CachedPtr<input::DefaultInputDeviceHub>    default_input_device_hub;
    CachedPtr<input::InputDeviceHub>    input_device_hub;
    CachedPtr<dispatch::MultiplexingDispatchable> input_reading_multiplexer;
    CachedPtr<input::InputDispatcher> input_dispatcher;
    CachedPtr<shell::InputTargeter> input_targeter;
    CachedPtr<input::CursorListener> cursor_listener;
    CachedPtr<input::TouchVisualizer> touch_visualizer;
    CachedPtr<input::Seat> seat;
    CachedPtr<graphics::GraphicBufferAllocator> buffer_allocator;
    CachedPtr<graphics::Display>      display;
    CachedPtr<graphics::Cursor>       cursor;
    CachedPtr<graphics::CursorImage>  default_cursor_image;
    CachedPtr<input::CursorImages> cursor_images;

    CachedPtr<frontend::SessionAuthorizer> session_authorizer;
    CachedPtr<renderer::RendererFactory> renderer_factory;
    CachedPtr<compositor::BufferStreamFactory> buffer_stream_factory;
    CachedPtr<scene::SurfaceStack> scene_surface_stack;
    CachedPtr<shell::SurfaceStack> surface_stack;
    CachedPtr<scene::SceneReport> scene_report;

    CachedPtr<scene::SurfaceFactory> surface_factory;
    CachedPtr<scene::SessionContainer>  session_container;
    CachedPtr<scene::SessionListener> session_listener;
    CachedPtr<scene::Clipboard>         clipboard;
    CachedPtr<scene::TextInputHub>      text_input_hub;
    CachedPtr<scene::IdleHub>           idle_hub;
    CachedPtr<shell::IdleHandler>       idle_handler;
    CachedPtr<shell::DisplayLayout>     shell_display_layout;
    CachedPtr<compositor::DisplayBufferCompositorFactory> display_buffer_compositor_factory;
    CachedPtr<compositor::Compositor> compositor;
    CachedPtr<compositor::CompositorReport> compositor_report;
    CachedPtr<compositor::ScreenShooter> screen_shooter;
    CachedPtr<logging::Logger> logger;
    CachedPtr<graphics::DisplayReport> display_report;
    CachedPtr<time::Clock> clock;
    CachedPtr<MainLoop> main_loop;
    CachedPtr<ServerStatusListener> server_status_listener;
    CachedPtr<graphics::DisplayConfigurationPolicy> display_configuration_policy;
    CachedPtr<scene::MediatingDisplayChanger> mediating_display_changer;
    CachedPtr<graphics::GLConfig> gl_config;
    CachedPtr<scene::PromptSessionListener> prompt_session_listener;
    CachedPtr<scene::PromptSessionManager> prompt_session_manager;
    CachedPtr<scene::SessionCoordinator> session_coordinator;
    CachedPtr<EmergencyCleanup> emergency_cleanup;
    CachedPtr<shell::PersistentSurfaceStore> persistent_surface_store;
    CachedPtr<SharedLibraryProberReport> shared_library_prober_report;
    CachedPtr<shell::Shell> shell;
    CachedPtr<shell::ShellReport> shell_report;
    CachedPtr<shell::decoration::Manager> decoration_manager;
    CachedPtr<scene::ApplicationNotRespondingDetector> application_not_responding_detector;
    CachedPtr<cookie::Authority> cookie_authority;
    CachedPtr<input::KeyMapper> key_mapper;
    std::shared_ptr<ConsoleServices> console_services;

private:
    std::shared_ptr<options::Configuration> const configuration_options;
    std::shared_ptr<input::EventFilter> default_filter;
    CachedPtr<ObserverMultiplexer<graphics::DisplayConfigurationObserver>>
        display_configuration_observer_multiplexer;
    CachedPtr<ObserverMultiplexer<input::SeatObserver>>
        seat_observer_multiplexer;

    // The following caches and factory functions are internal to the
    // default implementations of corresponding the Mir components
    CachedPtr<scene::BroadcastingSessionEventSink> broadcasting_session_event_sink;

    std::shared_ptr<scene::BroadcastingSessionEventSink> the_broadcasting_session_event_sink();

    auto report_factory(char const* report_opt) -> std::unique_ptr<report::ReportFactory>;

    std::vector<WaylandExtensionHook> wayland_extension_hooks;
    WaylandProtocolExtensionFilter wayland_extension_filter =
        [](std::shared_ptr<scene::Session> const&, char const*) { return true; };
    std::vector<std::string> enabled_wayland_extensions;

    // Helpers for platform library loading
    std::vector<std::shared_ptr<mir::SharedLibrary>> platform_libraries;
    auto the_platform_libaries() -> std::vector<std::shared_ptr<mir::SharedLibrary>> const&;

    std::vector<std::shared_ptr<graphics::DisplayPlatform>> display_platforms;
    std::vector<std::shared_ptr<graphics::RenderingPlatform>> rendering_platforms;
};
}


#endif /* MIR_DEFAULT_SERVER_CONFIGURATION_H_ */
