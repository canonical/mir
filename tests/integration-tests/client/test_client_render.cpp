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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/process/process.h"

#include "mir_client/mir_client_library.h"

#include "mir/frontend/protobuf_asio_communicator.h"
#include "mir/frontend/resource_cache.h"
#include "mir/graphics/android/android_buffer.h"
#include "mir/graphics/android/android_alloc_adaptor.h"
#include "mir/compositor/buffer_ipc_package.h"

#include "mir_test/mock_server_tool.h"
#include "mir_test/test_server.h"
#include "mir_test/empty_deleter.h"

#include <hardware/gralloc.h>

#include <gmock/gmock.h>

#include "mir/thread/all.h" 
namespace mp=mir::process;
namespace mt=mir::test;
namespace mc=mir::compositor;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

void connected_callback(MirConnection *connection, void* context)
{
    printf("CLIENT connect callback %X\n", (int) connection);
    MirConnection** tmp = (MirConnection**) context;
    *tmp = connection;
}
void create_callback(MirSurface *surface, void*context)
{
    printf("CLIENT create callback %X\n", (int) surface);
    MirSurface** surf = (MirSurface**) context;
    *surf = surface;
}

void render_pattern(MirGraphicsRegion *region)
{
    if (region->pixel_format != mir_pixel_format_rgba_8888 )
        return;

    int *pixel = (int*) region->vaddr; 
    int i,j;
    for(i=0; i< region->width; i++)
    {
        for(j=0; j<region->height; j++)
        {
            pixel[i*region->width + j] = 0x12345689;
        }
    }
}


namespace mir
{
namespace test
{
struct TestClient
{


/* client code */
static int main_function()
{
    printf("done sleeping\n");
    /* only use C api */
    MirConnection* connection = NULL;
    MirSurface* surface;
    MirSurfaceParameters surface_parameters;

     /* establish connection. wait for server to come up */
    while (connection == NULL)
    {
        mir_wait_for(mir_connect("./test_socket_surface", "test_renderer",
                                     &connected_callback, &connection));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    /* make surface */
    printf("client done waiting for connect %X\n", (int) connection);

    surface_parameters.name = "testsurface";
    surface_parameters.width = 48;
    surface_parameters.height = 64;
    surface_parameters.pixel_format = mir_pixel_format_rgba_8888;
    mir_wait_for(mir_surface_create( connection, &surface_parameters,
                                      &create_callback, &surface));
    printf("CREATION DONE\n");
#if 0
    MirGraphicsRegion graphics_region;
    /* grab a buffer*/
    mir_surface_get_graphics_region( surface, &graphics_region);

    printf("render pattern w %X\n", (int) graphics_region.width);
    printf("render pattern h %X\n", (int) graphics_region.height);
    printf("render pattern 0x%X\n", (int) graphics_region.vaddr);
    /* render pattern */
    render_pattern(&graphics_region);

    /* release */
    mir_connection_release(connection);
#endif
    return 0;
}

static int exit_function()
{
    return EXIT_SUCCESS;
}

};

struct MockServerGenerator : public mt::MockServerTool
{
    MockServerGenerator(const std::shared_ptr<mc::BufferIPCPackage>& pack)
     : package(pack)
    {

    }

    void create_surface(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        response->mutable_id()->set_value(13); // TODO distinct numbers & tracking
        response->set_width(64);
        response->set_height(48);
        response->set_pixel_format(request->pixel_format());
        response->mutable_buffer()->set_buffer_id(22);

        unsigned int i;
        printf("PACKAGE INT %i FD %i\n", package->ipc_data.size(), package->ipc_fds.size());
        response->mutable_buffer()->set_fds_on_side_channel(1);
        for(i=0; i<package->ipc_fds.size(); i++)
            response->mutable_buffer()->add_fd(package->ipc_fds[i]);
        for(i=0; i<package->ipc_data.size(); i++)
            response->mutable_buffer()->add_data(package->ipc_data[i]);

        std::unique_lock<std::mutex> lock(guard);
        surface_name = request->surface_name();
        wait_condition.notify_one();

        printf("MOCK SERVER: create surface DONE\n");
        done->Run();
    }

    std::shared_ptr<mc::BufferIPCPackage> package;
};
}
}

struct CallBack
{
    void msg() { printf("CALLBACK TETST\n"); }
};

struct TestClientIPCRender : public testing::Test
{
    void SetUp() {
        auto p = mp::fork_and_run_in_a_different_process(
            mt::TestClient::main_function,
            mt::TestClient::exit_function);

        size = geom::Size{geom::Width{64}, geom::Height{48}};
        pf = geom::PixelFormat::rgba_8888;

        int err;
        struct alloc_device_t *alloc_device_raw;
        const hw_module_t    *hw_module;
        err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module);
        if (err < 0)
            throw std::runtime_error("Could not open hardware module");
        gralloc_open(hw_module, &alloc_device_raw);

        auto alloc_device = std::shared_ptr<struct alloc_device_t> ( alloc_device_raw, mir::EmptyDeleter());

        auto alloc_adaptor = std::make_shared<mga::AndroidAllocAdaptor>(alloc_device);

        auto android_buffer = std::make_shared<mga::AndroidBuffer>(alloc_adaptor, size, pf);

        auto package = android_buffer->get_ipc_package();

        mock_server = std::make_shared<mt::MockServerGenerator>(package);

        test_server = std::make_shared<mt::TestServer>("./test_socket_surface", mock_server);
        test_server->comm.start();


        EXPECT_TRUE(p->wait_for_termination().succeeded());
    }

    void TearDown()
    {
        test_server->comm.stop();
    }

    mir::protobuf::Connection response;

    std::shared_ptr<mt::TestServer> test_server;
    std::shared_ptr<mt::MockServerGenerator> mock_server;

    geom::Size size;
    geom::PixelFormat pf; 
    CallBack callback;
};

TEST_F(TestClientIPCRender, test_render)
{
    /* start server */

    /* wait for connect */    
    /* wait for buffer sent back */

    /* verify pattern */
}
