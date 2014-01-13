/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_INTERNAL_NATIVE_DISPLAY_H_
#define MIR_GRAPHICS_MESA_INTERNAL_NATIVE_DISPLAY_H_

#include "mir_toolkit/mesa/native_display.h"
#include <memory>

namespace mir
{
namespace graphics
{
struct PlatformIPCPackage;
namespace mesa
{

class InternalNativeDisplay : public MirMesaEGLNativeDisplay
{
public:
    InternalNativeDisplay(std::shared_ptr<PlatformIPCPackage> const& platform_package);

private:
    static int native_display_get_platform(MirMesaEGLNativeDisplay* display, MirPlatformPackage* package);

    std::shared_ptr<PlatformIPCPackage> platform_package;
};

}
}
}
#endif /* MIR_GRAPHICS_MESA_INTERNAL_NATIVE_DISPLAY_H_ */
