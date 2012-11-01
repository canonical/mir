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
#include "mir/graphics/android/android_platform.h"
#include "mir/graphics/android/android_buffer_allocator.h"
#include "mir/graphics/android/android_display.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_bundle_surfaces.h"

#include "mir_test/mock_server_tool.h"
#include "mir_test/test_server.h"
#include "mir_test/empty_deleter.h"
#include "mir_test/test_utils_android_graphics.h"
#include "mir_test/test_utils_graphics.h"

#include <hardware/gralloc.h>

#include <gmock/gmock.h>

#include "mir/thread/all.h"
 
namespace mp=mir::process;
namespace mg=mir::graphics;
namespace mt=mir::test;
namespace mc=mir::compositor;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

/* used by both client/server for patterns */ 
bool render_pattern2(MirGraphicsRegion *region, bool)
{
    if (region->pixel_format != mir_pixel_format_rgba_8888 )
        return false;

    int *pixel = (int*) region->vaddr; 
    int i,j;
    for(i=0; i< region->width; i++)
    {
        for(j=0; j<region->height; j++)
        {
            //should render red/blue/teal/white square
            //lsb
            if (j < region->height/2) 
                if (i < region->width/2)
                    pixel[j*region->width + i] = 0xFFFF0000;
                else 
                    pixel[j*region->width + i] = 0xFFFFFF00;
            else 
                if (i < region->width/2)
                    pixel[j*region->width + i] = 0xFF0000FF;
                else 
                    pixel[j*region->width + i] = 0xFFFFFFFF;
        }
    }
    return true;
}

/* client code */
void connected_callback2(MirConnection *connection, void* context)
{
    MirConnection** tmp = (MirConnection**) context;
    *tmp = connection;
}
void create_callback2(MirSurface *surface, void*context)
{
    MirSurface** surf = (MirSurface**) context;
    *surf = surface;
}

namespace mir
{
namespace test
{
struct TestClient2
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
                                     &connected_callback2, &connection));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    /* make surface */

    surface_parameters.name = "testsurface";
    surface_parameters.width = 64;
    surface_parameters.height = 64;
    surface_parameters.pixel_format = mir_pixel_format_rgba_8888;
    mir_wait_for(mir_surface_create( connection, &surface_parameters,
                                      &create_callback2, &surface));
    MirGraphicsRegion graphics_region;
    /* grab a buffer*/
    mir_surface_get_graphics_region( surface, &graphics_region);

    /* render pattern */
    render_pattern2(&graphics_region, false);

    mir_wait_for(mir_surface_release(connection, surface, 
                            &create_callback2, &surface));
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
        response->set_width(64);
        response->set_height(64);
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

    void set_package(const std::shared_ptr<mc::BufferIPCPackage>& pack)
    {
        package = pack;
    }
    std::shared_ptr<mc::BufferIPCPackage> package;
};
}
}

struct CallBack
{
    void msg() { }
};

struct TestFullClientIPCRender : public testing::Test
{
    static void SetUpTestCase()
    {
        ASSERT_NO_THROW(
        {
            platform = mg::create_platform();
            display = platform->create_display();
        }); 
    }

    void SetUp() {
        client_process = mp::fork_and_run_in_a_different_process(
            mt::TestClient2::main_function,
            mt::TestClient2::exit_function);

        size = geom::Size{geom::Width{64}, geom::Height{64}};
        pf = geom::PixelFormat::rgba_8888;
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
    CallBack callback;
    std::shared_ptr<mp::Process> client_process;
    mt::glAnimationBasic gl_animation;
    static std::shared_ptr<mg::Platform> platform; 
    static std::shared_ptr<mg::Display> display;
};
std::shared_ptr<mg::Display> TestFullClientIPCRender::display;
std::shared_ptr<mg::Platform> TestFullClientIPCRender::platform; 

TEST_F(TestFullClientIPCRender, test_render)
{
    /* setup compositor */
    auto buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    auto allocator = platform->create_buffer_allocator(buffer_initializer);
    auto strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(allocator);
    size = geom::Size{geom::Width{gl_animation.texture_width()},
                      geom::Height{gl_animation.texture_height()}};
    pf  = geom::PixelFormat::rgba_8888;
    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(size, pf);
    auto bundle = std::make_shared<mc::BufferBundleSurfaces>(std::move(swapper));
    gl_animation.init_gl();

    auto client_buffer = bundle->secure_client_buffer();

    /* start a server */
    mock_server = std::make_shared<mt::MockServerGenerator>(client_buffer->ipc_package);
    mock_server->set_package(client_buffer->ipc_package);

    test_server = std::make_shared<mt::TestServer>("./test_socket_surface", mock_server);
    test_server->comm.start();

    /* wait for client to finish */
    EXPECT_TRUE(client_process->wait_for_termination().succeeded());
    sleep(1);
    client_buffer.reset();

    auto texture_res = bundle->lock_back_buffer();
    texture_res.reset();
    texture_res = bundle->lock_back_buffer();

    texture_res->bind_to_texture();

    gl_animation.render_gl();

    display->post_update();
    sleep(1);
    texture_res.reset();

    glClearColor(0.0, 0.0, 0.0, 0.0);
    display->post_update();    
}
