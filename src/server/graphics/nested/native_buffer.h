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

#ifndef MIR_GRAPHICS_NESTED_NATIVE_BUFFER_H_
#define MIR_GRAPHICS_NESTED_NATIVE_BUFFER_H_

#include "mir/geometry/size.h"
#include "mir/graphics/native_buffer.h"
#include "mir/fd.h"
#include "mir_toolkit/client_types.h"
#include "mir_toolkit/mir_native_buffer.h"
#include "mir_toolkit/extensions/fenced_buffers.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <tuple>
#include <chrono>
#include <functional>

namespace mir
{
namespace graphics
{
namespace nested
{

struct GraphicsRegion : MirGraphicsRegion
{
    GraphicsRegion();
    GraphicsRegion(MirBuffer*);
    ~GraphicsRegion();
    MirBufferLayout layout;
    MirBuffer* const handle;
};

class NativeBuffer : public graphics::NativeBuffer
{
public:
    virtual ~NativeBuffer() = default;
    virtual void sync(MirBufferAccess, std::chrono::nanoseconds) = 0;
    virtual MirBuffer* client_handle() const = 0;
    virtual std::unique_ptr<GraphicsRegion> get_graphics_region() = 0;
    virtual geometry::Size size() const = 0;
    virtual MirPixelFormat format() const = 0;
    virtual void on_ownership_notification(std::function<void()> const& fn) = 0;
    virtual MirBufferPackage* package() const = 0;
    virtual Fd fence() const = 0;
    virtual void set_fence(Fd) = 0;
    virtual std::tuple<EGLenum, EGLClientBuffer, EGLint*> egl_image_creation_hints() const = 0;
    virtual void available(MirBuffer* buffer) = 0;
protected:
    NativeBuffer() = default;
    NativeBuffer(NativeBuffer const&) = delete;
    NativeBuffer& operator=(NativeBuffer const&) = delete;
};
}
}
}

#endif /* MIR_GRAPHICS_NESTED_NATIVE_BUFFER_H_ */
