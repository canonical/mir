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

#include "mir_test_framework/process.h"

#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/buffer_initializer.h"
#include "src/server/graphics/android/android_graphic_buffer_allocator.h"

#include "mir_test/draw/android_graphics.h"
#include "mir_test/draw/patterns.h"
#include "mir_test/stub_server_tool.h"
#include "mir_test/test_protobuf_server.h"

#include "mir/frontend/connector.h"

#include <gmock/gmock.h>
#include <thread>
#include <hardware/gralloc.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

namespace mtf = mir_test_framework;
namespace mt=mir::test;
namespace mtd=mir::test::draw;
namespace mc=mir::compositor;
namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;

namespace
{
static int test_width  = 300;
static int test_height = 200;


static uint32_t pattern0 [2][2] = {{0x12345678, 0x23456789},
                                   {0x34567890, 0x45678901}};
static uint32_t pattern1 [2][2] = {{0xFFFFFFFF, 0xFFFF0000},
                                   {0xFF00FF00, 0xFF0000FF}};
static mtd::DrawPatternCheckered<2,2> draw_pattern0(pattern0);
static mtd::DrawPatternCheckered<2,2> draw_pattern1(pattern1);

struct TestClient
{
    static int render_cpu_pattern(int num_frames)
    {
        MirConnection* connection = NULL;
        MirSurface* surface;
        MirSurfaceParameters surface_parameters;

         /* establish connection. wait for server to come up */
        while (connection == NULL)
        {
            connection = mir_connect_sync("./test_socket_surface", "test_renderer");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        /* make surface */
        surface_parameters.name = "testsurface";
        surface_parameters.width = test_width;
        surface_parameters.height = test_height;
        surface_parameters.pixel_format = mir_pixel_format_abgr_8888;
        surface = mir_connection_create_surface_sync(connection, &surface_parameters);

        auto graphics_region = std::make_shared<MirGraphicsRegion>();
        for(int i=0u; i < num_frames; i++)
        {
            mir_surface_get_graphics_region(surface, graphics_region.get());
            switch (i % 2)
            {
                case 0:
                    draw_pattern0.draw(graphics_region);
                    break;
                case 1:
                default:
                    draw_pattern1.draw(graphics_region);
                    break; 
            }
            mir_surface_swap_buffers_sync(surface);
        }

        mir_surface_release_sync(surface);
        mir_connection_release(connection);
        return 0;
    }

    //performs num_frames renders, in red, green, blue repeating pattern
    static int render_rgb_with_gl(int num_frames)
    {
        MirConnection* connection = NULL;
        MirSurface* surface;
        MirSurfaceParameters surface_parameters;

         /* establish connection. wait for server to come up */
        while (connection == NULL)
        {
            connection = mir_connect_sync("./test_socket_surface", "test_renderer");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        /* make surface */
        surface_parameters.name = "testsurface";
        surface_parameters.width = test_width;
        surface_parameters.height = test_height;
        surface_parameters.pixel_format = mir_pixel_format_abgr_8888;
        surface = mir_connection_create_surface_sync(connection, &surface_parameters);

        /* set up egl context */
        int major, minor, n;
        EGLDisplay disp;
        EGLContext context;
        EGLSurface egl_surface;
        EGLConfig egl_config;
        EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_GREEN_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE };
        EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

        EGLNativeDisplayType native_display = (EGLNativeDisplayType)mir_connection_get_egl_native_display(connection);
        EGLNativeWindowType native_window = (EGLNativeWindowType) mir_surface_get_egl_native_window(surface);

        disp = eglGetDisplay(native_display);
        eglInitialize(disp, &major, &minor);

        eglChooseConfig(disp, attribs, &egl_config, 1, &n);
        egl_surface = eglCreateWindowSurface(disp, egl_config, native_window, NULL);
        context = eglCreateContext(disp, egl_config, EGL_NO_CONTEXT, context_attribs);
        eglMakeCurrent(disp, egl_surface, egl_surface, context);

        for (auto i=0; i < num_frames; i++)
        {
            switch (i % 3)
            {
                case 0: //red
                    glClearColor(1.0, 0.0, 0.0, 1.0);
                    break;
                case 1: //green
                    glClearColor(0.0, 1.0, 0.0, 1.0);
                    break;
                case 2: //blue
                default:
                    glClearColor(0.0, 0.0, 1.0, 1.0);
                    break;
            }
            glClear(GL_COLOR_BUFFER_BIT);

            eglSwapBuffers(disp, egl_surface);
        }

        mir_surface_release_sync(surface);
        mir_connection_release(connection);
        return 0;

    }

    static int exit_function()
    {
        return EXIT_SUCCESS;
    }
};

/* server code */
struct StubServerGenerator : public mt::StubServerTool
{
    StubServerGenerator(std::shared_ptr<MirNativeBuffer> const& handle, int id)
     : handle(handle),
       next_received(false),
       next_allowed(false),
       handle_id(id)
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
        response->mutable_buffer()->set_buffer_id(handle_id);
        response->mutable_buffer()->set_stride(handle->stride);

