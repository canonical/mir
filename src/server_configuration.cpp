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
class StubRenderer : public mg::Renderer
{
public:
    virtual void render(mg::Renderable&)
    {
    }
};

class StubBufferAllocationStrategy : public mc::BufferAllocationStrategy
{
public:
    virtual std::unique_ptr<mc::BufferSwapper> create_swapper(
        geom::Width, geom::Height, mc::PixelFormat)
    {
        return std::unique_ptr<mc::BufferSwapper>();
    }
};

// TODO this is turning from a "stub" into real implementation. Hence,
// it ought to be moved out of the testing framework to the system proper.
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
        response->set_width(request->width());
        response->set_height(request->height());
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

class StubIpcFactory : public mf::ProtobufIpcFactory
{
public:
    explicit StubIpcFactory(
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

// TODO This (with make_ipc_factory()) builds a large chunk of the "inner" system,
// it probably ought to be moved out of the testing framework to the system proper.
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

namespace mir { std::string const& test_socket_file(); } // FRIG

std::shared_ptr<mc::BufferAllocationStrategy> mir::DefaultServerConfiguration::make_buffer_allocation_strategy()
{
    return std::make_shared<StubBufferAllocationStrategy>();
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
    return std::make_shared<StubIpcFactory>(surface_organiser);
}

std::shared_ptr<mf::Communicator>
mir::DefaultServerConfiguration::make_communicator(
    std::shared_ptr<mf::ProtobufIpcFactory> const& ipc_server)
{
    return std::make_shared<mf::ProtobufAsioCommunicator>(test_socket_file(), ipc_server);
}

