/*
 * Copyright © 2016 Canonical Ltd.
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
 */

#ifndef MIR_GRAPHICS_MESA_NATIVE_BUFFER_H_
#define MIR_GRAPHICS_MESA_NATIVE_BUFFER_H_

#include <mir_toolkit/mir_native_buffer.h>
#include "mir/graphics/native_buffer.h"

#include <cstdlib>
#include <gbm.h>

namespace mir
{
namespace graphics
{
namespace mesa
{
struct NativeBuffer : graphics::NativeBuffer, MirBufferPackage
{
    struct gbm_bo *bo;
    bool is_gbm_buffer;
    uint32_t native_format;
    uint32_t native_flags;
};
}
}
}

#endif /* MIR_GRAPHICS_MESA_NATIVE_BUFFER_H_ */