        response->mutable_buffer()->set_fds_on_side_channel(1);
        native_handle_t const* native_handle = handle->handle;
        for(auto i=0; i<native_handle->numFds; i++)
            response->mutable_buffer()->add_fd(native_handle->data[i]);
        for(auto i=0; i<native_handle->numInts; i++)
            response->mutable_buffer()->add_data(native_handle->data[native_handle->numFds+i]);

        std::unique_lock<std::mutex> lock(guard);
        surface_name = request->surface_name();
        wait_condition.notify_one();

        done->Run();
    }

    virtual void next_buffer(
        ::google::protobuf::RpcController* /*controller*/,
        ::mir::protobuf::SurfaceId const* /*request*/,
        ::mir::protobuf::Buffer* response,
        ::google::protobuf::Closure* done)
    {
        {
            std::unique_lock<std::mutex> lk(next_guard);
            next_received = true;
            next_cv.notify_all();

            while (!next_allowed) {
                allow_cv.wait(lk);
            }
            next_allowed = false;
        }

        response->set_buffer_id(handle_id);

        response->set_fds_on_side_channel(1);
        native_handle_t const* native_handle = handle->handle;
        response->set_stride(handle->stride);
        for(auto i=0; i<native_handle->numFds; i++)
            response->add_fd(native_handle->data[i]);
        for(auto i=0; i<native_handle->numInts; i++)
            response->add_data(native_handle->data[native_handle->numFds+i]);

        done->Run();
    }

    void wait_on_next_buffer()
    {
        std::unique_lock<std::mutex> lk(next_guard);
        while (!next_received)
            next_cv.wait(lk);
        next_received = false;
    }

    void allow_next_continue()
    {
        std::unique_lock<std::mutex> lk(next_guard);
        next_allowed = true;
        allow_cv.notify_all();
        lk.unlock();
    }

    void set_handle(std::shared_ptr<MirNativeBuffer> const& pack, int id)
    {
        handle = pack;
        handle_id = id;
    }

private:
    std::shared_ptr<MirNativeBuffer> handle;

    std::mutex next_guard;
    std::condition_variable next_cv;
    std::condition_variable allow_cv;
    bool next_received;
    bool next_allowed;

    int handle_id;
};

}

struct TestClientIPCRender : public testing::Test
{
    /* kdub -- some of the (less thoroughly tested) android blob drivers annoyingly keep
       static state about what process they are in. Once you fork, this info is invalid,
       yet the driver uses the info and bad things happen.
       Fork all needed processes before touching the blob! */
    static void SetUpTestCase() {
        render_single_client_process = mtf::fork_and_run_in_a_different_process(
            std::bind(TestClient::render_cpu_pattern, 1),
            TestClient::exit_function);

        render_double_client_process = mtf::fork_and_run_in_a_different_process(
            std::bind(TestClient::render_cpu_pattern, 2),
            TestClient::exit_function);

        render_accelerated_process = mtf::fork_and_run_in_a_different_process(
            std::bind(TestClient::render_rgb_with_gl, 1),
            TestClient::exit_function);

        render_accelerated_process_double = mtf::fork_and_run_in_a_different_process(
            std::bind(TestClient::render_rgb_with_gl, 2),
            TestClient::exit_function);
    }

    void SetUp() {
        ASSERT_FALSE(mtd::is_surface_flinger_running());

        size = geom::Size{test_width, test_height};
        pf = geom::PixelFormat::abgr_8888;

        auto initializer = std::make_shared<mg::NullBufferInitializer>();
        allocator = std::make_shared<mga::AndroidGraphicBufferAllocator> (initializer);
        mg::BufferProperties properties(size, pf, mg::BufferUsage::hardware);
        android_buffer = allocator->alloc_buffer(properties);
        second_android_buffer = allocator->alloc_buffer(properties);

        buffer_converter = std::make_shared<mtd::TestGrallocMapper>();

        handle = android_buffer->native_buffer_handle();
        second_handle = second_android_buffer->native_buffer_handle();

        /* start a server */
        mock_server = std::make_shared<StubServerGenerator>(handle, 14);
        test_server = std::make_shared<mt::TestProtobufServer>("./test_socket_surface", mock_server);
        test_server->comm->start();
    }

