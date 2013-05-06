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
#include "mir/graphics/buffer_initializer.h"
#include "mir/graphics/android/mir_native_window.h"

#include <EGL/egl.h>
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mc=mir::compositor;
namespace geom=mir::geometry;

namespace
{
class AndroidInternalClient : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }
};
}

TEST_F(AndroidInternalClient, internal_client_creation_and_use)
{
    auto size = geom::Size{geom::Width{334},
                      geom::Height{122}};
    auto pf  = geom::PixelFormat::abgr_8888;
    auto buffer_properties = mc::BufferProperties{size, pf, mc::BufferUsage::software};


    auto null_buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    auto allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>(null_buffer_initializer);
    auto strategy = std::make_shared<mc::SwapperFactory>(allocator);
    mc::BufferProperties actual;
    auto swapper = strategy->create_swapper(actual, buffer_properties);
    auto cache = std::make_shared<mga::InterpreterCache>();
    auto interpreter = std::make_shared<mga::InternalClientWindow>(std::move(swapper), cache); 
    auto mnw = std::make_shared<mga::MirNativeWindow>(interpreter);

    int major, minor, n;
    //EGLContext context;
    //EGLSurface egl_surface;
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

    auto egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    int rc = eglInitialize(egl_display, &major, &minor);
    EXPECT_EQ(EGL_TRUE, rc);

    rc = eglChooseConfig(egl_display, attribs, &egl_config, 1, &n);
    EXPECT_EQ(EGL_TRUE, rc);

    auto surface = eglCreateWindowSurface(egl_display, egl_config, mnw.get(), NULL);
    EXPECT_NE(EGL_NO_SURFACE, surface);

    auto context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
    EXPECT_NE(EGL_NO_CONTEXT, context);

    rc = eglMakeCurrent(egl_display, surface, surface, context);
    EXPECT_EQ(EGL_TRUE, rc);
}
