/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */
#ifndef MIR_DEFAULT_SERVER_CONFIGURATION_H_
#define MIR_DEFAULT_SERVER_CONFIGURATION_H_

#include "mir/cached_ptr.h"
#include "mir/server_configuration.h"
#include "mir/default_configuration_options.h"

#include <memory>
#include <string>

namespace mir
{
namespace compositor
{
class Renderer;
class BufferStreamFactory;
class Scene;
class Drawer;
class DisplayBufferCompositorFactory;
class Compositor;
class RendererFactory;
class CompositorReport;
}
namespace frontend
{
class Shell;
class Connector;
class ConnectorReport;
class ProtobufIpcFactory;
class SessionCreator;
class SessionMediatorReport;
class MessageProcessorReport;
class SessionAuthorizer;
class EventSink;
class DisplayChanger;
class Screencast;
}

namespace shell
{
class SurfaceFactory;
class InputTargeter;
class FocusSetter;
class PlacementStrategy;
class SessionListener;
class FocusController;
class DisplayLayout;
class SurfaceConfigurator;
}
namespace time
{
class Clock;
}
namespace scene
{
class BroadcastingSessionEventSink;
class BufferStreamFactory;
class MediatingDisplayChanger;
class PixelBuffer;
class SessionContainer;
class SessionEventSink;
class SessionEventHandlerRegister;
class SessionManager;
class SnapshotStrategy;
class SurfaceBuilder;
class SurfaceStackModel;
class SurfaceStack;
class SurfaceRanker;
class SurfaceController;
class InputRegistrar;
class SceneReport;
}
namespace graphics
{
class Platform;
class Display;
class BufferInitializer;
class DisplayReport;
class GraphicBufferAllocator;
namespace nested { class HostConnection; }
}
namespace input
{
class InputReport;
class InputManager;
class CompositeEventFilter;
class InputChannelFactory;
class InputConfiguration;
class CursorListener;
class InputRegion;
class NestedInputRelay;
class EventHandler;
}

namespace logging
{
class Logger;
}

class DefaultServerConfiguration : public virtual ServerConfiguration, protected DefaultConfigurationOptions
{
public:
    DefaultServerConfiguration(int argc, char const* argv[]);

    /** @name DisplayServer dependencies
     * dependencies of DisplayServer on the rest of the Mir
     *  @{ */
    virtual std::shared_ptr<frontend::Connector>    the_connector();
    virtual std::shared_ptr<graphics::Display>      the_display();
    virtual std::shared_ptr<compositor::Compositor> the_compositor();
    virtual std::shared_ptr<input::InputManager>    the_input_manager();
    virtual std::shared_ptr<MainLoop>               the_main_loop();
    virtual std::shared_ptr<ServerStatusListener>   the_server_status_listener();
    virtual std::shared_ptr<DisplayChanger>         the_display_changer();
    virtual std::shared_ptr<graphics::Platform>     the_graphics_platform();
    virtual std::shared_ptr<input::InputConfiguration> the_input_configuration();
    /** @} */

    /** @name graphics configuration - customization
     * configurable interfaces for modifying graphics
     *  @{ */
    virtual std::shared_ptr<graphics::BufferInitializer> the_buffer_initializer();
    virtual std::shared_ptr<compositor::RendererFactory>   the_renderer_factory();
    virtual std::shared_ptr<graphics::DisplayConfigurationPolicy> the_display_configuration_policy();
    virtual std::shared_ptr<graphics::nested::HostConnection> the_host_connection();
    virtual std::shared_ptr<input::EventFilter> the_nested_event_filter();
    /** @} */

    /** @name graphics configuration - dependencies
     * dependencies of graphics on the rest of the Mir
     *  @{ */
    virtual std::shared_ptr<graphics::DisplayReport> the_display_report();
    /** @} */

