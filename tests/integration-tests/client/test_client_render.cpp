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
#include "mir/graphics/android/native_buffer.h"
#include "src/server/graphics/android/android_graphic_buffer_allocator.h"

#include "mir_test/draw/android_graphics.h"
#include "mir_test/draw/patterns.h"
#include "mir_test/stub_server_tool.h"
#include "mir_test/test_protobuf_server.h"

#include "mir/frontend/connector.h"

#include <iostream>
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

static uint32_t pattern0 [2][2] = {{0x12345678, 0x23456789},
                                   {0x34567890, 0x45678901}};

static uint32_t pattern1 [2][2] = {{0xFFFFFFFF, 0xFFFF0000},
                                   {0xFF00FF00, 0xFF0000FF}};
struct TestClient
{
    static MirPixelFormat select_format_for_visual_id(int visual_id)
    {
        if (visual_id == 5)
            return mir_pixel_format_argb_8888;
        if (visual_id == 1)
            return mir_pixel_format_abgr_8888;

        return mir_pixel_format_invalid;
    }

    static void sig_handle(int)
    {
    }

    static int render_single()
    {
        if (signal(SIGCONT, sig_handle) == SIG_ERR)
            return -1;
        pause();

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
        surface_parameters.pixel_format = mir_pixel_format_abgr_8888;
        mir_wait_for(mir_connection_create_surface(connection,
                                                   &surface_parameters,
                                                   &create_callback,
                                                   &surface));

        auto graphics_region = std::make_shared<MirGraphicsRegion>();
        /* grab a buffer*/
        mir_surface_get_graphics_region(surface, graphics_region.get());

        /* render pattern */
        mtd::DrawPatternCheckered<2,2> draw_pattern0(mt::pattern0);
        draw_pattern0.draw(graphics_region);
 
        mir_surface_swap_buffers_sync(surface);

        mir_wait_for(mir_surface_release(surface, &create_callback, &surface));

        /* release */
        mir_connection_release(connection);
        return 0;
    }

    static int render_double()
    {
        if (signal(SIGCONT, sig_handle) == SIG_ERR)
            return -1;
        pause();

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
        surface_parameters.pixel_format = mir_pixel_format_abgr_8888;

        mir_wait_for(mir_connection_create_surface(connection,
                                                   &surface_parameters,
                                                   &create_callback,
                                                   &surface));

        auto graphics_region = std::make_shared<MirGraphicsRegion>();
        mir_surface_get_graphics_region( surface, graphics_region.get());
        mtd::DrawPatternCheckered<2,2> draw_pattern0(mt::pattern0);
        draw_pattern0.draw(graphics_region);

        mir_surface_swap_buffers_sync(surface);
        mir_surface_get_graphics_region( surface, graphics_region.get());
        mtd::DrawPatternCheckered<2,2> draw_pattern1(mt::pattern1);
        draw_pattern1.draw(graphics_region);

        mir_surface_swap_buffers_sync(surface);

        mir_wait_for(mir_surface_release(surface, &create_callback, &surface));

        /* release */
        mir_connection_release(connection);
        return 0;
    }

    static MirSurface* create_mir_surface(MirConnection * connection, EGLDisplay display, EGLConfig config)
    {
        int visual_id;
        eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &visual_id);

