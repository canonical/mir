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

#include "mir/main_loop.h"
#include "mir/shell/focus_controller.h"
#include "mir/frontend/surface.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/shell/session.h"
#include "mir/frontend/session.h"
#include "mir/frontend/shell.h"
#include "mir/geometry/size.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/platform.h"
#include "mir/input/input_receiver_thread.h"
#include "mir/input/input_platform.h"
#include "mir/graphics/internal_client.h"
#include "mir/graphics/internal_surface.h"
#include "mir/frontend/event_sink.h"

#include "graphics.h"

#include <EGL/egl.h>

#include <xkbcommon/xkbcommon-keysyms.h>

#include <functional>

#include <assert.h>
#include <signal.h>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace me = mir::examples;
namespace mircv = mir::input::receiver;
namespace geom = mir::geometry;

me::InprocessEGLClient::InprocessEGLClient(
    std::shared_ptr<mg::Platform> const& graphics_platform,
    std::shared_ptr<frontend::Shell> const& shell,
    std::shared_ptr<shell::FocusController> const& focus_controller)
  : graphics_platform(graphics_platform),
    shell(shell),
    focus_controller(focus_controller),
    client_thread(std::mem_fn(&InprocessEGLClient::thread_loop), this),
    terminate(false)
{
}

me::InprocessEGLClient::~InprocessEGLClient()
{
    terminate = true;
    auto session = focus_controller->focussed_application().lock();
    if (session)
        session->force_requests_to_complete();
    client_thread.join();
}

namespace
{
struct NullEventSink : mf::EventSink
{
    void handle_event(MirEvent const& /*e*/) {}
    void handle_lifecycle_event(MirLifecycleState /*state*/) {}
    void handle_display_config_change(mg::DisplayConfiguration const& /*config*/) {}
};
}

void me::InprocessEGLClient::thread_loop()
{
    geom::Size const surface_size{512, 512};

    ///\internal [setup_tag]
    auto params = msh::a_surface().of_name("Inprocess EGL Demo")
        .of_size(surface_size)
        .of_buffer_usage(mg::BufferUsage::hardware)
        .of_pixel_format(mir_pixel_format_argb_8888);
    auto session = shell->open_session("Inprocess client", std::make_shared<NullEventSink>());
    // TODO: Why do we get an ID? ~racarr
    auto surface = session->get_surface(shell->create_surface_for(session, params));

    auto input_platform = mircv::InputPlatform::create();
    input_thread = input_platform->create_input_thread(
        surface->client_input_fd(), 
            std::bind(std::mem_fn(&me::InprocessEGLClient::handle_event), this, std::placeholders::_1));
    input_thread->start();

    auto internal_client = graphics_platform->create_internal_client();
    auto internal_surface = as_internal_surface(surface);
    me::EGLHelper helper(internal_client->egl_native_display(), internal_client->egl_native_window(internal_surface));

    auto rc = eglMakeCurrent(helper.the_display(), helper.the_surface(), helper.the_surface(), helper.the_context());
    assert(rc == EGL_TRUE);

    mir::draw::glAnimationBasic gl_animation;
    gl_animation.init_gl();
    ///\internal [setup_tag]

    ///\internal [loop_tag]
    while(!terminate)
    {
        gl_animation.render_gl();
        rc = eglSwapBuffers(helper.the_display(), helper.the_surface());
        assert(rc == EGL_TRUE);

        gl_animation.step();
    }

    (void)rc;

    input_thread->stop();
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
    terminate = true;
}
