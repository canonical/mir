/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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
#ifndef MIR_SERVER_CONFIGURATION_H_
#define MIR_SERVER_CONFIGURATION_H_

#include "mir/options/program_option.h"

#include <memory>
#include <string>
#include <initializer_list>

namespace mir
{
namespace compositor
{
class BufferAllocationStrategy;
class GraphicBufferAllocator;
}
namespace frontend
{
class Communicator;
class ProtobufIpcFactory;
class ApplicationListener;
class SessionManager;
class SurfaceOrganiser;
}
namespace graphics
{
class Renderer;
class Platform;
class Display;
class ViewableArea;
class BufferInitializer;
}
namespace input
{
class InputManager;
class EventFilter;
}

class ServerConfiguration
{
public:
    virtual std::shared_ptr<options::Option> make_options() = 0;
    virtual std::shared_ptr<graphics::Platform> make_graphics_platform() = 0;
    virtual std::shared_ptr<graphics::BufferInitializer> make_buffer_initializer() = 0;
    virtual std::shared_ptr<compositor::BufferAllocationStrategy> make_buffer_allocation_strategy(
        std::shared_ptr<compositor::GraphicBufferAllocator> const& buffer_allocator) = 0;
    virtual std::shared_ptr<graphics::Renderer> make_renderer(
        std::shared_ptr<graphics::Display> const& display) = 0;
    virtual std::shared_ptr<frontend::Communicator> make_communicator(
        std::shared_ptr<frontend::SurfaceOrganiser> const& surface_organiser, std::shared_ptr<graphics::Display> const& display) = 0;
    virtual std::shared_ptr<frontend::SessionManager> make_session_manager(
        std::shared_ptr<frontend::SurfaceOrganiser> const& surface_organiser) = 0;
    virtual std::shared_ptr<input::InputManager> make_input_manager(
        const std::initializer_list<std::shared_ptr<input::EventFilter> const>& event_filters, 
        std::shared_ptr<graphics::ViewableArea> const& viewable_area) = 0;

protected:
    ServerConfiguration() = default;
    virtual ~ServerConfiguration() {}

    ServerConfiguration(ServerConfiguration const&) = delete;
    ServerConfiguration& operator=(ServerConfiguration const&) = delete;
};

class DefaultServerConfiguration : public ServerConfiguration
{
public:
    DefaultServerConfiguration(std::string const& socket_file);

    virtual std::shared_ptr<options::Option> make_options();
    virtual std::shared_ptr<graphics::Platform> make_graphics_platform();
    virtual std::shared_ptr<graphics::BufferInitializer> make_buffer_initializer();
    virtual std::shared_ptr<compositor::BufferAllocationStrategy> make_buffer_allocation_strategy(
        std::shared_ptr<compositor::GraphicBufferAllocator> const& buffer_allocator);
    virtual std::shared_ptr<graphics::Renderer> make_renderer(
        std::shared_ptr<graphics::Display> const& display);
    virtual std::shared_ptr<frontend::Communicator> make_communicator(
        std::shared_ptr<frontend::SurfaceOrganiser> const& surface_organiser, 
        std::shared_ptr<graphics::Display> const& display);
    virtual std::shared_ptr<frontend::SessionManager> make_session_manager(
        std::shared_ptr<frontend::SurfaceOrganiser> const& surface_organiser);
    virtual std::shared_ptr<input::InputManager> make_input_manager(
        const std::initializer_list<std::shared_ptr<input::EventFilter> const>& event_filters, 
        std::shared_ptr<graphics::ViewableArea> const& viewable_area);

private:
    std::string socket_file;
    std::shared_ptr<options::Option> options;
    std::shared_ptr<graphics::Platform> graphics_platform;
    std::shared_ptr<frontend::ApplicationListener> application_listener;

    // the communications interface to use
    virtual std::shared_ptr<frontend::ProtobufIpcFactory> make_ipc_factory(
        std::shared_ptr<frontend::SurfaceOrganiser> const& surface_organiser,
        std::shared_ptr<graphics::Display> const& display);

    virtual std::shared_ptr<frontend::ApplicationListener> make_application_listener();
};
}


#endif /* MIR_SERVER_CONFIGURATION_H_ */
