/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_ANDROID_DISPLAY_NAME_H_
#define MIR_GRAPHICS_ANDROID_DISPLAY_NAME_H_

#include "mir/graphics/display_configuration.h"
#include <hardware/hwcomposer.h>

namespace mir
{
namespace graphics
{
namespace android
{

enum class DisplayName
{
    primary = HWC_DISPLAY_PRIMARY,
    external = HWC_DISPLAY_EXTERNAL,
    virt = HWC_DISPLAY_VIRTUAL
};

inline auto as_output_id(DisplayName name) -> DisplayConfigurationOutputId
{
    return DisplayConfigurationOutputId{1 + static_cast<int>(name)};
}

inline auto as_hwc_display(DisplayName name) -> int
{
    return static_cast<int>(name);
}
}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DISPLAY_NAME_H_ */