    /** @name compositor configuration - customization
     * configurable interfaces for modifying compositor
     *  @{ */
    virtual std::shared_ptr<compositor::CompositorReport> the_compositor_report();
    virtual std::shared_ptr<compositor::DisplayBufferCompositorFactory> the_display_buffer_compositor_factory();
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
    virtual std::shared_ptr<frontend::SessionMediatorReport>  the_session_mediator_report();
    virtual std::shared_ptr<frontend::MessageProcessorReport> the_message_processor_report();
    virtual std::shared_ptr<frontend::SessionAuthorizer>      the_session_authorizer();
    virtual std::shared_ptr<frontend::Shell>                  the_frontend_shell();
    virtual std::shared_ptr<frontend::EventSink>              the_global_event_sink();
    virtual std::shared_ptr<frontend::DisplayChanger>         the_frontend_display_changer();
    virtual std::shared_ptr<frontend::Screencast>             the_screencast();
    /** @name frontend configuration - internal dependencies
     * internal dependencies of frontend
     *  @{ */
    virtual std::shared_ptr<frontend::SessionCreator>         the_session_creator();
    virtual std::shared_ptr<frontend::ConnectorReport>        the_connector_report();
    /** @} */
    /** @} */

    virtual std::shared_ptr<shell::FocusController> the_focus_controller();

    /** @name shell configuration - customization
     * configurable interfaces for modifying shell
     *  @{ */
    virtual std::shared_ptr<shell::SurfaceFactory>      the_shell_surface_factory();
    virtual std::shared_ptr<shell::SurfaceFactory>      the_scene_surface_factory();
    virtual std::shared_ptr<shell::FocusSetter>         the_shell_focus_setter();
    virtual std::shared_ptr<shell::PlacementStrategy>   the_shell_placement_strategy();
    virtual std::shared_ptr<shell::SessionListener>     the_shell_session_listener();
    virtual std::shared_ptr<shell::DisplayLayout>       the_shell_display_layout();
    virtual std::shared_ptr<shell::SurfaceConfigurator> the_shell_surface_configurator();
    /** @} */

    /** @name internal scene configuration
     * builder functions used in the default implementation.
     * The interfaces returned are not published, so the functions are only useful in tests
     *  @{ */
    virtual std::shared_ptr<scene::PixelBuffer>       the_pixel_buffer();
    virtual std::shared_ptr<scene::SnapshotStrategy>  the_snapshot_strategy();
    virtual std::shared_ptr<scene::SessionContainer>  the_session_container();
    virtual std::shared_ptr<scene::SessionEventSink>  the_session_event_sink();
    virtual std::shared_ptr<scene::SessionEventHandlerRegister> the_session_event_handler_register();
    virtual std::shared_ptr<scene::SurfaceStackModel> the_surface_stack_model();
    virtual std::shared_ptr<scene::SurfaceRanker>   the_surface_ranker();
    /** @} */

    /** @name scene configuration - dependencies
     * dependencies of scene on the rest of the Mir
     *  @{ */
    virtual std::shared_ptr<scene::BufferStreamFactory> the_buffer_stream_factory();
    virtual std::shared_ptr<scene::SceneReport>      the_scene_report();
    /** @} */


    /** @name input configuration
     *  @{ */
    virtual std::shared_ptr<input::InputReport> the_input_report();
    virtual std::shared_ptr<input::CompositeEventFilter> the_composite_event_filter();
    virtual std::shared_ptr<scene::InputRegistrar> the_input_registrar();
    virtual std::shared_ptr<shell::InputTargeter> the_input_targeter();
    virtual std::shared_ptr<input::CursorListener> the_cursor_listener();
    virtual std::shared_ptr<input::InputRegion>    the_input_region();
    /** @} */

    /** @name logging configuration - customization
     * configurable interfaces for modifying logging
     *  @{ */
    virtual std::shared_ptr<logging::Logger> the_logger();
    /** @} */

    virtual std::shared_ptr<time::Clock> the_clock();

protected:
    using DefaultConfigurationOptions::the_options;
    using DefaultConfigurationOptions::add_options;
    using DefaultConfigurationOptions::parse_options;

    virtual std::shared_ptr<input::InputChannelFactory> the_input_channel_factory();
    virtual std::shared_ptr<scene::MediatingDisplayChanger> the_mediating_display_changer();
    virtual std::shared_ptr<frontend::ProtobufIpcFactory> the_ipc_factory(
        std::shared_ptr<frontend::Shell> const& shell,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator);

