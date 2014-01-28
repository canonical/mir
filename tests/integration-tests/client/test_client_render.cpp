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
#include "src/platform/graphics/android/buffer.h"
#include "mir/graphics/android/native_buffer.h"
#include "src/platform/graphics/android/android_graphic_buffer_allocator.h"

#include "mir_test_framework/cross_process_sync.h"
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
static const char socket_file[] = "./test_client_ipc_render_socket";

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

    static int render_cpu_pattern(mtf::CrossProcessSync& process_sync, int num_frames)
    {
        process_sync.wait_for_signal_ready_for();

        MirSurfaceParameters surface_parameters
        {
            "testsurface", test_width, test_height, mir_pixel_format_abgr_8888,
            mir_buffer_usage_software, mir_display_output_id_invalid
        };
        auto connection = mir_connect_sync(socket_file, "test_renderer");
        auto surface = mir_connection_create_surface_sync(connection, &surface_parameters);
        MirGraphicsRegion graphics_region;
        for(int i=0u; i < num_frames; i++)
        {
            mir_surface_get_graphics_region(surface, &graphics_region);
            if (i % 2)
            {
                draw_pattern1.draw(graphics_region);
            }
            else
            {
                draw_pattern0.draw(graphics_region);
            }
            mir_surface_swap_buffers_sync(surface);
        }

        mir_surface_release_sync(surface);
        mir_connection_release(connection);
        return 0;
    }

    //performs num_frames renders, in red, green, blue repeating pattern
    static int render_rgb_with_gl(mtf::CrossProcessSync& process_sync, int num_frames)
    {
        process_sync.wait_for_signal_ready_for();

        auto connection = mir_connect_sync(socket_file, "test_renderer");

        /* set up egl context */
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

            eglSwapBuffers(egl_display, egl_surface);
        }

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
    StubServerGenerator()
    {
        auto initializer = std::make_shared<mg::NullBufferInitializer>();
        allocator = std::make_shared<mga::AndroidGraphicBufferAllocator> (initializer);
        auto size = geom::Size{test_width, test_height};
        surface_pf = mir_pixel_format_abgr_8888;
        last_posted = allocator->alloc_buffer_platform(size, surface_pf, mga::BufferUsage::use_hardware);
        client_buffer = allocator->alloc_buffer_platform(size, surface_pf, mga::BufferUsage::use_hardware);
    }

    void create_surface(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        response->mutable_id()->set_value(13);
        response->set_width(test_width);
        response->set_height(test_height);
        surface_pf = MirPixelFormat(request->pixel_format());
        response->set_pixel_format(request->pixel_format());
        response->mutable_buffer()->set_buffer_id(client_buffer->id().as_uint32_t());

        auto buf = client_buffer->native_buffer_handle();
        //note about the stride. Mir protocol sends stride in bytes, android uses stride in pixels
        response->mutable_buffer()->set_stride(client_buffer->stride().as_uint32_t());

        auto const& size = client_buffer->size();
        response->mutable_buffer()->set_width(size.width.as_int());
        response->mutable_buffer()->set_height(size.height.as_int());

        response->mutable_buffer()->set_fds_on_side_channel(1);
        native_handle_t const* native_handle = buf->handle();
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
        std::unique_lock<std::mutex> lk(buffer_mutex);
        std::swap(last_posted, client_buffer);

        response->set_buffer_id(client_buffer->id().as_uint32_t());

        auto buf = client_buffer->native_buffer_handle();
        response->set_fds_on_side_channel(1);
        native_handle_t const* native_handle = buf->handle();
        response->set_stride(client_buffer->stride().as_uint32_t());

        auto const& size = client_buffer->size();
        response->set_width(size.width.as_int());
        response->set_height(size.height.as_int());

        for(auto i=0; i<native_handle->numFds; i++)
            response->add_fd(native_handle->data[i]);
        for(auto i=0; i<native_handle->numInts; i++)
            response->add_data(native_handle->data[native_handle->numFds+i]);
        done->Run();
    }

    uint32_t red_value_for_surface()
    {
        if ((surface_pf == mir_pixel_format_abgr_8888) || (surface_pf == mir_pixel_format_xbgr_8888))
            return 0xFF0000FF;

        if ((surface_pf == mir_pixel_format_argb_8888) || (surface_pf == mir_pixel_format_xrgb_8888))
            return 0xFFFF0000;

        return 0x0;
    }

    uint32_t green_value_for_surface()
    {
        return 0xFF00FF00;
    }

    std::shared_ptr<mg::Buffer> second_to_last_posted_buffer()
    {
        std::unique_lock<std::mutex> lk(buffer_mutex);
        return client_buffer;
    }

    std::shared_ptr<mg::Buffer> last_posted_buffer()
    {
        std::unique_lock<std::mutex> lk(buffer_mutex);
        return last_posted;
    }

private:
    std::shared_ptr<mga::AndroidGraphicBufferAllocator> allocator;
    std::shared_ptr<mg::Buffer> client_buffer;
    std::shared_ptr<mg::Buffer> last_posted;
    std::mutex buffer_mutex;
    MirPixelFormat surface_pf;
};

}