        /* make surface */
        MirSurfaceParameters surface_parameters;
        surface_parameters.name = "testsurface";
        surface_parameters.width = test_width;
        surface_parameters.height = test_height;
        surface_parameters.pixel_format = select_format_for_visual_id(visual_id);
        return mir_connection_create_surface_sync(connection, &surface_parameters);
    }

    static int render_accelerated()
    {
        if (signal(SIGCONT, sig_handle) == SIG_ERR)
            return -1;
        pause();

        int major, minor, n;
        EGLContext context;
        EGLSurface egl_surface;
        EGLConfig egl_config;
        EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BUFFER_SIZE, 32,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE };
        EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

        auto connection = mir_connect_sync("./test_socket_surface", "test_renderer");

        auto native_display = mir_connection_get_egl_native_display(connection);
        auto egl_display = eglGetDisplay(native_display);
        eglInitialize(egl_display, &major, &minor);
        eglChooseConfig(egl_display, attribs, &egl_config, 1, &n); 

        auto mir_surface = create_mir_surface(connection, egl_display, egl_config); 
        auto native_window = static_cast<EGLNativeWindowType>(
            mir_surface_get_egl_native_window(mir_surface)); 
    
        egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, NULL);
        context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
        eglMakeCurrent(egl_display, egl_surface, egl_surface, context);

        glClearColor(1.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        eglSwapBuffers(egl_display, egl_surface);
        
        mir_surface_release_sync(mir_surface);

        /* release */
        mir_connection_release(connection);
        return 0;
    }

    static int render_accelerated_double()
    {
        if (signal(SIGCONT, sig_handle) == SIG_ERR)
            return -1;
        pause();

        int major, minor, n;
        EGLContext context;
        EGLSurface egl_surface;
        EGLConfig egl_config;
        EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BUFFER_SIZE, 32,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE };
        EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

        auto connection = mir_connect_sync("./test_socket_surface", "test_renderer");

        auto native_display = mir_connection_get_egl_native_display(connection);
        auto egl_display = eglGetDisplay(native_display);
        eglInitialize(egl_display, &major, &minor);
        eglChooseConfig(egl_display, attribs, &egl_config, 1, &n); 

        auto mir_surface = create_mir_surface(connection, egl_display, egl_config); 
        auto native_window = static_cast<EGLNativeWindowType>(
            mir_surface_get_egl_native_window(mir_surface)); 
    
        egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, NULL);
        context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
        eglMakeCurrent(egl_display, egl_surface, egl_surface, context);

        //draw red
        glClearColor(1.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        eglSwapBuffers(egl_display, egl_surface);

        //draw green 
        glClearColor(0.0, 1.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        eglSwapBuffers(egl_display, egl_surface);
        
        mir_surface_release_sync(mir_surface);

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
    StubServerGenerator(std::shared_ptr<MirNativeBuffer> const& handle0,
                        std::shared_ptr<MirNativeBuffer> const& handle1)
     : handle0(handle0),
       handle1(handle1),
       next_buffer_count(0),
       buffer_count(2)
    {
    }

    void create_surface(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        auto next_handle = get_next_handle();

        response->mutable_id()->set_value(13); //surface id
        response->set_width(test_width);
        response->set_height(test_height);
        surface_pf = geom::PixelFormat(request->pixel_format());
        response->set_pixel_format(request->pixel_format());
        response->mutable_buffer()->set_buffer_id(next_buffer_count % buffer_count);
        response->mutable_buffer()->set_stride(next_handle->stride);

        response->mutable_buffer()->set_fds_on_side_channel(1);
        native_handle_t const* native_handle = handle->handle();
        for(auto i=0; i<native_handle->numFds; i++)
            response->mutable_buffer()->add_fd(native_handle->data[i]);
        for(auto i=0; i < native_handle->numInts; i++)
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
        auto next_handle = get_next_handle();
        response->set_buffer_id(next_buffer_count % buffer_count);

        response->set_fds_on_side_channel(1);
        native_handle_t const* native_handle = next_handle->handle;
        response->set_stride(next_handle->stride);
        for(auto i=0; i<native_handle->numFds; i++)
            response->add_fd(native_handle->data[i]);
        for(auto i=0; i<native_handle->numInts; i++)
            response->add_data(native_handle->data[native_handle->numFds+i]);

        done->Run();
    }

    std::shared_ptr<MirNativeBuffer> get_next_handle()
    {
        std::swap(handle0, handle1);
        next_buffer_count++;
        return handle0;
    }

    std::shared_ptr<MirNativeBuffer> second_to_last_returned()
    {
        return handle0;
    }

    std::shared_ptr<MirNativeBuffer> last_returned()
    {
        return handle1;
    }

    uint32_t red_value_for_surface()
    {
        if ((surface_pf == geom::PixelFormat::abgr_8888) || (surface_pf == geom::PixelFormat::xbgr_8888))
            return 0xFF0000FF;

        if ((surface_pf == geom::PixelFormat::argb_8888) || (surface_pf == geom::PixelFormat::xrgb_8888))
            return 0xFFFF0000;

        return 0x0;
    }

    uint32_t green_value_for_surface()
    {
        return 0xFF00FF00;
    }

private:
    geom::PixelFormat surface_pf; //must be a 32 bit one;
    std::shared_ptr<MirNativeBuffer> handle0, handle1;
    int next_buffer_count;
    int const buffer_count;
};

}
}

