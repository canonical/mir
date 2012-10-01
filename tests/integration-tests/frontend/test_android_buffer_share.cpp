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

#include "mir/graphics/android/android_platform.h"
#include "mir/graphics/android/android_buffer_allocator.h"
#include "mir/process/process.h"

#include <gtest/gtest.h>

#include <memory>

namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace mp=mir::process;
namespace geom=mir::geometry;

namespace 
{

static void connected_callback(MirConnection *connection, void *client_context)
{

}

static void created_callback(MirSurface *surface, void* client_context)
{

}

struct MainFunctions
{
    static int normal_main()
    {
        /* use C api */
        {
            void* context = NULL; 

            /* connect over api */
            mir_connect("./test_send_buffer_fd", 
                        "test_normal_main",
                        connected_callback,
                        context);

            /* wait_for_connect */

#if 0
            /* ask a buffer */
            MirConnection connection;


            mir_surface_lifecycle_callback* callback;
            callback = mir_surface_created_callback;

            MirSurfaceParameters parameters;
            parameters.name = "test_send";
            parameters.width = 42;
            parameters.height= 44;
            parameters.pixel_format = mir_pixel_format_rgba_8888;
        
            auto wait_handle = mir_surface_create(connection, &parameters, callback, context);

            /* wait for send */
            mir_wait_for(wait_handle);

            /* create client side buffer */
            /* verify buffer creation */
#endif
            return 0;
        }
    };
};

struct ExitFunctions
{
    static int exit_func()
    {
        return 0;
    }
};

}
 
class AndroidBufferShare : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        allocator = mg::create_platform()->create_buffer_allocator();
        auto pf  = geom::PixelFormat::rgba_8888;
        auto size = geom::Size{geom::Width{10},
                          geom::Height{20}};
        server_buffer = allocator->alloc_buffer(size, pf);
    }

    std::shared_ptr<mc::GraphicBufferAllocator> allocator;
    std::shared_ptr<mc::Buffer> server_buffer;
};

TEST_F(AndroidBufferShare, client_send_sucessful)
{
    using namespace testing;

    auto p = mp::fork_and_run_in_a_different_process(
        std::bind(
            MainFunctions::normal_main
        ),
        ExitFunctions::exit_func );

    /* wait for ask */
    /* send */
    p->wait_for_termination();
}
