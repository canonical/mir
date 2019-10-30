/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <epoxy/egl.h>

#include "display.h"
#include "cursor.h"

#include <mir/fatal.h>
#include <mir/graphics/display_configuration.h>
#include <mir/graphics/egl_error.h>
#include <mir/graphics/virtual_output.h>
#include <mir/log.h>
#include <mir/renderer/gl/context.h>

#include <boost/throw_exception.hpp>

namespace mgw = mir::graphics::wayland;
namespace mrg = mir::renderer::gl;
namespace geom= mir::geometry;

mgw::Display::Display(
    wl_display* const wl_display,
    std::shared_ptr<GLConfig> const& gl_config,
    std::shared_ptr<DisplayReport> const& report) :
    DisplayClient{wl_display, gl_config},
    report{report}
{
}

auto mgw::Display::configuration() const -> std::unique_ptr<DisplayConfiguration>
{
    return DisplayClient::display_configuration();
}

void mgw::Display::configure(DisplayConfiguration const& /*conf*/)
{
    puts(__PRETTY_FUNCTION__);
}

void mgw::Display::register_configuration_change_handler(
    EventHandlerRegister& /*handlers*/,
    DisplayConfigurationChangeHandler const& /*conf_change_handler*/)
{
}

void mgw::Display::register_pause_resume_handlers(
    EventHandlerRegister& /*handlers*/,
    DisplayPauseHandler const& /*pause_handler*/,
    DisplayResumeHandler const& /*resume_handler*/)
{
}

void mgw::Display::pause()
{
   BOOST_THROW_EXCEPTION(std::runtime_error("'Display::pause()' not yet supported on wayland platform"));
}

void mgw::Display::resume()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("'Display::resume()' not yet supported on wayland platform"));
}

auto mgw::Display::create_hardware_cursor() -> std::shared_ptr<Cursor>
{
    return std::make_shared<platform::wayland::Cursor>(display, std::shared_ptr<CursorImage>{});
}

auto mgw::Display::create_virtual_output(int /*width*/, int /*height*/) -> std::unique_ptr<VirtualOutput>
{
    return {};
}

auto mgw::Display::native_display() -> NativeDisplay*
{
    return this;
}

bool mgw::Display::apply_if_configuration_preserves_display_buffers(DisplayConfiguration const& /*conf*/)
{
    return false;
}

auto mgw::Display::last_frame_on(unsigned) const -> Frame
{
    fatal_error(__PRETTY_FUNCTION__);
    return {};
}

auto mgw::Display::create_gl_context() const -> std::unique_ptr<mrg::Context>
{
    class GlContext : public mir::renderer::gl::Context
    {
    public:
        GlContext(EGLDisplay egldisplay, EGLConfig eglconfig, EGLContext eglctx) :
            egldisplay{egldisplay},
            eglctx{eglCreateContext(egldisplay, eglconfig, eglctx, nullptr)} {}

        void make_current()     const override
        {
            if (eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, eglctx) != EGL_TRUE)
            {
                log_warning(
                    "%s FAILED: %s",
                    __PRETTY_FUNCTION__,
                    egl_category().message(eglGetError()).c_str());
            }
        }

        void release_current()  const override
            { eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT); }

    private:
        EGLDisplay const egldisplay;
        EGLContext const eglctx;
    };

    return std::make_unique<GlContext>(egldisplay, eglconfig, eglctx);
}

void  mgw::Display::for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f)
{
    DisplayClient::for_each_display_sync_group(f);
}
