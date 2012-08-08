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

#include "testing_process_manager.h"
#include "mir/display_server.h"

#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/frontend/communicator.h"
#include "mir/frontend/protobuf_asio_communicator.h"
#include "mir/graphics/renderer.h"
#include "mir/geometry/dimensions.h"
#include "mir/compositor/buffer_swapper.h"

#include "mir_protobuf.pb.h"

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mg = mir::graphics;

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

class StubDisplayServer : public mir::protobuf::DisplayServer
{
public:
    void connect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::ConnectMessage* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        // TODO do the real work
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
};

class StubIpcFactory : public mf::ProtobufIpcFactory
{
    virtual std::shared_ptr<mir::protobuf::DisplayServer> make_ipc_server()
    {
        return std::make_shared<StubDisplayServer>();
    }
};
}


std::shared_ptr<mc::BufferAllocationStrategy> mir::TestingServerConfiguration::make_buffer_allocation_strategy()
{
    return std::make_shared<StubBufferAllocationStrategy>();
}

void mir::TestingServerConfiguration::exec(DisplayServer* )
{
}

void mir::TestingServerConfiguration::on_exit(DisplayServer* )
{
}

std::shared_ptr<mg::Renderer> mir::TestingServerConfiguration::make_renderer()
{
    return std::make_shared<StubRenderer>();
}

std::shared_ptr<mir::frontend::ProtobufIpcFactory>
mir::TestingServerConfiguration::make_ipc_factory()
{
    return std::make_shared<StubIpcFactory>();
}

std::shared_ptr<mf::Communicator>
mir::TestingServerConfiguration::make_communicator(
    std::shared_ptr<mf::ProtobufIpcFactory> const& ipc_server)
{
    return std::make_shared<mf::ProtobufAsioCommunicator>(test_socket_file(), ipc_server);
}

std::string const& mir::test_socket_file()
{
    static const std::string socket_file{"/tmp/mir_socket_test"};
    return socket_file;
}


