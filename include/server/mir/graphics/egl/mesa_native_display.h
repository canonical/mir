/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#ifndef MIR_GRAPHICS_EGL_NATIVE_DISPLAY_H_
#define MIR_GRAPHICS_EGL_NATIVE_DISPLAY_H_

#include "mir_toolkit/mesa/native_display.h"

#include <memory>

namespace mir
{
namespace graphics
{
class Platform;

namespace egl
{
namespace mesa
{
std::shared_ptr<MirMesaEGLNativeDisplay> create_native_display(std::shared_ptr<Platform> const& platform);
}
}
}
} // namespace mir

#endif // MIR_GRAPHICS_EGL_NATIVE_DISPLAY_H_