    CachedPtr<frontend::Connector>   connector;

    CachedPtr<input::InputConfiguration> input_configuration;

    CachedPtr<input::InputReport> input_report;
    CachedPtr<input::CompositeEventFilter> composite_event_filter;
    CachedPtr<input::InputManager>    input_manager;
    CachedPtr<input::InputRegion>     input_region;
    CachedPtr<scene::InputRegistrar> input_registrar;
    CachedPtr<shell::InputTargeter> input_targeter;
    CachedPtr<input::CursorListener> cursor_listener;
    CachedPtr<graphics::Platform>     graphics_platform;
    CachedPtr<graphics::BufferInitializer> buffer_initializer;
    CachedPtr<graphics::GraphicBufferAllocator> buffer_allocator;
    CachedPtr<graphics::Display>      display;

    CachedPtr<frontend::ConnectorReport>   connector_report;
    CachedPtr<frontend::ProtobufIpcFactory>  ipc_factory;
    CachedPtr<frontend::SessionMediatorReport> session_mediator_report;
    CachedPtr<frontend::MessageProcessorReport> message_processor_report;
    CachedPtr<frontend::SessionAuthorizer> session_authorizer;
    CachedPtr<frontend::EventSink> global_event_sink;
    CachedPtr<frontend::SessionCreator>    session_creator;
    CachedPtr<frontend::Screencast> screencast;
    CachedPtr<compositor::RendererFactory> renderer_factory;
    CachedPtr<compositor::BufferStreamFactory> buffer_stream_factory;
    CachedPtr<scene::SurfaceStack> surface_stack;
    CachedPtr<scene::SceneReport> scene_report;

    CachedPtr<shell::SurfaceFactory> shell_surface_factory;
    CachedPtr<shell::SurfaceFactory> scene_surface_factory;
    CachedPtr<scene::SessionContainer>  session_container;
    CachedPtr<shell::FocusSetter>       shell_focus_setter;
    CachedPtr<shell::PlacementStrategy> shell_placement_strategy;
    CachedPtr<shell::SessionListener> shell_session_listener;
    CachedPtr<scene::PixelBuffer>       pixel_buffer;
    CachedPtr<scene::SnapshotStrategy>  snapshot_strategy;
    CachedPtr<shell::DisplayLayout>     shell_display_layout;
    CachedPtr<shell::SurfaceConfigurator> shell_surface_configurator;
    CachedPtr<compositor::DisplayBufferCompositorFactory> display_buffer_compositor_factory;
    CachedPtr<compositor::Compositor> compositor;
    CachedPtr<compositor::CompositorReport> compositor_report;
    CachedPtr<logging::Logger> logger;
    CachedPtr<graphics::DisplayReport> display_report;
    CachedPtr<time::Clock> clock;
    CachedPtr<MainLoop> main_loop;
    CachedPtr<ServerStatusListener> server_status_listener;
    CachedPtr<graphics::DisplayConfigurationPolicy> display_configuration_policy;
    CachedPtr<graphics::nested::HostConnection> host_connection;
    CachedPtr<scene::MediatingDisplayChanger> mediating_display_changer;

private:
    std::shared_ptr<input::EventFilter> const default_filter;

    virtual std::string the_socket_file() const;

    // The following caches and factory functions are internal to the
    // default implementations of corresponding the Mir components
    CachedPtr<scene::BroadcastingSessionEventSink> broadcasting_session_event_sink;
    CachedPtr<input::NestedInputRelay> nested_input_relay;
    CachedPtr<scene::SurfaceController> surface_controller;
    CachedPtr<scene::SessionManager> session_manager;

    std::shared_ptr<scene::BroadcastingSessionEventSink> the_broadcasting_session_event_sink();
    std::shared_ptr<input::NestedInputRelay>        the_nested_input_relay();
    std::shared_ptr<scene::SessionManager>       the_session_manager();
    std::shared_ptr<scene::SurfaceBuilder>       the_surface_builder();
    std::shared_ptr<scene::SurfaceController>    the_surface_controller();
};
}


#endif /* MIR_DEFAULT_SERVER_CONFIGURATION_H_ */
