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

#ifndef MIR_DISPLAY_PLATFORM_H
#define MIR_DISPLAY_PLATFORM_H

#include "mir/graphics/platform.h"

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
namespace rpi
{
class DisplayPlatform : public graphics::DisplayPlatform
{
public:
    DisplayPlatform();

    auto create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<GLConfig> const& gl_config)
        ->UniqueModulePtr<graphics::Display> override;
    NativeDisplayPlatform* native_display_platform() override;
    std::vector<ExtensionDescription> extensions() const override;

private:
    EGLDisplay dpy;
};
}
}
}

#endif  // MIR_DISPLAY_PLATFORM_H