struct TestClientIPCRender : public testing::Test
{
    /* kdub -- some of the (less thoroughly tested) android blob drivers annoyingly keep
       static state about what process they are in. Once you fork, this info is invalid,
       yet the driver uses the info and bad things happen.
       Fork all needed processes before touching the blob! */
    static void SetUpTestCase() {
        render_single_client_process = mtf::fork_and_run_in_a_different_process(
            mt::TestClient::render_single,
            mt::TestClient::exit_function);

        render_double_client_process = mtf::fork_and_run_in_a_different_process(
            mt::TestClient::render_double,
            mt::TestClient::exit_function);

        render_accelerated_process
             = mtf::fork_and_run_in_a_different_process(
                            mt::TestClient::render_accelerated,
                            mt::TestClient::exit_function);

        render_accelerated_process_double
             = mtf::fork_and_run_in_a_different_process(
                            mt::TestClient::render_accelerated_double,
                            mt::TestClient::exit_function);
    }

    void SetUp() {
        ASSERT_FALSE(mtd::is_surface_flinger_running());

        auto size = geom::Size{test_width, test_height};
        auto pf = geom::PixelFormat::argb_8888;

        buffer_converter = std::make_shared<mtd::TestGrallocMapper>();

        auto initializer = std::make_shared<mg::NullBufferInitializer>();
        auto allocator = std::make_shared<mga::AndroidGraphicBufferAllocator> (initializer);
        mg::BufferProperties properties(size, pf, mg::BufferUsage::hardware);
        android_buffer0 = allocator->alloc_buffer(properties);
        android_buffer1 = allocator->alloc_buffer(properties);

        auto handle0 = android_buffer0->native_buffer_handle();
        auto handle1 = android_buffer1->native_buffer_handle();

        /* start a server */
        mock_server = std::make_shared<mt::StubServerGenerator>(handle0, handle1);
        test_server = std::make_shared<mt::TestProtobufServer>("./test_socket_surface", mock_server);
        test_server->comm->start();
    }

    void TearDown()
    {
        test_server.reset();
    }

    mir::protobuf::Connection response;

    std::shared_ptr<mt::TestProtobufServer> test_server;
    std::shared_ptr<mt::StubServerGenerator> mock_server;

    std::shared_ptr<mtd::TestGrallocMapper> buffer_converter;
    std::shared_ptr<mtf::Process> client_process;

    std::shared_ptr<mg::Buffer> android_buffer0;
    std::shared_ptr<mg::Buffer> android_buffer1;

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
    mtd::DrawPatternCheckered<2,2> rendered_pattern(mt::pattern0);
    /* activate client */
    render_single_client_process->cont();

    /* wait for client to finish */
    EXPECT_TRUE(render_single_client_process->wait_for_termination().succeeded());

    /* check content */
    auto last_handle = mock_server->last_returned();
    EXPECT_TRUE(rendered_pattern.check(buffer_converter->graphic_region_from_handle(last_handle)));
}

TEST_F(TestClientIPCRender, test_render_double)
{
    mtd::DrawPatternCheckered<2,2> rendered_pattern0(mt::pattern0);
    mtd::DrawPatternCheckered<2,2> rendered_pattern1(mt::pattern1);
    /* activate client */
    render_double_client_process->cont();

    /* wait for client to finish */
    EXPECT_TRUE(render_double_client_process->wait_for_termination().succeeded());

    auto second_to_last_handle = mock_server->second_to_last_returned();
    auto region = buffer_converter->graphic_region_from_handle(second_to_last_handle);
    EXPECT_TRUE(rendered_pattern0.check(region));

    auto last_handle = mock_server->last_returned();
    auto second_region = buffer_converter->graphic_region_from_handle(last_handle);
    EXPECT_TRUE(rendered_pattern1.check(second_region));
}

TEST_F(TestClientIPCRender, test_accelerated_render)
{
    render_accelerated_process->cont();
    EXPECT_TRUE(render_accelerated_process->wait_for_termination().succeeded());

    /* check content */
    mtd::DrawPatternSolid red_pattern(mock_server->red_value_for_surface());
    auto last_handle = mock_server->last_returned();
    EXPECT_TRUE(red_pattern.check(buffer_converter->graphic_region_from_handle(last_handle)));
}

TEST_F(TestClientIPCRender, test_accelerated_render_double)
{
    render_accelerated_process_double->cont();
    EXPECT_TRUE(render_accelerated_process_double->wait_for_termination().succeeded());

    /* check content */
    mtd::DrawPatternSolid red_pattern(mock_server->red_value_for_surface());
    auto second_to_last_handle = mock_server->second_to_last_returned();
    auto region = buffer_converter->graphic_region_from_handle(second_to_last_handle);
    EXPECT_TRUE(red_pattern.check(region));

    mtd::DrawPatternSolid green_pattern(mock_server->green_value_for_surface());
    auto last_handle = mock_server->last_returned();
    auto second_region = buffer_converter->graphic_region_from_handle(last_handle);
    EXPECT_TRUE(green_pattern.check(second_region));
}
