/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 *
 */

#ifndef MIR_GRAPHICS_MESA_EXTENSIONS_H_
#define MIR_GRAPHICS_MESA_EXTENSIONS_H_
#include "mir/extension_description.h"
#include <vector>

namespace mir
{
namespace graphics
{
namespace mesa
{
inline std::vector<ExtensionDescription> mesa_extensions()
{
    return
    {
        { "mir_extension_gbm_buffer", { 1, 2 } },
        { "mir_extension_graphics_module", { 1 } },
        { "mir_extension_mesa_drm_auth", { 1 } },
        { "mir_extension_set_gbm_device", { 1 } },
        { "mir_extension_hardware_buffer_stream", { 1 } }
    };
}
}
}
}
#endif /* MIR_GRAPHICS_MESA_EXTENSIONS_H_ */
