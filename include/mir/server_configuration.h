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

#include <memory>
#include <string>

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
}
namespace graphics
{
class Renderer;
class Platform;
}

class ServerConfiguration
{
public:
    // the communicator to use
    virtual std::shared_ptr<frontend::Communicator>
    make_communicator(std::shared_ptr<compositor::BufferAllocationStrategy> const& buffer_allocation_strategy) = 0;

    // the renderer to use
    virtual std::shared_ptr<graphics::Renderer> make_renderer() = 0;

    // the allocator strategy to use
    virtual std::shared_ptr<compositor::BufferAllocationStrategy> make_buffer_allocation_strategy() = 0;

    virtual std::shared_ptr<graphics::Platform> make_graphics_platform() = 0;


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

    // the communicator to use
    virtual std::shared_ptr<frontend::Communicator>
    make_communicator(std::shared_ptr<compositor::BufferAllocationStrategy> const& buffer_allocation_strategy);

    // the renderer to use
    virtual std::shared_ptr<graphics::Renderer> make_renderer();

    // the allocator strategy to use
    virtual std::shared_ptr<compositor::BufferAllocationStrategy> make_buffer_allocation_strategy();

    virtual std::shared_ptr<graphics::Platform> make_graphics_platform();
private:
    std::string socket_file;
    std::shared_ptr<graphics::Platform> graphics_platform;

    // the communications interface to use
    virtual std::shared_ptr<frontend::ProtobufIpcFactory> make_ipc_factory(
        std::shared_ptr<compositor::BufferAllocationStrategy> const& buffer_allocation_strategy);

    // the graphic buffer source to use
    virtual std::shared_ptr<compositor::GraphicBufferAllocator> make_graphic_buffer_allocator();
};
}


#endif /* MIR_SERVER_CONFIGURATION_H_ */
