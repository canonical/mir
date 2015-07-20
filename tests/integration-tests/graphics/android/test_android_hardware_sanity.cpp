/*
 * Copyright © 2012-2015 Canonical Ltd.
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

#include "mir/default_server_configuration.h"
#include "mir/compositor/compositor.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/buffer.h"

#include "mir_test_framework/server_runner.h"
#include "mir/test/doubles/stub_display.h"
#include "mir/test/doubles/stub_renderable.h"
#include "mir/test/validity_matchers.h"
#include "patterns.h"
#include "examples/graphics.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>

namespace mtf = mir_test_framework;
namespace mt = mir::test;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mc = mir::compositor;

namespace
{
static int test_width  = 300;
static int test_height = 200;
static uint32_t pattern [2][2] = {{0x12345678, 0x23456789},
                                   {0x34567890, 0x45678901}};
static mt::DrawPatternCheckered<2,2> draw_pattern(pattern);
MirPixelFormat select_format_for_visual_id(int visual_id)
{
    if (visual_id == 5)
        return mir_pixel_format_argb_8888;
    if (visual_id == 1)
        return mir_pixel_format_abgr_8888;
    return mir_pixel_format_invalid;
}

struct NoCompositingServer : mir::DefaultServerConfiguration
{
    NoCompositingServer(int argc, char const* argv[]) :
        DefaultServerConfiguration(argc, argv)
    {
    }

    std::shared_ptr<mg::Cursor> the_cursor() override { return nullptr; }
    std::shared_ptr<mc::Compositor> the_compositor() override
    {
        struct NullCompositor : mc::Compositor
        {
            void start() override {};
            void stop() override {};
        };
        return compositor( [this]() { return std::make_shared<NullCompositor>(); });
    }
};

struct Runner : mtf::ServerRunner
{
    mir::DefaultServerConfiguration& server_config() override
    {
        return config;
    }
    char const* argv = "./aa";
    NoCompositingServer config{1, &argv};
};
 
struct AndroidHardwareSanity : testing::Test
{
    static void SetUpTestCase()
    {
        runner = std::make_unique<Runner>();
        runner->start_server();
    }
    static void TearDownTestCase()
    {
        runner->stop_server();
        runner.reset();
    }
    static std::unique_ptr<Runner> runner;
};
std::unique_ptr<Runner> AndroidHardwareSanity::runner;
}

TEST_F(AndroidHardwareSanity, client_can_draw_with_cpu)
{
    auto connection = mir_connect_sync(runner->new_connection().c_str(), "test_renderer");
    EXPECT_THAT(connection, IsValid());

    auto const spec = mir_connection_create_spec_for_normal_surface(
        connection, test_width, test_height, mir_pixel_format_abgr_8888);
    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_software);
    auto const surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    EXPECT_THAT(surface, IsValid());
    MirGraphicsRegion graphics_region;
    mir_buffer_stream_get_graphics_region(mir_surface_get_buffer_stream(surface), &graphics_region);
    draw_pattern.draw(graphics_region);
    mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));

    auto scene = runner->config.the_scene();
    auto seq = scene->scene_elements_for(this);
    ASSERT_THAT(seq, testing::SizeIs(1));
    auto buffer = seq[0]->renderable()->buffer();
    auto valid_content = false;
    buffer->read([&valid_content, &buffer](unsigned char const* data){
        MirGraphicsRegion region{
            buffer->size().width.as_int(), buffer->size().height.as_int(),
            buffer->stride().as_int(), buffer->pixel_format(),
            reinterpret_cast<char*>(const_cast<unsigned char*>(data))};
        valid_content = draw_pattern.check(region);
    });
    EXPECT_TRUE(valid_content);

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

TEST_F(AndroidHardwareSanity, client_can_draw_with_gpu)
{
    auto connection = mir_connect_sync(runner->new_connection().c_str(), "test_renderer");
    EXPECT_THAT(connection, IsValid());

    int major, minor, n, visual_id;
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

    eglGetConfigAttrib(egl_display, egl_config, EGL_NATIVE_VISUAL_ID, &visual_id);
    auto const spec = mir_connection_create_spec_for_normal_surface(
        connection, test_width, test_height, select_format_for_visual_id(visual_id));
    auto const mir_surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    EXPECT_THAT(mir_surface, IsValid());

    auto native_window = static_cast<EGLNativeWindowType>(
        mir_buffer_stream_get_egl_native_window(mir_surface_get_buffer_stream(mir_surface)));

    egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, NULL);
    context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
    eglMakeCurrent(egl_display, egl_surface, egl_surface, context);

    glClearColor(0.0, 1.0, 0.0, 1.0); //green
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(egl_display, egl_surface);

    auto scene = runner->config.the_scene();
    auto seq = scene->scene_elements_for(this);
    ASSERT_THAT(seq, testing::SizeIs(1));
    auto buffer = seq[0]->renderable()->buffer();
    auto valid_content = false;
    buffer->read([&valid_content, &buffer](unsigned char const* data){
        MirGraphicsRegion region{
            buffer->size().width.as_int(), buffer->size().height.as_int(),
            buffer->stride().as_int(), buffer->pixel_format(),
            reinterpret_cast<char*>(const_cast<unsigned char*>(data))};
        mt::DrawPatternSolid green_pattern(0xFF00FF00);
        valid_content = green_pattern.check(region);
    });
    EXPECT_TRUE(valid_content);
    mir_surface_release_sync(mir_surface);
    mir_connection_release(connection);
}

TEST_F(AndroidHardwareSanity, display_can_post)
{
    auto display = runner->config.the_display();
    display->for_each_display_sync_group([](mg::DisplaySyncGroup& group) {
        group.for_each_display_buffer([](mg::DisplayBuffer& buffer)
        {
            buffer.make_current();
            mir::draw::glAnimationBasic gl_animation;
            gl_animation.init_gl();

            gl_animation.render_gl();
            buffer.gl_swap_buffers();

            gl_animation.render_gl();
            buffer.gl_swap_buffers();
        });
        group.post();
    });
}

TEST_F(AndroidHardwareSanity, display_can_post_overlay)
{
    auto display = runner->config.the_display();
    display->for_each_display_sync_group([](mg::DisplaySyncGroup& group) {
        group.for_each_display_buffer([](mg::DisplayBuffer& db)
        {
            db.make_current();
            auto area = db.view_area();
            mg::BufferProperties properties{
                area.size, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware};
            auto buffer = runner->config.the_buffer_allocator()->alloc_buffer(properties);
            mg::RenderableList list{
                std::make_shared<mt::doubles::StubRenderable>(buffer, area)
            };

            db.post_renderables_if_optimizable(list);
        });
        group.post();
    });
}

TEST_F(AndroidHardwareSanity, can_allocate_sw_buffer)
{
    using namespace testing;

    auto size = geom::Size{334, 122};
    auto pf  = mir_pixel_format_abgr_8888;
    mg::BufferProperties sw_properties{size, pf, mg::BufferUsage::software};
    auto buffer = runner->config.the_buffer_allocator()->alloc_buffer(sw_properties);
    EXPECT_NE(nullptr, buffer);

    int i = 0;
    bool valid_content = false;
    std::vector<unsigned char> px(
        buffer->size().height.as_int() *
        buffer->size().width.as_int() * 
        MIR_BYTES_PER_PIXEL(buffer->pixel_format()));
    uint32_t green{0x00FF00FF};
    mt::DrawPatternSolid green_pattern(green);
    std::generate(px.begin(), px.end(), [&i]{ if(i++ % 2) return 0x00; else return 0xFF; });

    buffer->write(px.data(), px.size());
    buffer->read([&](unsigned char const* data){
        MirGraphicsRegion region{
            buffer->size().width.as_int(), buffer->size().height.as_int(),
            buffer->stride().as_int(), buffer->pixel_format(),
            reinterpret_cast<char*>(const_cast<unsigned char*>(data))};
        valid_content = green_pattern.check(region);
    });
    EXPECT_TRUE(valid_content);
}

TEST_F(AndroidHardwareSanity, can_allocate_hw_buffer)
{
    using namespace testing;

    auto size = geom::Size{334, 122};
    auto pf  = mir_pixel_format_abgr_8888;
    mg::BufferProperties hw_properties{size, pf, mg::BufferUsage::hardware};

    //TODO: kdub it is a bit trickier to test that a gpu can render... just check creation for now
    auto test_buffer = runner->config.the_buffer_allocator()->alloc_buffer(hw_properties);
    EXPECT_NE(nullptr, test_buffer);
}