struct TestClientIPCRender : public testing::Test
{
    /* note: we fork here so that the loaded driver code does not fork */
    static void SetUpTestCase() {

        render_single_client_process = mtf::fork_and_run_in_a_different_process(
            std::bind(TestClient::render_cpu_pattern, std::ref(sync1), 1),
            TestClient::exit_function);
        render_double_client_process = mtf::fork_and_run_in_a_different_process(
            std::bind(TestClient::render_cpu_pattern, std::ref(sync2), 2),
            TestClient::exit_function);
        render_accelerated_process = mtf::fork_and_run_in_a_different_process(
            std::bind(TestClient::render_rgb_with_gl, std::ref(sync3), 1),
            TestClient::exit_function);
        render_accelerated_process_double = mtf::fork_and_run_in_a_different_process(
            std::bind(TestClient::render_rgb_with_gl, std::ref(sync4), 2),
            TestClient::exit_function);
    }

    void SetUp() {
        buffer_converter = std::make_shared<mtd::TestGrallocMapper>();

        /* start a server */
        mock_server = std::make_shared<StubServerGenerator>();
        test_server = std::make_shared<mt::TestProtobufServer>(socket_file, mock_server);
        test_server->comm->start();
    }

    void TearDown()
    {
        test_server.reset();
        std::remove(socket_file);
    }

    std::shared_ptr<mt::TestProtobufServer> test_server;
    std::shared_ptr<StubServerGenerator> mock_server;

    std::shared_ptr<mtd::TestGrallocMapper> buffer_converter;

    static std::shared_ptr<mtf::Process> render_single_client_process;
    static std::shared_ptr<mtf::Process> render_double_client_process;
    static std::shared_ptr<mtf::Process> render_accelerated_process;
    static std::shared_ptr<mtf::Process> render_accelerated_process_double;
    static mtf::CrossProcessSync sync1, sync2, sync3, sync4;
};

mtf::CrossProcessSync TestClientIPCRender::sync1;
std::shared_ptr<mtf::Process> TestClientIPCRender::render_single_client_process;
mtf::CrossProcessSync TestClientIPCRender::sync2;
std::shared_ptr<mtf::Process> TestClientIPCRender::render_double_client_process;
mtf::CrossProcessSync TestClientIPCRender::sync3;
std::shared_ptr<mtf::Process> TestClientIPCRender::render_accelerated_process;
mtf::CrossProcessSync TestClientIPCRender::sync4;
std::shared_ptr<mtf::Process> TestClientIPCRender::render_accelerated_process_double;

TEST_F(TestClientIPCRender, test_render_single)
{
    sync1.try_signal_ready_for();

    /* wait for client to finish */
    EXPECT_TRUE(render_single_client_process->wait_for_termination().succeeded());

    /* check content */
    auto buf = mock_server->last_posted_buffer();
    auto region = buffer_converter->graphic_region_from_handle(buf->native_buffer_handle()->anwb());
    EXPECT_TRUE(draw_pattern0.check(*region));
}

TEST_F(TestClientIPCRender, test_render_double)
{
    sync2.try_signal_ready_for();

    /* wait for client to finish */
    EXPECT_TRUE(render_double_client_process->wait_for_termination().succeeded());

    auto buf0 = mock_server->second_to_last_posted_buffer();
    auto region0 = buffer_converter->graphic_region_from_handle(buf0->native_buffer_handle()->anwb());
    EXPECT_TRUE(draw_pattern0.check(*region0));

    auto buf1 = mock_server->last_posted_buffer();
    auto region1 = buffer_converter->graphic_region_from_handle(buf1->native_buffer_handle()->anwb());
    EXPECT_TRUE(draw_pattern1.check(*region1));
}

TEST_F(TestClientIPCRender, test_accelerated_render)
{
    mtd::DrawPatternSolid red_pattern(0xFF0000FF);

    sync3.try_signal_ready_for();
    /* wait for client to finish */
    EXPECT_TRUE(render_accelerated_process->wait_for_termination().succeeded());

    /* check content */
    auto buf0 = mock_server->last_posted_buffer();
    auto region0 = buffer_converter->graphic_region_from_handle(buf0->native_buffer_handle()->anwb());
    EXPECT_TRUE(red_pattern.check(*region0));
}

TEST_F(TestClientIPCRender, test_accelerated_render_double)
{
    mtd::DrawPatternSolid red_pattern(0xFF0000FF);
    mtd::DrawPatternSolid green_pattern(0xFF00FF00);

    sync4.try_signal_ready_for();
    /* wait for client to finish */
    EXPECT_TRUE(render_accelerated_process_double->wait_for_termination().succeeded());

    auto buf0 = mock_server->second_to_last_posted_buffer();
    auto region0 = buffer_converter->graphic_region_from_handle(buf0->native_buffer_handle()->anwb());
    EXPECT_TRUE(red_pattern.check(*region0));

    auto buf1 = mock_server->last_posted_buffer();
    auto region1 = buffer_converter->graphic_region_from_handle(buf1->native_buffer_handle()->anwb());
    EXPECT_TRUE(green_pattern.check(*region1));
}
