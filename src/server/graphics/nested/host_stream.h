/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_HOST_STREAM_H_
#define MIR_GRAPHICS_NESTED_HOST_STREAM_H_

#include "mir_toolkit/client_types.h"
#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
namespace nested
{
class HostStream
{
public:
    virtual ~HostStream() = default;

    virtual MirBufferStream* handle() const = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    virtual MirRenderSurface* rs() const = 0;
#pragma GCC diagnostic pop
    virtual EGLNativeWindowType egl_native_window() const = 0;
protected:
    HostStream() = default;
    HostStream(HostStream const&) = delete;
    HostStream& operator=(HostStream const&) = delete;
};
}
}
}
#endif // MIR_GRAPHICS_NESTED_HOST_STREAM_H_
