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
class OverlayRenderer;
class RendererFactory;
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
}

namespace shell
{
class SurfaceFactory;
class SurfaceBuilder;
class SurfaceController;
class InputTargeter;
class SessionContainer;
class FocusSetter;
class FocusSequence;
class PlacementStrategy;
class SessionListener;
class FocusController;
class SessionManager;
class PixelBuffer;
class SnapshotStrategy;
class DisplayLayout;
class SurfaceConfigurator;
class MediatingDisplayChanger;
class SessionEventSink;
class SessionEventHandlerRegister;
class BroadcastingSessionEventSink;
}
namespace time
{
class TimeSource;
}
namespace surfaces
{
class BufferStreamFactory;
class SurfaceStackModel;
class SurfaceStack;
class SurfaceController;
class InputRegistrar;
class SurfacesReport;
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
}

namespace logging
{
class Logger;
}

class DefaultServerConfiguration : public virtual ServerConfiguration, DefaultConfigurationOptions
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
    virtual std::shared_ptr<input::NestedInputRelay> the_nested_input_relay();
    /** @} */

    /** @name graphics configuration - dependencies
     * dependencies of graphics on the rest of the Mir
     *  @{ */
    virtual std::shared_ptr<graphics::DisplayReport> the_display_report();
    /** @} */

    /** @name compositor configuration - customization
     * configurable interfaces for modifying compositor
     *  @{ */
    virtual std::shared_ptr<compositor::DisplayBufferCompositorFactory> the_display_buffer_compositor_factory();
    virtual std::shared_ptr<compositor::OverlayRenderer>          the_overlay_renderer();
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
    virtual std::shared_ptr<shell::SessionContainer>    the_shell_session_container();
    virtual std::shared_ptr<shell::FocusSetter>         the_shell_focus_setter();
    virtual std::shared_ptr<shell::FocusSequence>       the_shell_focus_sequence();
    virtual std::shared_ptr<shell::PlacementStrategy>   the_shell_placement_strategy();
    virtual std::shared_ptr<shell::SessionListener>     the_shell_session_listener();
    virtual std::shared_ptr<shell::PixelBuffer>         the_shell_pixel_buffer();
    virtual std::shared_ptr<shell::SnapshotStrategy>    the_shell_snapshot_strategy();
    virtual std::shared_ptr<shell::DisplayLayout>       the_shell_display_layout();
    virtual std::shared_ptr<shell::SurfaceConfigurator> the_shell_surface_configurator();
    virtual std::shared_ptr<shell::SessionEventSink>    the_shell_session_event_sink();
    virtual std::shared_ptr<shell::SessionEventHandlerRegister> the_shell_session_event_handler_register();
    virtual std::shared_ptr<shell::SurfaceController>   the_shell_surface_controller();
    /** @} */

    /** @name shell configuration - dependencies
     * dependencies of shell on the rest of the Mir
     *  @{ */
    virtual std::shared_ptr<shell::SurfaceBuilder>     the_surface_builder();
    virtual std::shared_ptr<surfaces::SurfaceController>     the_surface_controller();

    /** @} */


    /** @name surfaces configuration - customization
     * configurable interfaces for modifying surfaces
     *  @{ */
    virtual std::shared_ptr<surfaces::SurfaceStackModel> the_surface_stack_model();
    virtual std::shared_ptr<surfaces::SurfacesReport>    the_surfaces_report();
    /** @} */

    /** @name surfaces configuration - dependencies
     * dependencies of surfaces on the rest of the Mir
     *  @{ */
    virtual std::shared_ptr<surfaces::BufferStreamFactory> the_buffer_stream_factory();
    /** @} */


    /** @name input configuration
     *  @{ */
    virtual std::shared_ptr<input::InputReport> the_input_report();
    virtual std::shared_ptr<input::CompositeEventFilter> the_composite_event_filter();
    virtual std::shared_ptr<surfaces::InputRegistrar> the_input_registrar();
    virtual std::shared_ptr<shell::InputTargeter> the_input_targeter();
    virtual std::shared_ptr<input::CursorListener> the_cursor_listener();
    virtual std::shared_ptr<input::InputRegion>    the_input_region();
    /** @} */

    /** @name logging configuration - customization
     * configurable interfaces for modifying logging
     *  @{ */
    virtual std::shared_ptr<logging::Logger> the_logger();
    /** @} */

    virtual std::shared_ptr<time::TimeSource>    the_time_source();

    virtual std::shared_ptr<shell::SessionManager> the_session_manager();