    void TearDown()
    {
        test_server.reset();
    }

    mir::protobuf::Connection response;

    std::shared_ptr<mt::TestProtobufServer> test_server;
    std::shared_ptr<StubServerGenerator> mock_server;

    geom::Size size;
    geom::PixelFormat pf;
    mg::BufferID id1;
    mg::BufferID id2;
    std::shared_ptr<mtd::TestGrallocMapper> buffer_converter;
    std::shared_ptr<mtf::Process> client_process;

    std::shared_ptr<MirNativeBuffer> handle;
    std::shared_ptr<MirNativeBuffer> second_handle;

    std::shared_ptr<mg::Buffer> android_buffer;
    std::shared_ptr<mg::Buffer> second_android_buffer;
    std::shared_ptr<mga::AndroidGraphicBufferAllocator>  allocator;

    static std::shared_ptr<mtf::Process> render_single_client_process;
    static std::shared_ptr<mtf::Process> render_double_client_process;
    static std::shared_ptr<mtf::Process> render_accelerated_process;
    static std::shared_ptr<mtf::Process> render_accelerated_process_double;
};
std::shared_ptr<mtf::Process> TestClientIPCRender::render_single_client_process;
std::shared_ptr<mtf::Process> TestClientIPCRender::render_double_client_process;
std::shared_ptr<mtf::Process> TestClientIPCRender::render_accelerated_process;
std::shared_ptr<mtf::Process> TestClientIPCRender::render_accelerated_process_double;

TEST_F(TestClientIPCRender, test_render_single)
{
    /* activate client */
    render_single_client_process->cont();

    /* wait for client to finish */
    EXPECT_TRUE(render_single_client_process->wait_for_termination().succeeded());

    /* check content */
    auto region = buffer_converter->graphic_region_from_handle(handle);
    EXPECT_TRUE(draw_pattern0.check(region));
}

TEST_F(TestClientIPCRender, test_render_double)
{
    /* activate client */
    render_double_client_process->cont();

    /* wait for next buffer */
    mock_server->wait_on_next_buffer();
    auto region = buffer_converter->graphic_region_from_handle(handle);
    EXPECT_TRUE(draw_pattern0.check(region));

    mock_server->set_handle(second_handle, 15);
    mock_server->allow_next_continue();

    /* wait for client to finish */
    EXPECT_TRUE(render_double_client_process->wait_for_termination().succeeded());

    auto second_region = buffer_converter->graphic_region_from_handle(second_handle);
    EXPECT_TRUE(draw_pattern1.check(region));
}

TEST_F(TestClientIPCRender, test_accelerated_render)
{
    mtd::DrawPatternSolid red_pattern(0xFF0000FF);

    /* activate client */
    render_accelerated_process->cont();

    /* wait for next buffer */
    mock_server->wait_on_next_buffer();
    mock_server->allow_next_continue();

    /* wait for client to finish */
    EXPECT_TRUE(render_accelerated_process->wait_for_termination().succeeded());

    /* check content */
    auto region = buffer_converter->graphic_region_from_handle(handle);
    EXPECT_TRUE(red_pattern.check(region));
}

TEST_F(TestClientIPCRender, test_accelerated_render_double)
{
    mtd::DrawPatternSolid red_pattern(0xFF0000FF);
    mtd::DrawPatternSolid green_pattern(0xFF00FF00);

    /* activate client */
    render_accelerated_process_double->cont();

    /* wait for next buffer */
    mock_server->wait_on_next_buffer();
    mock_server->set_handle(second_handle, 15);
    mock_server->allow_next_continue();

    mock_server->wait_on_next_buffer();
    mock_server->allow_next_continue();

    /* wait for client to finish */
    EXPECT_TRUE(render_accelerated_process_double->wait_for_termination().succeeded());

    /* check content */
    auto region = buffer_converter->graphic_region_from_handle(handle);
    EXPECT_TRUE(red_pattern.check(region));

    auto second_region = buffer_converter->graphic_region_from_handle(second_handle);
    EXPECT_TRUE(green_pattern.check(second_region));
}
