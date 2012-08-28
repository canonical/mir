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

#include "mir/server_configuration.h"

#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/frontend/protobuf_asio_communicator.h"
#include "mir/graphics/renderer.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/surfaces/surface.h"

#include "mir_protobuf.pb.h"

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;

namespace
{

// TODO replace with a real renderer appropriate to the platform default
class StubRenderer : public mg::Renderer
{
public:
    virtual void render(mg::Renderable&)
    {
    }
};

// TODO replace with a real buffer allocator appropriate to the platform default
class StubGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
 public:
    std::unique_ptr<mc::Buffer> alloc_buffer(geom::Width, geom::Height, mc::PixelFormat)
    {
        return std::unique_ptr<mc::Buffer>();
    }
};

class ApplicationProxy : public mir::protobuf::DisplayServer
{
public:

    ApplicationProxy(std::shared_ptr<ms::ApplicationSurfaceOrganiser> const& surface_organiser) :
    surface_organiser(surface_organiser)
    {
    }

private:
    void connect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::ConnectMessage* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        auto tmp = surface_organiser->create_surface(
            ms::SurfaceCreationParameters()
            .of_width(geom::Width(request->width()))
            .of_height(geom::Height(request->height()))
            );

        auto surface = tmp.lock();
        // TODO cannot interrogate the surface for its properties.

        // These need to be set for the response to be valid,
        // but they should derive from the surface we just created!
        response->set_width(surface->width().as_uint32_t());
        response->set_height(surface->height().as_uint32_t());
        response->set_pixel_format(request->pixel_format());

        done->Run();
    }

    void disconnect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::Void* /*request*/,
                 mir::protobuf::Void* /*response*/,
                 google::protobuf::Closure* done)
    {
        done->Run();
    }

    std::shared_ptr<ms::ApplicationSurfaceOrganiser> surface_organiser;
};

class DefaultIpcFactory : public mf::ProtobufIpcFactory
{
public:
    explicit DefaultIpcFactory(
        std::shared_ptr<ms::ApplicationSurfaceOrganiser> const& surface_organiser) :
        surface_organiser(surface_organiser)
    {
    }

private:
    std::shared_ptr<ms::ApplicationSurfaceOrganiser> surface_organiser;

    virtual std::shared_ptr<mir::protobuf::DisplayServer> make_ipc_server()
    {
        return std::make_shared<ApplicationProxy>(surface_organiser);
    }
};

// This (with make_ipc_factory()) builds a large chunk of the "inner" system.
// We may want to move this out of "configuration" someday
struct Surfaces :
    public mc::BufferBundleManager,
    public ms::SurfaceStack,
    public ms::SurfaceController
{
    Surfaces(std::shared_ptr<mc::BufferAllocationStrategy> const& strategy) :
        mc::BufferBundleManager(strategy),
        ms::SurfaceStack(this),
        ms::SurfaceController(this) {}
};
}

std::shared_ptr<mc::GraphicBufferAllocator> mir::DefaultServerConfiguration::make_graphic_buffer_allocator()
{
    return std::make_shared<StubGraphicBufferAllocator>();
}

std::shared_ptr<mc::BufferAllocationStrategy> mir::DefaultServerConfiguration::make_buffer_allocation_strategy()
{
    auto graphic_buffer_allocator = make_graphic_buffer_allocator();
    return std::make_shared<mc::DoubleBufferAllocationStrategy>(graphic_buffer_allocator);
}

std::shared_ptr<mg::Renderer> mir::DefaultServerConfiguration::make_renderer()
{
    return std::make_shared<StubRenderer>();
}

std::shared_ptr<mir::frontend::ProtobufIpcFactory>
mir::DefaultServerConfiguration::make_ipc_factory(
    std::shared_ptr<compositor::BufferAllocationStrategy> const& buffer_allocation_strategy)
{
    auto surface_organiser = std::make_shared<Surfaces>(
        buffer_allocation_strategy);
    return std::make_shared<DefaultIpcFactory>(surface_organiser);
}

std::shared_ptr<mf::Communicator>
mir::DefaultServerConfiguration::make_communicator(
    std::shared_ptr<compositor::BufferAllocationStrategy> const& buffer_allocation_strategy)
{
    return std::make_shared<mf::ProtobufAsioCommunicator>(
        socket_file(), make_ipc_factory(buffer_allocation_strategy));
}

std::string const& mir::DefaultServerConfiguration::socket_file()
{
    static const std::string socket_file{"/tmp/mir_socket"};
    return socket_file;
}
