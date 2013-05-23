/*
 * Copyright © 2013 Canonical Ltd.
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

#include "src/server/graphics/android/android_graphic_buffer_allocator.h"
#include "src/server/graphics/android/internal_client_window.h"
#include "src/server/graphics/android/interpreter_cache.h"
#include "mir/compositor/swapper_factory.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/graphics/null_display_report.h"
#include "mir/graphics/android/mir_native_window.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/internal_client.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/shell/surface_source.h"
#include "mir/shell/surface.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/frontend/surface_id.h"
#include "mir/input/input_channel_factory.h"

#include <EGL/egl.h>
#include <gtest/gtest.h>

#include <GLES2/gl2.h>


namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mc=mir::compositor;
namespace geom=mir::geometry;
namespace ms=mir::surfaces;
namespace msh=mir::shell;
namespace mf=mir::frontend;
namespace mi=mir::input;

namespace
{
class AndroidInternalClient : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }
};

struct StubInputFactory : public mi::InputChannelFactory 
{
    std::shared_ptr<mi::InputChannel> make_input_channel()
    {
        return std::shared_ptr<mi::InputChannel>();
    }
};
}

TEST_F(AndroidInternalClient, internal_client_creation_and_use)
{
    auto size = geom::Size{geom::Width{334},
                      geom::Height{122}};
    auto pf  = geom::PixelFormat::abgr_8888;
    msh::SurfaceCreationParameters params;
    params.name = std::string("test");
    params.size = size; 
    params.pixel_format = pf;
    params.buffer_usage = mc::BufferUsage::hardware; 
    auto id = mf::SurfaceId{4458};

    auto stub_input_factory = std::make_shared<StubInputFactory>();
    auto null_buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    auto allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>(null_buffer_initializer);
    auto strategy = std::make_shared<mc::SwapperFactory>(allocator);
    auto buffer_bundle_factory = std::make_shared<mc::BufferBundleManager>(strategy);
    auto ss = std::make_shared<ms::SurfaceStack>(buffer_bundle_factory);
    auto surface_controller = std::make_shared<ms::SurfaceController>(ss);
    auto surface_source = std::make_shared<msh::SurfaceSource>(surface_controller, stub_input_factory);
    auto mir_surface = surface_source->create_surface(params, id, std::shared_ptr<mir::events::EventSink>());

    auto report = std::shared_ptr<mg::NullDisplayReport>(); 
    auto platform = mg::create_platform(report);
    auto internal_client = platform->create_internal_client();

    int major, minor, n;
    EGLContext egl_context;
    EGLSurface egl_surface;
    EGLConfig egl_config;
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE };
    EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    auto egl_display = eglGetDisplay(internal_client->egl_native_display());
    int rc = eglInitialize(egl_display, &major, &minor);
    EXPECT_EQ(EGL_TRUE, rc);

    rc = eglChooseConfig(egl_display, attribs, &egl_config, 1, &n);
    EXPECT_EQ(EGL_TRUE, rc);

    egl_surface = eglCreateWindowSurface(egl_display, egl_config, internal_client->egl_native_window(mir_surface), NULL);
    EXPECT_NE(EGL_NO_SURFACE, egl_surface);

    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
    EXPECT_NE(EGL_NO_CONTEXT, egl_context);

    rc = eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
    EXPECT_EQ(EGL_TRUE, rc);

    glClearColor(1.0f, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    rc = eglSwapBuffers(egl_display, egl_surface);
    EXPECT_EQ(EGL_TRUE, rc);
}
