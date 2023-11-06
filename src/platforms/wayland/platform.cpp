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

#include <epoxy/egl.h>

#include "wl_egl_display_provider.h"
#include "platform.h"
#include "display.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/platform.h"

namespace mg = mir::graphics;
namespace mgw = mir::graphics::wayland;
using namespace std::literals;

namespace
{
auto make_initialised_egl_display(struct wl_display* wl_display) -> EGLDisplay
{
    EGLDisplay dpy;
    // TODO: When we require EGL 1.5 support this check can go away; the EXT_platform_base is core in EGL 1.5
    if (epoxy_has_egl_extension(EGL_NO_DISPLAY, "EGL_EXT_platform_base"))
    {
        if (!epoxy_has_egl_extension(EGL_NO_DISPLAY, "EGL_EXT_platform_wayland"))
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"EGL implementation does not have Wayland support"}));
        }
        dpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_WAYLAND_KHR, wl_display, nullptr);
    }
    else
    {
        dpy = eglGetDisplay(wl_display);
    }
    if (dpy == EGL_NO_DISPLAY)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to get EGLDisplay")));
    }

    std::tuple<EGLint, EGLint> version;
    if (eglInitialize(dpy, &std::get<0>(version), &std::get<1>(version)) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to initialise EGLDisplay")));
    }
    if (version < std::make_tuple(1, 4))
    {
        BOOST_THROW_EXCEPTION((
            std::runtime_error{
                "EGL version unsupported. Require at least 1.4, got " +
                std::to_string(std::get<0>(version)) + 
                "." + 
                std::to_string(std::get<1>(version))
            }));
    }
    return dpy;
}
}

mgw::Platform::Platform(struct wl_display* const wl_display, std::shared_ptr<mg::DisplayReport> const& report) :
    wl_display{wl_display},
    report{report},
    provider{std::make_shared<WlDisplayProvider>(make_initialised_egl_display(wl_display))}
{
}

mir::UniqueModulePtr<mg::Display> mgw::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const&,
    std::shared_ptr<GLConfig> const& gl_config)
{
    return mir::make_module_ptr<mgw::Display>(wl_display, provider, gl_config, report);
}

auto mgw::Platform::maybe_create_provider(const DisplayProvider::Tag& type_tag) -> std::shared_ptr<DisplayProvider>
{
    if (dynamic_cast<GenericEGLDisplayProvider const*>(&type_tag))
    {
        return provider;
    }
    return nullptr;
}
