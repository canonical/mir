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
#include "src/server/graphics/android/internal_client_interpreter.h"
#include "mir/compositor/swapper_factory.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/graphics/android/mir_native_window.h"
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

TEST_F(AndroidInternalClient, creation)
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
    auto interpreter = std::make_shared<mga::InternalClientInterpreter>(std::move(swapper)); 
//    auto mnw = std::make_shared<mga::MirNativeWindow>(interpreter); 
#if 0
        Buffer a, b;
        BufferSwapperMulti bsm({a,b});
        BufferBundle bb(bsm);
        Surface s(bb)
        MirNativeWindow mnw(s);


        /* egl setup junk */
        EGLNativeWindowType egl_nwt = (EGLNativeWindowType*) mnw;
        eglCreateSurface(egl_nwt)

        //render [pattern] 

        eglSwapBuffers();

        auto test = swapper->compositor_acquire(); 
        check_buffer(test, [pattern]) 
#endif

}
