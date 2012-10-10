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

namespace
{
static int test_width  = 64;
static int test_height = 48;

/* used by both client/server for patterns */ 
bool render_pattern(MirGraphicsRegion *region, bool check)
{
    if (region->pixel_format != mir_pixel_format_rgba_8888 )
        return false;

    int *pixel = (int*) region->vaddr; 
    int i,j;
    for(i=0; i< region->width; i++)
    {
        for(j=0; j<region->height; j++)
        {
            if (check)
            {
                if (pixel[j*region->width + i] != 0x12345689)
                    return false;
            }
            else
            {
                pixel[j*region->width + i] = 0x12345689;
            }
        }
    }
    return true;
}
}

namespace mir
{
namespace test
{
/* client code */
static void connected_callback(MirConnection *connection, void* context)
{
    MirConnection** tmp = (MirConnection**) context;
    *tmp = connection;
}
static void create_callback(MirSurface *surface, void*context)
{
    MirSurface** surf = (MirSurface**) context;
    *surf = surface;
}

struct TestClient
{

static int main_function()
{
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
    surface_parameters.name = "testsurface";
    surface_parameters.width = test_width;
    surface_parameters.height = test_height;
    surface_parameters.pixel_format = mir_pixel_format_rgba_8888;
    mir_wait_for(mir_surface_create( connection, &surface_parameters,
                                      &create_callback, &surface));
    MirGraphicsRegion graphics_region;
    /* grab a buffer*/
    mir_surface_get_graphics_region( surface, &graphics_region);

    /* render pattern */
    render_pattern(&graphics_region, false);

    mir_wait_for(mir_surface_release(connection, surface, &create_callback, &surface));

    /* release */
    mir_connection_release(connection);
    return 0;
}

static int exit_function()
{
    return EXIT_SUCCESS;
}
};

/* server code */
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
        response->set_width(test_width);
        response->set_height(test_height);
        response->set_pixel_format(request->pixel_format());
        response->mutable_buffer()->set_buffer_id(22);

        unsigned int i;
        response->mutable_buffer()->set_fds_on_side_channel(1);
        for(i=0; i<package->ipc_fds.size(); i++)
            response->mutable_buffer()->add_fd(package->ipc_fds[i]);
        for(i=0; i<package->ipc_data.size(); i++)
            response->mutable_buffer()->add_data(package->ipc_data[i]);

        std::unique_lock<std::mutex> lock(guard);
        surface_name = request->surface_name();
        wait_condition.notify_one();

        done->Run();
    }

    std::shared_ptr<mc::BufferIPCPackage> package;
};

bool check_buffer(std::shared_ptr<mc::BufferIPCPackage> package, const hw_module_t *hw_module)
{
    native_handle_t* handle;
    handle = (native_handle_t*) malloc(sizeof(int) * ( 3 + package->ipc_data.size() + package->ipc_fds.size() ));
    handle->numInts = package->ipc_data.size();
    handle->numFds  = package->ipc_fds.size();
    int i;
    for(i = 0; i< handle->numFds; i++)
        handle->data[i] = package->ipc_fds[i];
    for(; i < handle->numFds + handle->numInts; i++)
        handle->data[i] = package->ipc_data[i-handle->numFds];

    int *vaddr;
    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
    gralloc_module_t *grmod = (gralloc_module_t*) hw_module;
    grmod->lock(grmod, handle, usage, 0, 0, test_width, test_height, (void**) &vaddr); 

    MirGraphicsRegion region;
    region.vaddr = (char*) vaddr;
    region.width = test_width;
    region.height = test_height;
    region.pixel_format = mir_pixel_format_rgba_8888; 

    auto valid = render_pattern(&region, false);
    grmod->unlock(grmod, handle);
    return valid; 
}

}
}

struct TestClientIPCRender : public testing::Test
{
    void SetUp() {
        client_process = mp::fork_and_run_in_a_different_process(
            mt::TestClient::main_function,
            mt::TestClient::exit_function);

        size = geom::Size{geom::Width{test_width}, geom::Height{test_height}};
        pf = geom::PixelFormat::rgba_8888;

        /* allocate an android buffer */
        int err;
        struct alloc_device_t *alloc_device_raw;
        err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module);
        if (err < 0)
            throw std::runtime_error("Could not open hardware module");
        gralloc_open(hw_module, &alloc_device_raw);
        auto alloc_device = std::shared_ptr<struct alloc_device_t> ( alloc_device_raw, mir::EmptyDeleter());
        auto alloc_adaptor = std::make_shared<mga::AndroidAllocAdaptor>(alloc_device);
        android_buffer = std::make_shared<mga::AndroidBuffer>(alloc_adaptor, size, pf);
        package = android_buffer->get_ipc_package();
    }

    void TearDown()
    {
        test_server->comm.stop();
    }

    mir::protobuf::Connection response;

    std::shared_ptr<mt::TestServer> test_server;
    std::shared_ptr<mt::MockServerGenerator> mock_server;

    const hw_module_t    *hw_module;
    geom::Size size;
    geom::PixelFormat pf; 
    std::shared_ptr<mp::Process> client_process;
    std::shared_ptr<mc::BufferIPCPackage> package;
    std::shared_ptr<mga::AndroidBuffer> android_buffer;
};

TEST_F(TestClientIPCRender, test_render)
{
    /* start a server */
    mock_server = std::make_shared<mt::MockServerGenerator>(package);
    test_server = std::make_shared<mt::TestServer>("./test_socket_surface", mock_server);
    test_server->comm.start();

    /* wait for client to finish */
    EXPECT_TRUE(client_process->wait_for_termination().succeeded());

    /* check content */
    EXPECT_TRUE(mt::check_buffer(mock_server->package, hw_module));
}