protected:
    using DefaultConfigurationOptions::the_options;
    using DefaultConfigurationOptions::add_options;

    virtual std::shared_ptr<input::InputChannelFactory> the_input_channel_factory();
    virtual std::shared_ptr<shell::MediatingDisplayChanger> the_mediating_display_changer();
    virtual std::shared_ptr<shell::BroadcastingSessionEventSink> the_broadcasting_session_event_sink();

    CachedPtr<frontend::Connector>   connector;
    CachedPtr<shell::SessionManager> session_manager;


    CachedPtr<input::InputConfiguration> input_configuration;

    CachedPtr<input::InputReport> input_report;
    CachedPtr<input::CompositeEventFilter> composite_event_filter;
    CachedPtr<input::InputManager>    input_manager;
    CachedPtr<input::InputRegion>     input_region;
    CachedPtr<surfaces::InputRegistrar> input_registrar;
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
    CachedPtr<compositor::RendererFactory> renderer_factory;
    CachedPtr<compositor::BufferStreamFactory> buffer_stream_factory;
    CachedPtr<surfaces::SurfaceStack> surface_stack;
    CachedPtr<surfaces::SurfacesReport> surfaces_report;

    CachedPtr<shell::SurfaceFactory> shell_surface_factory;
    CachedPtr<shell::SessionContainer>  shell_session_container;
    CachedPtr<shell::FocusSetter>       shell_focus_setter;
    CachedPtr<shell::FocusSequence>     shell_focus_sequence;
    CachedPtr<shell::PlacementStrategy> shell_placement_strategy;
    CachedPtr<shell::SessionListener> shell_session_listener;
    CachedPtr<shell::PixelBuffer>       shell_pixel_buffer;
    CachedPtr<shell::SnapshotStrategy>  shell_snapshot_strategy;
    CachedPtr<shell::DisplayLayout>     shell_display_layout;
    CachedPtr<shell::SurfaceConfigurator> shell_surface_configurator;
    CachedPtr<compositor::DisplayBufferCompositorFactory> display_buffer_compositor_factory;
    CachedPtr<compositor::OverlayRenderer> overlay_renderer;
    CachedPtr<compositor::Compositor> compositor;
    CachedPtr<logging::Logger> logger;
    CachedPtr<graphics::DisplayReport> display_report;
    CachedPtr<surfaces::SurfaceController> surface_controller;
    CachedPtr<time::TimeSource> time_source;
    CachedPtr<MainLoop> main_loop;
    CachedPtr<ServerStatusListener> server_status_listener;
    CachedPtr<graphics::DisplayConfigurationPolicy> display_configuration_policy;
    CachedPtr<graphics::nested::HostConnection> host_connection;
    CachedPtr<input::NestedInputRelay> nested_input_relay;
    CachedPtr<shell::MediatingDisplayChanger> mediating_display_changer;
    CachedPtr<shell::BroadcastingSessionEventSink> broadcasting_session_event_sink;

private:
    std::shared_ptr<input::EventFilter> const default_filter;

    // the communications interface to use
    virtual std::shared_ptr<frontend::ProtobufIpcFactory> the_ipc_factory(
        std::shared_ptr<frontend::Shell> const& shell,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator);

    virtual std::string the_socket_file() const;
};
}


#endif /* MIR_DEFAULT_SERVER_CONFIGURATION_H_ */
