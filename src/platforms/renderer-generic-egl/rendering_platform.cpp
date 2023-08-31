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

#include "rendering_platform.h"
#include "buffer_allocator.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/egl_error.h"

#include <EGL/egl.h>
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mge = mg::egl::generic;

namespace
{
auto egl_display_from_platforms(std::vector<std::shared_ptr<mg::DisplayInterfaceProvider>> const& displays) -> EGLDisplay
{
    for (auto const& display : displays)
    {
        if (auto egl_provider = display->acquire_interface<mg::GenericEGLDisplayProvider>())
        {
            return egl_provider->get_egl_display();
        }
    }
    // No Displays provide an EGL display
    // We can still work, falling back to CPU-copy output, as long as we can get *any* EGL display
    auto dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to create any EGL display"}));
    }
    return dpy;
} 

auto make_share_only_context(EGLDisplay dpy) -> EGLContext
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
    
    auto ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, context_attr);
    if (ctx == EGL_NO_CONTEXT)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
    }
    return ctx;
}
}

mge::RenderingPlatform::RenderingPlatform(std::vector<std::shared_ptr<DisplayInterfaceProvider>> const& displays)
    : dpy{egl_display_from_platforms(displays)},
      ctx{make_share_only_context(dpy)}
{
}

auto mge::RenderingPlatform::create_buffer_allocator() -> mir::UniqueModulePtr<mg::GraphicBufferAllocator>
{
    return make_module_ptr<mge::BufferAllocator>(dpy, ctx);
}

auto mge::RenderingPlatform::maybe_create_interface(RendererInterfaceBase::Tag const& tag)
    -> std::shared_ptr<RendererInterfaceBase>
{
    if (dynamic_cast<GLRenderingProvider::Tag const*>(&tag))
    {
        return std::make_shared<mge::GLRenderingProvider>(dpy, ctx);
    }
    return nullptr;
}
