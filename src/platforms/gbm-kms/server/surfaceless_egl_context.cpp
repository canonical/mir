/*
 * Copyright © Canonical Ltd.
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

#include "surfaceless_egl_context.h"
#include "mir/graphics/egl_error.h"

#include <optional>
#include <EGL/egl.h>
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;

namespace
{
auto make_share_only_context(EGLDisplay dpy, std::optional<EGLContext> share_with) -> EGLContext
{
    eglBindAPI(EGL_OPENGL_ES_API);

    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig cfg;
    EGLint num_configs;

    if (eglChooseConfig(dpy, config_attr, &cfg, 1, &num_configs) != EGL_TRUE || num_configs != 1)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to find any matching EGL config")));
    }

    auto ctx = eglCreateContext(dpy, cfg, share_with.value_or(EGL_NO_CONTEXT), context_attr);
    if (ctx == EGL_NO_CONTEXT)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
    }
    return ctx;
}
}

mg::gbm::SurfacelessEGLContext::SurfacelessEGLContext(EGLDisplay dpy)
        : dpy{dpy},
          ctx{make_share_only_context(dpy, {})}
    {
    }

mg::gbm::SurfacelessEGLContext::SurfacelessEGLContext(EGLDisplay dpy, EGLContext share_with)
    : dpy{dpy},
      ctx{make_share_only_context(dpy, share_with)}
{
}

mg::gbm::SurfacelessEGLContext::~SurfacelessEGLContext()
{
    release_current();
    eglDestroyContext(dpy, ctx);
}

void mg::gbm::SurfacelessEGLContext::make_current() const
{
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make context current"));
    }
}

void mg::gbm::SurfacelessEGLContext::release_current() const
{
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release current EGL context"));
    }
}

auto mg::gbm::SurfacelessEGLContext::make_share_context() const -> std::unique_ptr<Context>
{
    return std::unique_ptr<Context>{new SurfacelessEGLContext{dpy, ctx}};
}

mg::gbm::SurfacelessEGLContext::operator EGLContext()
{
    return ctx;
}

auto mg::gbm::SurfacelessEGLContext::egl_display() const -> EGLDisplay
{
    return dpy;
}
