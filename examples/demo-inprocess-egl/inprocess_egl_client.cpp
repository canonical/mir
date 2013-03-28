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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "inprocess_egl_client.h"
#include "example_egl_helper.h"

#include "mir/display_server.h"
#include "mir/shell/surface_factory.h"
#include "mir/shell/surface.h"
#include "mir/frontend/surface_creation_parameters.h"
#include "mir/geometry/size.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/graphics/egl/mesa_native_display.h"

#include "mir/draw/graphics.h"

#include <EGL/egl.h>

#include <functional>

#include <assert.h>

namespace mf = mir::frontend;
namespace mc = mir::compositor;
namespace me = mir::examples;
namespace mgeglm = mir::graphics::egl::mesa;
namespace geom = mir::geometry;

me::InprocessEGLClient::InprocessEGLClient(mir::DisplayServer* server)
  : server(server)
{
}

void me::InprocessEGLClient::start()
{
    client_thread = std::thread(std::mem_fn(&InprocessEGLClient::thread_loop), this);
}

void me::InprocessEGLClient::thread_loop()
{
    geom::Size const surface_size = geom::Size{geom::Width{512},
                                               geom::Height{512}};
    
    auto params = mf::a_surface().of_name("Inprocess EGL Demo")
        .of_size(surface_size)
        .of_buffer_usage(mc::BufferUsage::hardware)
        .of_pixel_format(geom::PixelFormat::argb_8888);
    auto surface_factory = server->surface_factory();
    auto surface = surface_factory->create_surface(params);
    
    surface->advance_client_buffer(); // TODO: What a wart!
    
    auto native_display = mgeglm::create_native_display(server);
    me::EGLHelper helper(reinterpret_cast<EGLNativeDisplayType>(native_display.get()), reinterpret_cast<EGLNativeWindowType>(surface.get()));

    auto rc = eglMakeCurrent(helper.the_display(), helper.the_surface(), helper.the_surface(), helper.the_context());
    assert(rc == EGL_TRUE);
    
    mir::draw::glAnimationBasic gl_animation;
    gl_animation.init_gl();
    
    for(;;)
    {
        gl_animation.render_gl();
        rc = eglSwapBuffers(helper.the_display(), helper.the_surface());
        assert(rc = EGL_TRUE);

        gl_animation.step();
    }
}
