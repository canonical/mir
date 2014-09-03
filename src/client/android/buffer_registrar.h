/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_ANDROID_BUFFER_REGISTRAR_H_
#define MIR_CLIENT_ANDROID_BUFFER_REGISTRAR_H_

#include "mir_toolkit/common.h"
#include <mir_toolkit/mir_native_buffer.h>
#include "mir/graphics/native_buffer.h"
#include "mir/geometry/rectangle.h"
#include <cutils/native_handle.h>
#include <memory>

namespace mir
{
namespace client
{
class MemoryRegion;

namespace android
{
class BufferRegistrar
{
public:
    virtual ~BufferRegistrar() = default;
    virtual std::shared_ptr<graphics::NativeBuffer> register_buffer(
        MirBufferPackage const& package,
        MirPixelFormat pf) const = 0;
    virtual std::shared_ptr<char> secure_for_cpu(
        std::shared_ptr<graphics::NativeBuffer> const& handle,
        geometry::Rectangle const) = 0;
protected:
    BufferRegistrar() = default;
    BufferRegistrar(BufferRegistrar const&) = delete;
    BufferRegistrar& operator=(BufferRegistrar const&) = delete;
};

}
}
}
#endif /* MIR_CLIENT_ANDROID_BUFFER_REGISTRAR_H_ */
