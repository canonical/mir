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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_H_
#define MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_H_

#include "mir_toolkit/mir_client_library.h"
#include "../client_buffer.h"
#include "android_registrar.h"

#include <system/window.h>
#include <memory>

namespace mir
{
namespace client
{
namespace android
{

class AndroidClientBuffer : public ClientBuffer
{
public:
    AndroidClientBuffer(std::shared_ptr<AndroidRegistrar> const&,
                        std::shared_ptr<MirBufferPackage> const&,
                        geometry::Size size, geometry::PixelFormat pf );
    ~AndroidClientBuffer();

    std::shared_ptr<MemoryRegion> secure_for_cpu_write();
    geometry::Size size() const;
    geometry::Stride stride() const;
    geometry::PixelFormat pixel_format() const;
    MirNativeBuffer get_native_handle();
    std::shared_ptr<MirBufferPackage> get_buffer_package() const;

    AndroidClientBuffer(const AndroidClientBuffer&) = delete;
    AndroidClientBuffer& operator=(const AndroidClientBuffer&) = delete;
private:
    void pack_native_window_buffer();
    const native_handle_t* convert_to_native_handle(const std::shared_ptr<MirBufferPackage>& package);

    std::shared_ptr<MirBufferPackage> creation_package;
    std::shared_ptr<const native_handle_t> native_handle;
    std::shared_ptr<AndroidRegistrar> buffer_registrar;

    const geometry::Rectangle rect;
    const geometry::PixelFormat buffer_pf;

    ANativeWindowBuffer native_window_buffer;
};

}
}
}
#endif /* MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_H_ */
