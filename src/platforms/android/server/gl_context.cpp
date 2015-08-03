/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "gl_context.h"
#include "framebuffer_bundle.h"
#include "mir/graphics/android/android_format_conversion-inl.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_error.h"

#include <algorithm>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <sstream>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;

namespace
{

static EGLint const default_egl_context_attr[] =
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

static EGLint const dummy_pbuffer_attribs[] =
{
    EGL_WIDTH, 1,
    EGL_HEIGHT, 1,
    EGL_NONE
};

static EGLDisplay create_and_initialize_display()
{
    EGLint major, minor;

    auto egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(mg::egl_error("eglGetDisplay failed"));

    if (eglInitialize(egl_display, &major, &minor) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(mg::egl_error("eglInitialize failure"));

    if ((major != 1) || (minor != 4))
        BOOST_THROW_EXCEPTION(std::runtime_error("must have EGL 1.4"));
    return egl_display;
}

/* the minimum requirement is to have EGL_WINDOW_BIT and EGL_OPENGL_ES2_BIT, and to select a config
   whose pixel format matches that of the framebuffer. */
EGLConfig select_egl_config_with_format(
    EGLDisplay egl_display, MirPixelFormat display_format,
    mg::GLConfig const& gl_config)
{
    EGLint const required_egl_config_attr [] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_DEPTH_SIZE, gl_config.depth_buffer_bits(),
        EGL_STENCIL_SIZE, gl_config.stencil_buffer_bits(),
        EGL_NONE
    };

    int required_visual_id = mga::to_android_format(display_format);
    int num_potential_configs;
    EGLint num_match_configs;

    eglGetConfigs(egl_display, NULL, 0, &num_potential_configs);
    std::vector<EGLConfig> config_slots(num_potential_configs);

    eglChooseConfig(egl_display, required_egl_config_attr, config_slots.data(), num_potential_configs, &num_match_configs);
    config_slots.resize(num_match_configs);

    auto const pegl_config = std::find_if(begin(config_slots), end(config_slots),
        [&](EGLConfig& current) -> bool
        {
            int visual_id;
            eglGetConfigAttrib(egl_display, current, EGL_NATIVE_VISUAL_ID, &visual_id);
            return (visual_id == required_visual_id);
        });

    if (pegl_config == end(config_slots))
        BOOST_THROW_EXCEPTION(std::runtime_error("could not select EGL config for use with framebuffer"));

    return *pegl_config;
}

}

void mga::GLContext::make_current(EGLSurface egl_surface) const
{
    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(
            mg::egl_error("could not activate surface with eglMakeCurrent"));
    }
}

void mga::GLContext::release_current() const
{
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}


mga::GLContext::~GLContext()
{
    if (eglGetCurrentContext() == egl_context)
        release_current();
    if (own_display)
        eglTerminate(egl_display);
}

mga::GLContext::GLContext(
    MirPixelFormat display_format, mg::GLConfig const& gl_config, mg::DisplayReport& report) :
    egl_display(create_and_initialize_display()),
    egl_config(select_egl_config_with_format(egl_display, display_format, gl_config)),
    egl_context{egl_display,
                eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, default_egl_context_attr)},
    own_display(true)
{
    report.report_egl_configuration(egl_display, egl_config);
}

mga::GLContext::GLContext(GLContext const& shared_gl_context) :
    mg::GLContext(),
    egl_display(shared_gl_context.egl_display),
    egl_config(shared_gl_context.egl_config),
    egl_context{egl_display,
                eglCreateContext(egl_display, egl_config, shared_gl_context.egl_context,
                                 default_egl_context_attr)},
    own_display(false)
{
}

mga::PbufferGLContext::PbufferGLContext(
    MirPixelFormat display_format, mg::GLConfig const& gl_config, mg::DisplayReport& report) :
    GLContext(display_format, gl_config, report),
    egl_surface{egl_display,
                eglCreatePbufferSurface(egl_display, egl_config, dummy_pbuffer_attribs)}
{
}

mga::PbufferGLContext::PbufferGLContext(PbufferGLContext const& shared_gl_context) :
    GLContext(shared_gl_context),
    egl_surface{egl_display,
                eglCreatePbufferSurface(egl_display, egl_config, dummy_pbuffer_attribs)}
{
}

void mga::PbufferGLContext::make_current() const
{
    GLContext::make_current(egl_surface);
}

void mga::PbufferGLContext::release_current() const
{
    GLContext::release_current();
}

mga::FramebufferGLContext::FramebufferGLContext(
    GLContext const& shared_gl_context,
    std::shared_ptr<FramebufferBundle> const& fb_bundle,
    std::shared_ptr<ANativeWindow> const& native_window)
     : GLContext(shared_gl_context),
       fb_bundle(fb_bundle),
       egl_surface{egl_display,
                   eglCreateWindowSurface(egl_display, egl_config, native_window.get(), NULL)}
{
}

void mga::FramebufferGLContext::swap_buffers() const
{
    if (eglSwapBuffers(egl_display, egl_surface) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(mg::egl_error("eglSwapBuffers failure"));
}

std::shared_ptr<mg::Buffer> mga::FramebufferGLContext::last_rendered_buffer() const
{
    return fb_bundle->last_rendered_buffer();
}

void mga::FramebufferGLContext::make_current() const
{
    GLContext::make_current(egl_surface);
}

void mga::FramebufferGLContext::release_current() const
{
    GLContext::release_current();
}
