/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */
#ifndef MIR_DEFAULT_SERVER_CONFIGURATION_H_
#define MIR_DEFAULT_SERVER_CONFIGURATION_H_

#include "mir/cached_ptr.h"
#include "mir/server_configuration.h"
#include "mir/options/program_option.h"

#include <memory>
#include <string>

namespace mir
{
namespace compositor
{
class BufferAllocationStrategy;
class GraphicBufferAllocator;
class BufferBundleManager;
class RenderView;
class Drawer;
class CompositingStrategy;
class Compositor;
}
namespace frontend
{
class Shell;
class Communicator;
class ProtobufIpcFactory;
class SessionMediatorReport;
class MessageProcessorReport;
}

namespace shell
{
class SessionManager;
class SurfaceFactory;
class SurfaceSource;
class SurfaceBuilder;
class InputFocusSelector;
}
namespace time
{
class TimeSource;
}
namespace surfaces
{
class BufferBundleFactory;
class SurfaceStackModel;
class SurfaceStack;
class SurfaceController;
}
namespace graphics
{
class Renderer;
class Platform;
class Display;
class ViewableArea;
class BufferInitializer;
class DisplayReport;
}
namespace input
{
class InputManager;
class EventFilter;
class InputChannelFactory;
}

namespace logging
{
class Logger;
}

class DefaultServerConfiguration : public ServerConfiguration
{
public:
    DefaultServerConfiguration(int argc, char const* argv[]);

    virtual std::shared_ptr<graphics::Display> the_display();
    virtual std::shared_ptr<graphics::DisplayReport> the_display_report();
    virtual std::shared_ptr<graphics::Platform> the_graphics_platform();
    virtual std::shared_ptr<graphics::BufferInitializer> the_buffer_initializer();
    virtual std::shared_ptr<graphics::Renderer> the_renderer();
    virtual std::shared_ptr<graphics::ViewableArea> the_viewable_area();

    virtual std::shared_ptr<compositor::Compositor> the_compositor();
    virtual std::shared_ptr<compositor::CompositingStrategy> the_compositing_strategy();
    virtual std::shared_ptr<compositor::BufferAllocationStrategy> the_buffer_allocation_strategy();
    virtual std::shared_ptr<compositor::GraphicBufferAllocator> the_buffer_allocator();
    virtual std::shared_ptr<compositor::RenderView> the_render_view();

    virtual std::shared_ptr<frontend::Communicator> the_communicator();
    virtual std::shared_ptr<frontend::SessionMediatorReport> the_session_mediator_report();
    virtual std::shared_ptr<frontend::MessageProcessorReport> the_message_processor_report();

    virtual std::shared_ptr<frontend::Shell> the_frontend_shell();
    virtual std::shared_ptr<shell::SurfaceFactory> the_surface_factory();

    virtual std::shared_ptr<surfaces::BufferBundleFactory> the_buffer_bundle_factory();
    virtual std::shared_ptr<surfaces::SurfaceStackModel> the_surface_stack_model();

    virtual std::shared_ptr<logging::Logger> the_logger();

    virtual std::initializer_list<std::shared_ptr<input::EventFilter> const> the_event_filters();
    virtual std::shared_ptr<input::InputManager> the_input_manager();
    virtual std::shared_ptr<shell::InputFocusSelector> the_input_focus_selector();

    virtual std::shared_ptr<shell::SurfaceBuilder> the_surface_builder();
    virtual std::shared_ptr<time::TimeSource> the_time_source();

protected:
    virtual std::shared_ptr<options::Option> the_options() const;
    virtual std::shared_ptr<input::InputChannelFactory> the_input_channel_factory();

    CachedPtr<frontend::Communicator> communicator;
    CachedPtr<frontend::Shell> session_manager;
    CachedPtr<input::InputManager>    input_manager;
    CachedPtr<graphics::Platform>     graphics_platform;
    CachedPtr<graphics::BufferInitializer> buffer_initializer;
    CachedPtr<compositor::GraphicBufferAllocator> buffer_allocator;
    CachedPtr<graphics::Display>      display;

    CachedPtr<frontend::ProtobufIpcFactory>  ipc_factory;
    CachedPtr<frontend::SessionMediatorReport> session_mediator_report;
    CachedPtr<frontend::MessageProcessorReport> message_processor_report;
    CachedPtr<compositor::BufferAllocationStrategy> buffer_allocation_strategy;
    CachedPtr<graphics::Renderer> renderer;
    CachedPtr<compositor::BufferBundleManager> buffer_bundle_manager;
    CachedPtr<surfaces::SurfaceStack> surface_stack;
    CachedPtr<shell::SurfaceSource> surface_source;
    CachedPtr<compositor::CompositingStrategy> compositing_strategy;
    CachedPtr<compositor::Compositor> compositor;
    CachedPtr<logging::Logger> logger;
    CachedPtr<graphics::DisplayReport> display_report;
    CachedPtr<surfaces::SurfaceController> surface_controller;
    CachedPtr<time::TimeSource> time_source;

private:
    std::shared_ptr<options::Option> options;

    // the communications interface to use
    virtual std::shared_ptr<frontend::ProtobufIpcFactory> the_ipc_factory(
        std::shared_ptr<frontend::Shell> const& shell,
        std::shared_ptr<graphics::ViewableArea> const& display,
        std::shared_ptr<compositor::GraphicBufferAllocator> const& allocator);

    virtual std::string the_socket_file() const;
};
}


#endif /* MIR_DEFAULT_SERVER_CONFIGURATION_H_ */
