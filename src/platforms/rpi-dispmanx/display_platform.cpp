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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <boost/throw_exception.hpp>

#include "mir/graphics/egl_error.h"

#include "display_platform.h"
#include "display.h"

namespace mg = mir::graphics;

mg::rpi::DisplayPlatform::DisplayPlatform()
    : dpy{eglGetDisplay(EGL_DEFAULT_DISPLAY)}
{
    if (dpy == EGL_NO_DISPLAY)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL Display"));
    }

    EGLint major, minor;
    if (!eglInitialize(dpy, &major, &minor))
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to initialise EGL"));
    }
    if (major != 1 || minor < 4)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{
            std::string{"Unsupported EGL version. Need > 1.4, found: "}
            + std::to_string(major) + "." + std::to_string(minor)
        }));
    }
}

auto mir::graphics::rpi::DisplayPlatform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& /*initial_conf_policy*/,
    std::shared_ptr<GLConfig> const& gl_config)
    -> mir::UniqueModulePtr<graphics::Display>
{
    return mir::make_module_ptr<rpi::Display>(dpy, *gl_config, 0);
}

mir::graphics::NativeDisplayPlatform* mir::graphics::rpi::DisplayPlatform::native_display_platform()
{
    return nullptr;
}

auto mir::graphics::rpi::DisplayPlatform::extensions() const -> std::vector<ExtensionDescription>
{
    BOOST_THROW_EXCEPTION((std::runtime_error{"rpi-dispmanx platform does not support mirclient"}));
}
