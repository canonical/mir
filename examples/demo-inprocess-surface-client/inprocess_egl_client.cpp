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

#include "mir/shell/session_manager.h"
#include "mir/shell/surface.h"
#include "mir/frontend/surface_creation_parameters.h"
#include "mir/frontend/session.h"
#include "mir/geometry/size.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/graphics/platform.h"
#include "mir/input/input_receiver_thread.h"
#include "mir/input/input_platform.h"

#include "graphics.h"

#include <EGL/egl.h>

#include <xkbcommon/xkbcommon-keysyms.h>

#include <functional>

#include <assert.h>

namespace mf = mir::frontend;
namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace me = mir::examples;
namespace mcli = mir::client::input;
namespace geom = mir::geometry;

me::InprocessEGLClient::InprocessEGLClient(std::shared_ptr<mg::Platform> const& graphics_platform,
                                           std::shared_ptr<msh::SessionManager> const& session_manager)
  : graphics_platform(graphics_platform),
    session_manager(session_manager),
    running(true),
    client_thread(std::mem_fn(&InprocessEGLClient::thread_loop), this)
{
    client_thread.detach();
}

void me::InprocessEGLClient::thread_loop()
{
    geom::Size const surface_size = geom::Size{geom::Width{512},
                                               geom::Height{512}};

    ///\internal [setup_tag]
    auto params = mf::a_surface().of_name("Inprocess EGL Demo")
        .of_size(surface_size)
        .of_buffer_usage(mc::BufferUsage::hardware)
        .of_pixel_format(geom::PixelFormat::argb_8888);
    auto session = session_manager->open_session("Inprocess client",
                                                 std::shared_ptr<mir::events::EventSink>());
    // TODO: Why do we get an ID? ~racarr
    auto surface = session->get_surface(session_manager->create_surface_for(session, params));
    
    auto input_platform = mcli::InputPlatform::create();
    input_thread = input_platform->create_input_thread(
        surface->client_input_fd(), 
            std::bind(std::mem_fn(&me::InprocessEGLClient::handle_event), this, std::placeholders::_1));
    input_thread->start();

    surface->advance_client_buffer(); // TODO: What a wart!

    auto native_display = graphics_platform->shell_egl_display();
    me::EGLHelper helper(reinterpret_cast<EGLNativeDisplayType>(native_display), reinterpret_cast<EGLNativeWindowType>(surface.get()));

    auto rc = eglMakeCurrent(helper.the_display(), helper.the_surface(), helper.the_surface(), helper.the_context());
    assert(rc == EGL_TRUE);

    mir::draw::glAnimationBasic gl_animation;
    gl_animation.init_gl();
    ///\internal [setup_tag]

    ///\internal [loop_tag]
    while (running)
    {
        gl_animation.render_gl();
        rc = eglSwapBuffers(helper.the_display(), helper.the_surface());
        assert(rc = EGL_TRUE);

        gl_animation.step();
    }
    ///\internal [loop_tag]
}

void me::InprocessEGLClient::handle_event(MirEvent *event)
{
    if (event->type != mir_event_type_key)
        return;
    if (event->key.action != mir_key_action_down)
        return;
    if (event->key.key_code != XKB_KEY_Escape)
        return;
    input_thread->stop();
    running = false;
}
