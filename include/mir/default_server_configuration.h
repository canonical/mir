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
class Compositor;
}
namespace frontend
{
class Communicator;
class ProtobufIpcFactory;
class ApplicationMediatorReport;
}

namespace sessions
{
class SessionStore;
class SurfaceFactory;
}
namespace surfaces
{
class BufferBundleFactory;
class SurfaceController;
class SurfaceStackModel;
class SurfaceStack;
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

    virtual std::shared_ptr<compositor::Drawer> the_drawer();
    virtual std::shared_ptr<compositor::BufferAllocationStrategy> the_buffer_allocation_strategy();
    virtual std::shared_ptr<compositor::GraphicBufferAllocator> the_buffer_allocator();
    virtual std::shared_ptr<compositor::RenderView> the_render_view();

    virtual std::shared_ptr<frontend::Communicator> the_communicator();
    virtual std::shared_ptr<frontend::ApplicationMediatorReport> the_application_mediator_report();

    virtual std::shared_ptr<sessions::SessionStore> the_session_store();
    virtual std::shared_ptr<sessions::SurfaceFactory> the_surface_factory();

    virtual std::shared_ptr<surfaces::BufferBundleFactory> the_buffer_bundle_factory();
    virtual std::shared_ptr<surfaces::SurfaceStackModel> the_surface_stack_model();

    virtual std::shared_ptr<logging::Logger> the_logger();

    virtual std::shared_ptr<input::InputManager> the_input_manager(
        const std::initializer_list<std::shared_ptr<input::EventFilter> const>& event_filters);

protected:
    virtual std::shared_ptr<options::Option> the_options() const;

    mir::CachedPtr<frontend::Communicator> communicator;
    mir::CachedPtr<sessions::SessionStore> session_store;
    mir::CachedPtr<input::InputManager>    input_manager;
    mir::CachedPtr<graphics::Platform>     graphics_platform;
    mir::CachedPtr<graphics::BufferInitializer> buffer_initializer;
    mir::CachedPtr<compositor::GraphicBufferAllocator> buffer_allocator;
    mir::CachedPtr<graphics::Display>      display;

    mir::CachedPtr<frontend::ProtobufIpcFactory>  ipc_factory;
    mir::CachedPtr<frontend::ApplicationMediatorReport> application_mediator_report;
    mir::CachedPtr<compositor::BufferAllocationStrategy> buffer_allocation_strategy;
    mir::CachedPtr<graphics::Renderer> renderer;
    mir::CachedPtr<compositor::BufferBundleManager> buffer_bundle_manager;
    mir::CachedPtr<surfaces::SurfaceStack> surface_stack;
    mir::CachedPtr<surfaces::SurfaceController> surface_controller;
    mir::CachedPtr<compositor::Compositor> compositor;
    mir::CachedPtr<logging::Logger> logger;
    mir::CachedPtr<graphics::DisplayReport> display_report;

private:
    std::shared_ptr<options::Option> options;

    // the communications interface to use
    virtual std::shared_ptr<frontend::ProtobufIpcFactory> the_ipc_factory(
        std::shared_ptr<sessions::SessionStore> const& session_store,
        std::shared_ptr<graphics::ViewableArea> const& display,
        std::shared_ptr<compositor::GraphicBufferAllocator> const& allocator);

    virtual std::string the_socket_file() const;
};
}


#endif /* MIR_DEFAULT_SERVER_CONFIGURATION_H_ */
